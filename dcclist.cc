// ===========================================================================
// OOMon - Objected Oriented Monitor Bot
// Copyright (C) 2004  Timothy L. Jensen
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
#include <string>
#include <list>
#include <algorithm>
#include <cerrno>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

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
DCCList::connect(const BotSock::Address address, const int port,
  const std::string & nick, const std::string & userhost,
  const BotSock::Address ircIp)
{
  try
  {
    DCCPtr newClient(new DCC);

    if (newClient->connect(ntohl(address), port, nick, userhost, ircIp))
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
DCCList::listen(const std::string & nick, const std::string & userhost,
  const BotSock::Address ircIp)
{
  try
  {
    DCCPtr newListener(new DCC);

    if (newListener->listen(nick, userhost, ircIp, htons(config.dccPort())))
    {
      std::string request("DCC CHAT chat ");
      request += boost::lexical_cast<std::string>(htonl(
	server.getLocalAddress()));
      request += ' ';
      request += boost::lexical_cast<std::string>(htons(
	newListener->getLocalPort()));

      server.ctcp(nick, request);
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
  FDSetter<DCCPtr> fds(readset, writeset);

  std::for_each(this->listeners.begin(), this->listeners.end(), fds);
  std::for_each(this->connections.begin(), this->connections.end(), fds);
}


bool
DCCList::ListenProcessor::operator()(DCCPtr listener)
{
  bool remove;

  try
  {
    remove = !listener->process(this->readset_, this->writeset_);
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
      DCCPtr tmp(new DCC(listener.get()));
      newConnection.swap(tmp);
    }
    catch (OOMon::errno_error & e)
    {
      std::cerr << "DCC::accept() threw exception: " << e.what() << std::endl;
      addClient = (EWOULDBLOCK == e.getErrno());
    }

    if (addClient && newConnection->onConnect())
    {
      this->connections_.push_back(newConnection);
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
    remove = !client->process(this->readset_, this->writeset_);
  }
  catch (OOMon::timeout_error)
  {
    remove = true;
#ifdef DCCLIST_DEBUG
    std::cout << "DCC::process() threw exception: timeout_error" << std::endl;
#endif
    if (!client->isConnected())
    {
      server.notice(client->nick(), "DCC CHAT connection timed out.");
      Log::Write(client->userhost() + " timed out on DCC connect");
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
    ::SendAll(client->userhost() + " has disconnected", UserFlags::AUTHED,
      WatchSet(), client.get());
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


// sendChat(From, Text, skip)
//
// Writes a chat message to all DCC connections who don't match skip.
// Returns false if user doesn't exist.
//
bool
DCCList::sendChat(const std::string & From, std::string Text,
  const BotClient *skip)
{
  if (!Text.empty())
  {
    if (Text[0] != '\001')
    {
      this->sendAll("<" + From + "> " + Text, UserFlags::AUTHED, WATCH_CHAT,
	skip);
    }
    else
    {
      // Need to be ready for CTCP ACTION type stuff here.
      Text.erase((std::string::size_type) 0, 1);
      std::string::size_type i = Text.find('\001');
      if (i != std::string::npos)
        Text = Text.substr(0, i);
      std::string Command = FirstWord(Text);
      if (!Text.empty() && (Command == "ACTION"))
      {
        this->sendAll("<-> " + From + " " + Text, UserFlags::AUTHED, WATCH_CHAT,
	  skip);
      }
    }  
  }
  return true;
}


bool
DCCList::sendTo(const std::string & from, const std::string & clientId,
  const std::string & message)
{
  DCCPtr client(this->find(clientId));
  bool result = false;

  if (0 != client.get())
  {
    std::string notice("[");
    notice += from;
    notice += "] ";
    notice += message;

    client->send(notice);
    result = true;
  }

  return result;
}


// who(client)
//
// Returns a list of connected users
//
void
DCCList::who(BotClient * client)
{
  if (this->connections.empty())
  {
    client->send("No client connections!");
  }
  else
  {
    client->send("Client connections:");

    std::for_each(this->connections.begin(), this->connections.end(),
      boost::bind(&DCC::who, _1, client));
  }
}


// statsP(Output)
//
// Returns a list of connected users (in a pseudo-"stats p" format)
//
void
DCCList::statsP(StrList & output)
{
  output.clear();
  int operCount = 0;

  for (SockList::const_iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    DCCPtr copy(*pos);

    if (copy->statsP(output)) ++operCount;
  }

  output.push_back(boost::lexical_cast<std::string>(operCount) + " OOMon oper" +
    (operCount == 1 ? "" : "s"));
}


// sendAll()
//
// Writes a status message to all connections with the required flags and
// watches.
//
void
DCCList::sendAll(const std::string & message, const UserFlags flags,
  const WatchSet & watches, const BotClient *skip)
{
  SendFilter filter(message, flags, watches, skip);

  std::for_each(this->connections.begin(), this->connections.end(), filter);
}


void
DCCList::sendAll(const std::string & from, const std::string & text,
  const std::string & flags, const std::string & watches)
{
  try
  {
    UserFlags f = UserFlags(flags, ',');
    WatchSet w = WatchSet::getWatchValues(watches);

    std::string notice("[");
    notice += from;
    notice += "] ";
    notice += text;

    this->sendAll(notice, f, w);
  }
  catch (OOMon::invalid_watch_name & e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch (UserFlags::invalid_flag & e)
  {
    std::cerr << e.what() << std::endl;
  }
}


void
DCCList::sendAll(const std::string & from, const std::string & skipId,
  const std::string & text, const std::string & flags,
  const std::string & watches)
{
  try
  {
    UserFlags f = UserFlags(flags, ',');
    WatchSet w = WatchSet::getWatchValues(watches);

    std::string notice("[");
    notice += from;
    notice += "] ";
    notice += text;

    for (SockList::iterator pos = this->connections.begin();
      pos != this->connections.end(); ++pos)
    {
      DCCPtr copy(*pos);

      if (!Same(copy->id(), skipId))
      {
	copy->send(notice, f, w);
      }
    }
  }
  catch (OOMon::invalid_watch_name & e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch (UserFlags::invalid_flag & e)
  {
    std::cerr << e.what() << std::endl;
  }
}


// status(output)
//
// Report status information about DCC connections
//
void
DCCList::status(BotClient * client) const
{
  client->send("DCC Connections: " +
    boost::lexical_cast<std::string>(this->connections.size()));
  client->send("DCC Listeners: " +
    boost::lexical_cast<std::string>(this->listeners.size()));
}


DCCList::DCCPtr
DCCList::find(const std::string & id) const
{
  DCCPtr result;

  for (SockList::const_iterator pos = this->connections.begin();
    pos != this->connections.end(); ++pos)
  {
    DCCPtr copy(*pos);

    if (0 == copy->id().compare(id))
    {
      result = copy;
      break;
    }
  }

  return result;
}

