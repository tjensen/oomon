// ===========================================================================
// OOMon - Objected Oriented Monitor Bot
// Copyright (C) 2003  Timothy L. Jensen
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ===========================================================================

// $Id$

// Std C++ headers
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>

// Std C headers
#include <errno.h>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "dcclist.h"
#include "dcc.h"
#include "botexcept.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "irc.h"
#include "watch.h"
#include "vars.h"


#ifdef DEBUG
# define DCCLIST_DEBUG
#endif


DCCList clients;


// connect(address, port, nick, userhost)
//
// Makes a DCC connection to the given address and port.
//
// Returns true if no errors ocurred.
//
bool
DCCList::connect(const DCC::Address address, const int port,
  const std::string & nick, const std::string & userhost)
{
  try
  {
    class DCCPtr newClient(new DCC);

    if (newClient->connect(ntohl(address), port, nick, userhost))
    {
      this->connections.push_back(newClient);
      return true;
    }
    else
    {
      return false;
    }
  }
  catch (OOMon::socket_error)
  {
    return false;
  }
}

// listen(nick, userhost)
//
// Listens for a DCC connection
//
// Returns 0 if no errors ocurred.
//
bool
DCCList::listen(const std::string & nick, const std::string & userhost)
{
  try
  {
    class DCCPtr newListener(new DCC);

    if (newListener->listen(nick, userhost))
    {
      server.ctcp(nick, std::string("DCC CHAT chat ") +
        ULongToStr(htonl(server.getLocalAddress())) + " " +
        ULongToStr(htons(newListener->getLocalPort())));
      listeners.push_back(newListener);
      return true;
    }
    else
    {
      return false;
    }
  }
  catch (OOMon::socket_error)
  {
    return false;
  }
}


// setAllFD(readset)
//
// Do this if you want to be able to receive data :P
//
void
DCCList::setAllFD(fd_set & readset, fd_set & writeset)
{
  BotSock::FDSetter s(readset, writeset);

  std::for_each(this->listeners.begin(), this->listeners.end(), s);
  std::for_each(this->connections.begin(), this->connections.end(), s);
}


bool
DCCList::ListenProcessor::operator()(DCCPtr listener)
{
  bool remove;

  try
  {
    remove = !listener->process(this->_readset, this->_writeset);
  }
  catch (OOMon::ready_for_accept)
  {
#ifdef DCCLIST_DEBUG
    std::cout << "DCC::process() threw exception: ready_for_accept" <<
      std::endl;
#endif

    remove = true;

    DCCPtr newConnection;
    bool addClient = true;

    try
    {
      newConnection = DCCPtr(new DCC(listener.get()));
    }
    catch (OOMon::errno_error & e)
    {
      std::cerr << "DCC::accept() threw exception: " << e.what() << std::endl;
      addClient = (EWOULDBLOCK == e.getErrno());
    }

    if (addClient && newConnection->onConnect())
    {
      this->_connections.push_back(newConnection);
    }
  }
  catch (OOMon::timeout_error)
  {
    std::cerr << "DCC listener socket timed out!" << std::endl;
    remove = true;
  }

  return remove;
}


bool
DCCList::ClientProcessor::operator()(DCCPtr client)
{
  bool remove;

  try
  {
    remove = !client->process(this->_readset, this->_writeset);
  }
  catch (OOMon::timeout_error)
  {
    remove = true;
#ifdef DCCLIST_DEBUG
    std::cout << "DCC::process() threw exception: timeout_error" << std::endl;
#endif
    if (!client->isConnected())
    {
      server.notice(client->getNick(), "DCC CHAT connection timed out.");
      Log::Write(client->getUserhost() + " timed out on DCC connect");
    }
  }
  catch (OOMon::errno_error & e)
  {
    remove = true;
    std::cerr << "DCC::process() threw exception: errno_error: " <<
      e.what() << std::endl;
  }

  if (remove && client->isConnected())
  {
    ::SendAll(client->getUserhost() + " has disconnected", UF_AUTHED);
  }

  return remove;
}


// processAll(readset, writeset)
//
// Processes any received data at each DCC connection.
//
// Returns true if no errors ocurred.
//
void
DCCList::processAll(const fd_set & readset, const fd_set & writeset)
{
  ListenProcessor lp(readset, writeset, this->connections);
  this->listeners.remove_if(lp);
    
  ClientProcessor cp(readset, writeset);
  this->connections.remove_if(cp);
}


