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

  void preSelect(fd_set & readset, fd_set & writeset);
  void postSelect(const fd_set & readset, const fd_set & writeset);

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
      SockList & connections) : readset_(readset), writeset_(writeset),
      connections_(connections) { }
    bool operator()(DCCPtr listener);
  private:
    const fd_set & readset_;
    const fd_set & writeset_;
    SockList & connections_;
  };

  class ClientProcessor
  {
  public:
    ClientProcessor(const fd_set & readset, const fd_set & writeset)
      : readset_(readset), writeset_(writeset) { }
    bool operator()(DCCPtr listener);
  private:
    const fd_set & readset_;
    const fd_set & writeset_;
  };

  class SendFilter
  {
  public:
    SendFilter(const std::string & message, const UserFlags flags, 
      const WatchSet & watches, const class BotClient *skip = 0)
      : message_(message), flags_(flags), watches_(watches), skip_(skip) { }

    void operator()(DCCPtr client)
    {
      if (this->skip_ != client.get())
      {
        client->send(this->message_, this->flags_, this->watches_);
      }
    }

  private:
    const std::string message_;
    const UserFlags flags_;
    const WatchSet watches_;
    const BotClient * skip_;
  };

  class SendToFilter
  {
  public:
    SendToFilter(const std::string & handle, const std::string & message,
      const UserFlags flags, const WatchSet & watches) : handle_(handle),
      message_(message), flags_(flags), watches_(watches) { }

    bool operator()(DCCPtr client)
    {
      bool count = false;
      if (this->handle_ == client->handle())
      {
        client->send(this->message_, this->flags_, this->watches_);
        count = true;
      }
      return count;
    }

  private:
    const std::string handle_;
    const std::string message_;
    const UserFlags flags_;
    const WatchSet watches_;
  };
};


extern DCCList clients;


#endif /* __DCCLIST_H__ */

