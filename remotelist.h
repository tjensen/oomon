#ifndef __REMOTELIST_H__
#define __REMOTELIST_H__
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

  bool connect(class BotClient * from, const std::string & handle);
  void listen(void);

  void sendBroadcast(const std::string & from, const std::string & text,
    const std::string & flags = "NONE", const std::string & watches = "NONE");
  void sendBroadcastPtr(const std::string & from, const Remote *skip,
    const std::string & text, const std::string & flags = "NONE",
    const std::string & watches = "NONE");
  void sendBroadcastId(const std::string & from, const std::string & skipId,
    const std::string & skipBot, const std::string & text,
    const std::string & flags = "NONE", const std::string & watches = "NONE");

  void sendNotice(const std::string & from, const std::string & clientId,
    const std::string & clientBot, const std::string & text);

  void sendChat(const std::string & from, const std::string & text,
    const Remote *skip = 0);
  void sendBotJoin(const std::string & oldnode, const std::string & newnode,
    const Remote *skip = 0);
  void sendBotPart(const std::string & from, const std::string & node,
    const Remote *skip = 0);
  void sendRemoteCommand(class BotClient * client, const std::string & bot,
    const std::string & command, const std::string & parameters);
  void sendAllRemoteCommand(BotClient * from, const std::string & command,
    const std::string & parameters);

  void cmdConn(BotClient * from, const std::string & command,
    std::string parameters);
  void cmdDisconn(BotClient * from, const std::string & command,
    std::string parameters);

  void getLinks(class BotClient * client);
  void getBotNet(BotLinkList & list, const Remote *skip = 0);

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
      const Remote *skip = 0) : _from(from), _text(text),
      _skip(skip) { }
    void operator()(RemotePtr r)
    {
      if (r.get() != this->_skip)
      {
        r->sendChat(this->_from, this->_text);
      }
    }
  private:
    const std::string & _from;
    const std::string & _text;
    const Remote *_skip;
  };

  class SendBotJoinPart
  {
  public:
    SendBotJoinPart(const bool join, const std::string node1,
      const std::string node2, const Remote *skip = 0)
      : _join(join), _node1(node1), _node2(node2), _skip(skip) { }
    void operator()(RemotePtr r)
    {
      if (r.get() != this->_skip)
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
    const Remote *_skip;
  };

  ListenerList _listeners;
  ConnectionList _connections;
};

extern RemoteList remotes;

#endif /* __REMOTELIST_H__ */

