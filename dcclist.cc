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


DCCList::~DCCList()
{
  for (SockList::iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    delete (*pos);
  }

  for (SockList::iterator pos = this->listeners.begin();
    pos != this->listeners.end(); ++pos)
  {
    delete (*pos);
  }
}


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
    class DCC *newClient = new DCC;

    if (newClient->connect(ntohl(address), port, nick, userhost))
    {
      this->connections.push_back(newClient);
      return true;
    }
    else
    {
      delete newClient;
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
    class DCC *newListener = new DCC;

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
      delete newListener;
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
  for (SockList::iterator pos = this->listeners.begin();
    pos != this->listeners.end(); ++pos)
  {
    (*pos)->setFD(readset, writeset);
  }

  for (SockList::iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    (*pos)->setFD(readset, writeset);
  }
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
  for (SockList::iterator pos = this->listeners.begin();
    pos != this->listeners.end(); ++pos)
  {
    bool keepListener;

    try
    {
      keepListener = (*pos)->process(readset, writeset);
    }
    catch (OOMon::ready_for_accept)
    {
#ifdef DCCLIST_DEBUG
      std::cout << "DCC::process() threw exception: ready_for_accept" <<
	std::endl;
#endif
      try
      {
        DCC *newConnection = new DCC((*pos));
        if (newConnection->onConnect())
	{
	  this->connections.push_back(newConnection);
	}
	else
	{
	  delete newConnection;
	}
        keepListener = false;
      }
      catch (OOMon::errno_error & e)
      {
#ifdef DCCLIST_DEBUG
        std::cout << "DCC::accept() threw exception: " << e.what() << std::endl;
#endif
        keepListener = (EWOULDBLOCK == e.getErrno());
      }
    }
    catch (OOMon::timeout_error)
    {
#ifdef DCCLIST_DEBUG
      std::cout << "DCC listener socket timed out!" << std::endl;
#endif
      keepListener = false;
    }

    if (!keepListener)
    {
      DCC *tmp = (*pos);
      pos = this->listeners.erase(pos);
      delete tmp;
      --pos;
    }
  }

  for (SockList::iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    bool keepConnection;

    try
    {
      keepConnection = (*pos)->process(readset, writeset);
    }
    catch (OOMon::timeout_error)
    {
#ifdef DCCLIST_DEBUG
      std::cout << "DCC::process() threw exception: timeout_error" << std::endl;
#endif
      keepConnection = false;

      if (!(*pos)->isConnected())
      {
        server.notice((*pos)->getNick(), "DCC CHAT connection timed out.");
        Log::Write((*pos)->getUserhost() + " timed out on DCC connect");
      }
    }
    catch (OOMon::errno_error & e)
    {
      keepConnection = false;
#ifdef DCCLIST_DEBUG
      std::cout << "DCC::process() threw exception: errno_error: " <<
	e.what() << std::endl;
#endif
    }

    if (!keepConnection)
    {
      DCC *tmp = (*pos);
      pos = this->connections.erase(pos);

      if (tmp->isConnected())
      {
        ::SendAll(tmp->getUserhost() + " has disconnected", UF_AUTHED);
      }

      delete tmp;
      --pos;
    }
  }
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


// who(Output)
//
// Returns a list of connected users
//
void
DCCList::who(StrList & Output)
{
  Output.clear();
  if (this->connections.size() > 0)
  {
    Output.push_back("Client connections:");

    for (SockList::iterator pos = this->connections.begin();
      pos != this->connections.end(); ++pos)
    {
      if ((*pos)->isConnected())
      {
        Output.push_back((*pos)->getFlags() + " " + (*pos)->getHandle() +
	  " (" + (*pos)->getUserhost() + ") " + IntToStr((*pos)->getIdle()));
      }
      else
      {
        Output.push_back((*pos)->getFlags() + " " + (*pos)->getHandle() +
	  " (" + (*pos)->getUserhost() + ") -connecting-");
      }
    }
  }
  else
  {
    Output.push_back("No client connections!");
  }
}


// statsP(Output)
//
// Returns a list of connected users (in a pseudo-"stats p" format)
//
void
DCCList::statsP(StrList & Output)
{
  int operCount = 0;

  Output.clear();
  for (SockList::iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    if ((*pos)->isConnected() && (*pos)->isOper())
    {
      std::string notice((*pos)->getHandle(false));
      if (vars[VAR_STATSP_SHOW_USERHOST]->getBool())
      {
	notice += " (" + (*pos)->getUserhost() + ")";
      }
      if (vars[VAR_STATSP_SHOW_IDLE]->getBool())
      {
	notice += " Idle: " + IntToStr((*pos)->getIdle());
      }
      Output.push_back(notice);
      operCount++;
    }
  }
  Output.push_back(IntToStr(operCount) + " OOMon opers");
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
  bool result = false;

  for (SockList::iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    if (handle == (*pos)->getHandle())
    {
      (*pos)->send(message, flags, watches);
      result = true;
    }
  }

  return result;
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

