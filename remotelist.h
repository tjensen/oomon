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
  typedef boost::shared_ptr<Remote> RemotePtr;

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
  RemotePtr findBot(const std::string &) const;
  bool isConnected(const std::string &) const;

private:
  bool listen(const std::string & address, const BotSock::Port & port);

  typedef std::list<BotSock::ptr> ListenerList;
  typedef std::list<RemotePtr> ConnectionList;

  class ListenProcess
  {
  public:
    ListenProcess(const fd_set & readset, const fd_set & writeset,
      ConnectionList & connections) : _readset(readset), _writeset(writeset),
      _connections(connections) { }
    bool operator()(BotSock::ptr s);
  private:
    const fd_set & _readset;
    const fd_set & _writeset;
    ConnectionList & _connections;
  };

  class RemoteProcess
  {
  public:
    RemoteProcess(const fd_set & readset, const fd_set & writeset)
      : _readset(readset), _writeset(writeset) { }
    bool operator()(RemotePtr r);
  private:
    const fd_set & _readset;
    const fd_set & _writeset;
  };

  class SendChat
  {
  public:
    SendChat(const std::string & from, const std::string & text,
      const Remote *exception = (Remote *) 0) : _from(from), _text(text),
      _exception(exception) { }
    void operator()(RemotePtr r)
    {
      if (r.get() != this->_exception)
      {
        r->sendChat(this->_from, this->_text);
      }
    }
  private:
    const std::string & _from;
    const std::string & _text;
    const Remote *_exception;
  };

  class SendBotJoinPart
  {
  public:
    SendBotJoinPart(const bool join, const std::string node1,
      const std::string node2, const Remote *exception = (Remote *) 0)
      : _join(join), _node1(node1), _node2(node2), _exception(exception) { }
    void operator()(RemotePtr r)
    {
      if (r.get() != this->_exception)
      {
	if (this->_join)
	{
          r->sendBotJoin(this->_node1, this->_node2);
	}
	else
	{
          r->sendBotPart(this->_node1, this->_node2);
	}
      }
    }
  private:
    const bool _join;
    const std::string & _node1;
    const std::string & _node2;
    const Remote *_exception;
  };

  ListenerList _listeners;
  ConnectionList _connections;
};

extern RemoteList remotes;

#endif /* __REMOTELIST_H__ */

