#ifndef __REMOTELIST_H__
#define __REMOTELIST_H__
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

#include "oomon.h"

#include <string>
#include <list>

#include "strtype"
#include "remote.h"
#include "botsock.h"
#include "links.h"


class RemoteList
{
public:
  RemoteList(void) { this->shutdown(); };
  ~RemoteList(void) { };

  void shutdown(void);

  void setFD(fd_set & readset, fd_set & writeset) const;
  void processAll(const fd_set & readset, const fd_set & writeset);

  bool connect(const std::string & handle);
  void listen(void);

  void send(const std::string &, const std::string &,
    const std::string &, const std::string &,
    const class Remote *exception = 0);

  void sendChat(const std::string & from, const std::string & text,
    const Remote *exception = (Remote *) 0);
  void sendBotJoin(const std::string & oldnode, const std::string & newnode,
    const Remote *exception = (Remote *) 0);
  void sendBotPart(const std::string & from, const std::string & node,
    const Remote *exception = (Remote *) 0);

  void conn(const std::string & From, const std::string & To,
    const std::string & Target);
  void disconn(const std::string & From, const std::string & To,
    const std::string & Target);

  void getLinks(StrList & Output);
  void getBotNet(BotLinkList & list, const Remote *exception = (Remote *) 0);

  bool isLinkedDirectlyToMe(const std::string &) const;
  Remote *findBot(const std::string &) const;
  bool isConnected(const std::string &) const;

private:
  bool listen(const std::string & address, const BotSock::Port & port);
  void shutdownListeners(void);

  typedef std::list<BotSock *> ListenerList;
  typedef std::list<Remote *> ConnectionList;

  ListenerList _listeners;
  ConnectionList _connections;
};

extern RemoteList remotes;

#endif /* __REMOTELIST_H__ */

