#ifndef __REMOTE_H__
#define __REMOTE_H__
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

// Std C++ Headers
#include <string>
#include <map>
#include <ctime>

// Boost C++ Headers
#include <boost/function.hpp>

// OOMon Headers
#include "strtype"
#include "links.h"
#include "botsock.h"
#include "botclient.h"
#include "cmdparser.h"


class Remote : public BotClient
{
public:
  Remote(const std::string & handle);
  Remote(BotSock *listener);
  virtual ~Remote(void);

  bool isConnectedTo(const std::string &) const;
  void getLinks(class BotClient * client, const std::string &) const;

  std::string getHandle(void) const { return this->handle_; };
  std::string getHostname(void) const { return this->hostname_; };
  void getBotNetBranch(BotLinkList & list) const;

  bool isClient(void) const { return this->client_; };
  bool isServer(void) const { return !this->client_; };

  bool authenticated(void) const
  {
    return Remote::STAGE_AUTHED == this->stage_;
  }
  bool ready(void) const
  {
    return Remote::STAGE_READY == this->stage_;
  }

  int sendBroadcast(const std::string & from, const std::string & text,
    const std::string & flags, const std::string & watches);
  int sendBroadcastPtr(const std::string & from, const Remote *skip,
    const std::string & text, const std::string & flags,
    const std::string & watches);
  int sendBroadcastId(const std::string & from, const std::string & skipId,
    const std::string & skipBot, const std::string & text,
    const std::string & flags, const std::string & watches);

  int sendNotice(const std::string & from, const std::string & skipId,
    const std::string & skipBot, const std::string & text);

  int sendError(const std::string & text);
  int sendChat(const std::string & from, const std::string & text);
  int sendBotJoin(const std::string & oldnode, const std::string & newnode);
  int sendBotPart(const std::string & from, const std::string & node);

  int sendRemoteCommand(const std::string & from, const std::string & to,
    const std::string & clientId, const std::string & command,
    const std::string & parameters);
  int sendAllRemoteCommandPtr(BotClient * skip, const std::string & command,
    const std::string & parameters);

  bool onConnect(void);

  virtual UserFlags flags(void) const;
  virtual std::string handle(void) const;
  virtual std::string bot(void) const;
  virtual std::string id(void) const;
  virtual void send(const std::string & text);

  void preSelect(fd_set & readset, fd_set & writeset) const;
  bool postSelect(const fd_set & readset, const fd_set & writeset);

  bool isConnected(void) const { return this->sock_.isConnected(); }
  time_t getIdle(void) const { return this->sock_.getIdle(); }
  bool connect(const std::string & hostname, BotSock::Port port)
    { return this->sock_.connect(hostname, port); }

protected:
  bool onRead(std::string text);

private:
  bool isAuthorized(void) const;
  bool authenticate(std::string text);
  bool parse(std::string text);

  void configureCallbacks(void);

  void registerCommand(const std::string & command,
    bool (Remote::*callback)(const std::string & from,
    const std::string & command, const StrVector & parameters));
  void unregisterCommand(const std::string & command);

  int sendVersion(void);
  int sendPing(void);
  int sendAuth(void);
  int sendUnknownCommand(const std::string & command);
  int sendSyntaxError(const std::string & command);
  int sendCommand(const std::string & from, const std::string & command,
    const std::string & parameters, const bool queue = true);
  int sendMyBotNet(void);

  int write(const std::string & text);

  // Callbacks
  bool onError(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onPing(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onPong(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onAuth(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onBotJoin(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onBotPart(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onChat(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onCommand(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onBroadcast(const std::string & from, const std::string & command,
    const StrVector & parameters);
  bool onNotice(const std::string & from, const std::string & command,
    const StrVector & parameters);

  static bool isCompatibleProtocolVersion(std::string text);

  // Stages:
  //  INIT - have not yet received the protocol version from the remote bot
  //  GOODVERSION - remote bot supports a compatible protocol but has not yet
  //                authenticated itself
  //  AUTHED - remote bot has authenticated itself but has not yet relayed
  //           its botnet
  //  READY - remote bot has relayed its botnet and link is ready for normal
  //          communications
  enum Stage { STAGE_INIT, STAGE_GOODVERSION, STAGE_AUTHED, STAGE_READY };

  typedef boost::function<bool (const std::string &, const std::string &,
    const StrVector &)> CommandCallback;
  typedef std::map<std::string, CommandCallback> CommandMap;

  Links children_;
  std::string handle_;
  std::string hostname_;
  Stage stage_;
  bool client_;
  std::string sendQ_;
  CommandParser parser_;
  BotSock sock_;
  bool targetEstablished_;
  std::string clientHandle_;
  std::string clientBot_;
  std::string clientId_;
  UserFlags clientFlags_;
  CommandMap commands;

  static const std::string PROTOCOL_NAME;
  static const int PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR;
};


#endif /* __REMOTE_H__ */

