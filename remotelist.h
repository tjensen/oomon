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

  void preSelect(fd_set & readset, fd_set & writeset) const;
  void postSelect(const fd_set & readset, const fd_set & writeset);

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
      ConnectionList & connections) : readset_(readset), writeset_(writeset),
      connections_(connections) { }
    bool operator()(BotSock::ptr s);
  private:
    const fd_set & readset_;
    const fd_set & writeset_;
    ConnectionList & connections_;
  };

  class RemoteProcess
  {
  public:
    RemoteProcess(const fd_set & readset, const fd_set & writeset)
      : readset_(readset), writeset_(writeset) { }
    bool operator()(RemotePtr r);
  private:
    const fd_set & readset_;
    const fd_set & writeset_;
  };

  class SendChat
  {
  public:
    SendChat(const std::string & from, const std::string & text,
      const Remote *skip = 0) : from_(from), text_(text),
      skip_(skip) { }
    void operator()(RemotePtr r)
    {
      if (r.get() != this->skip_)
      {
        r->sendChat(this->from_, this->text_);
      }
    }
  private:
    const std::string & from_;
    const std::string & text_;
    const Remote * skip_;
  };

  class SendBotJoinPart
  {
  public:
    SendBotJoinPart(const bool join, const std::string node1,
      const std::string node2, const Remote *skip = 0)
      : join_(join), node1_(node1), node2_(node2), skip_(skip) { }
    void operator()(RemotePtr r)
    {
      if (r.get() != this->skip_)
      {
	if (this->join_)
	{
          r->sendBotJoin(this->node1_, this->node2_);
	}
	else
	{
          r->sendBotPart(this->node1_, this->node2_);
	}
      }
    }
  private:
    const bool join_;
    const std::string & node1_;
    const std::string & node2_;
    const Remote * skip_;
  };

  ListenerList listeners_;
  ConnectionList connections_;
};

extern RemoteList remotes;

#endif /* __REMOTELIST_H__ */

