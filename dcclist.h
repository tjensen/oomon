#ifndef __DCCLIST_H__
#define __DCCLIST_H__
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
#include <string>
#include <list>

// Boost C++ headers
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

// OOMon headers
#include "strtype"
#include "oomon.h"
#include "config.h"
#include "dcc.h"
#include "watch.h"


class DCCList
{
public:
  bool connect(const BotSock::Address address, const int port,
    const std::string & nick, const std::string & userhost,
    const BotSock::Address ircIp);
  bool listen(const std::string & nick, const std::string & userhost,
    const BotSock::Address ircIp);
  void setAllFD(fd_set & readset, fd_set & writeset);
  void processAll(const fd_set & readset, const fd_set & writeset);

  void sendAll(const std::string & message,
    const UserFlags flags = UserFlags::NONE(),
    const WatchSet & watches = WatchSet(), const class BotClient *skip = 0);
  void sendAll(const std::string & from, const std::string & text,
    const std::string & flags, const std::string & watches);
  void sendAll(const std::string & from, const std::string & skipId,
    const std::string & text, const std::string & flags,
    const std::string & watches);
  bool sendChat(const std::string & from, std::string message,
    const class BotClient *skip = 0);
  bool sendTo(const std::string & from, const std::string & clientId,
    const std::string & message);

  void who(class BotClient * client);
  void statsP(StrList & output);

  void status(class BotClient * client) const;

  DCCList() { };
  virtual ~DCCList() { };

private:
  typedef boost::shared_ptr<DCC> DCCPtr;
  typedef std::list<DCCPtr> SockList;
  SockList connections;
  SockList listeners;

  DCCPtr find(const std::string & id) const;

  class ListenProcessor
  {
  public:
    ListenProcessor(const fd_set & readset, const fd_set & writeset,
      SockList & connections) : _readset(readset), _writeset(writeset),
      _connections(connections) { }
    bool operator()(DCCPtr listener);
  private:
    const fd_set & _readset;
    const fd_set & _writeset;
    SockList & _connections;
  };

  class ClientProcessor
  {
  public:
    ClientProcessor(const fd_set & readset, const fd_set & writeset)
      : _readset(readset), _writeset(writeset) { }
    bool operator()(DCCPtr listener);
  private:
    const fd_set & _readset;
    const fd_set & _writeset;
  };

  class SendFilter
  {
  public:
    SendFilter(const std::string & message, const UserFlags flags, 
      const WatchSet & watches, const class BotClient *skip = 0)
      : _message(message), _flags(flags), _watches(watches), _skip(skip) { }

    void operator()(DCCPtr client)
    {
      if (this->_skip != client.get())
      {
        client->send(this->_message, this->_flags, this->_watches);
      }
    }

  private:
    const std::string _message;
    const UserFlags _flags;
    const WatchSet _watches;
    const BotClient * _skip;
  };

  class SendToFilter
  {
  public:
    SendToFilter(const std::string & handle, const std::string & message,
      const UserFlags flags, const WatchSet & watches) : _handle(handle),
      _message(message), _flags(flags), _watches(watches) { }

    bool operator()(DCCPtr client)
    {
      bool count = false;
      if (this->_handle == client->handle())
      {
        client->send(this->_message, this->_flags, this->_watches);
        count = true;
      }
      return count;
    }

  private:
    const std::string _handle;
    const std::string _message;
    const UserFlags _flags;
    const WatchSet _watches;
  };
};


extern DCCList clients;


#endif /* __DCCLIST_H__ */