// sendChat(From, Text, exception)
//
// Writes a chat message to all DCC connections who don't match the
// exception.  Returns false if user doesn't exist.
//
bool
DCCList::sendChat(const std::string & From, std::string Text,
  const class DCC *exception)
{
  if (Text != "")
  {
    if (Text[0] != '\001')
    {
      this->sendAll("<" + From + "> " + Text, UF_AUTHED, WATCH_CHAT,
        exception);
    }
    else
    {
      // Need to be ready for CTCP ACTION type stuff here.
      Text.erase((std::string::size_type) 0, 1);
      std::string::size_type i = Text.find('\001');
      if (i != std::string::npos)
        Text = Text.substr(0, i);
      std::string Command = FirstWord(Text);
      if ((Command == "ACTION") && (Text != ""))
      {
        this->sendAll("<-> " + From + " " + Text, UF_AUTHED, WATCH_CHAT,
	  exception);
      }
    }  
  }
  return true;
}


void
DCCList::WhoList::operator()(DCCPtr client)
{
  if (client->isConnected())
  {
    this->_output.push_back(client->getFlags() + " " + client->getHandle() +
      " (" + client->getUserhost() + ") " + IntToStr(client->getIdle()));
  }
  else
  {
    this->_output.push_back(client->getFlags() + " " + client->getHandle() +
      " (" + client->getUserhost() + ") -connecting-");
  }
}


// who(Output)
//
// Returns a list of connected users
//
void
DCCList::who(StrList & output)
{
  output.clear();

  if (this->connections.empty())
  {
    output.push_back("No client connections!");
  }
  else
  {
    output.push_back("Client connections:");

    std::for_each(this->connections.begin(), this->connections.end(),
      WhoList(output));
  }
}


bool
DCCList::StatsPList::operator()(DCCPtr client)
{
  bool count = false;

  if (client->isConnected() && client->isOper())
  {
    std::string notice(client->getHandle(false));

    if (vars[VAR_STATSP_SHOW_USERHOST]->getBool())
    {
      notice += " (" + client->getUserhost() + ")";
    }
    if (vars[VAR_STATSP_SHOW_IDLE]->getBool())
    {
      notice += " Idle: " + IntToStr(client->getIdle());
    }

    this->_output.push_back(notice);
    count = true;
  }

  return count;
}


// statsP(Output)
//
// Returns a list of connected users (in a pseudo-"stats p" format)
//
void
DCCList::statsP(StrList & output)
{
  output.clear();

  int operCount = std::count_if(this->connections.begin(),
    this->connections.end(), StatsPList(output));

  output.push_back(IntToStr(operCount) + " OOMon oper" +
    (operCount == 1 ? "" : "s"));
}


// sendAll()
//
// Writes a status message to all connections with the required flags and
// watches.
//
void
DCCList::sendAll(const std::string & message, const int flags,
  const WatchSet & watches, const DCC *exception)
{
  SendFilter filter(message, flags, watches, exception);

  std::for_each(this->connections.begin(), this->connections.end(), filter);
}


// sendAll()
//
// Writes a status message to all connections with the required flags and
// watches.
//
void
DCCList::sendAll(const std::string & message, const int flags,
  const Watch watch, const DCC *exception)
{
  WatchSet watches;
  watches.add(watch);

  this->sendAll(message, flags, watches, exception);
}


// sendTo()
//
// Writes a status message to all connections matching the nickname.
//
bool
DCCList::sendTo(const std::string & handle, const std::string & message,
  const int flags, const WatchSet & watches)
{
  return (0 != std::count_if(this->connections.begin(), this->connections.end(),
    SendToFilter(handle, message, flags, watches)));
}


// SendTo()
//
// Writes a status message to all connections matching the nickname.
//
bool
DCCList::sendTo(const std::string & handle, const std::string & message,
  const int flags, const Watch watch)
{
  WatchSet watches;
  watches.add(watch);
  return this->sendTo(handle, message, flags, watches);
}


// status(output)
//
// Report status information about DCC connections
//
void
DCCList::status(StrList & output) const
{
  output.push_back("DCC Connections: " + IntToStr(this->connections.size()));
  output.push_back("DCC Listeners: " + IntToStr(this->listeners.size()));
}

