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

#include <string>
#include <list>

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

  std::string getHandle(void) const { return this->_handle; };
  std::string getHostname(void) const { return this->_hostname; };
  void getBotNetBranch(BotLinkList & list) const;

  bool isClient(void) const { return this->_client; };
  bool isServer(void) const { return !this->_client; };

  bool authenticated(void) const
  {
    return Remote::STAGE_AUTHED == this->_stage;
  }
  bool ready(void) const
  {
    return Remote::STAGE_READY == this->_stage;
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
  int sendCommand(const std::string & from, const std::string & to,
    const std::string & clientId, const std::string & command,
    const std::string & parameters);

  bool onConnect(void);

  virtual UserFlags flags(void) const;
  virtual std::string handle(void) const;
  virtual std::string bot(void) const;
  virtual std::string id(void) const;
  virtual void send(const std::string & text);

  void setFD(fd_set & readset, fd_set & writeset) const
    { this->_sock.setFD(readset, writeset); }
  bool process(const fd_set & readset, const fd_set & writeset)
    { return this->_sock.process(readset, writeset); }
  bool isConnected(void) const { return this->_sock.isConnected(); }
  bool connect(const std::string & hostname, BotSock::Port port)
    { return this->_sock.connect(hostname, port); }

protected:
  bool onRead(std::string text);

private:
  bool isAuthorized(void) const;
  bool authenticate(std::string text);
  bool parse(std::string text);

  int sendVersion(void);
  int sendAuth(void);
  int sendUnknownCommand(const std::string & command);
  int sendCommand(const std::string & from, const std::string & command,
    const std::string & parameters = "");
  int sendMyBotNet(void);

  bool onBotJoin(const std::string & from, const std::string & node);
  bool onBotPart(const std::string & from, const std::string & node);
  bool onChat(const std::string & from, const std::string & text);
  bool onCommand(const std::string & from, std::string text);
  bool onBroadcast(const std::string & from, std::string text);
  bool onNotice(const std::string & from, std::string text);

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

  Links _children;
  std::string _handle;
  std::string _hostname;
  Stage _stage;
  bool _client;
  std::string _sendQ;
  CommandParser _parser;
  BotSock _sock;
  bool _targetEstablished;
  std::string _clientHandle;
  std::string _clientBot;
  std::string _clientId;
  UserFlags _clientFlags;

  static const std::string PROTOCOL_NAME;
  static const int PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR;
};


#endif /* __REMOTE_H__ */

