#ifndef __DCC_H__
#define __DCC_H__
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

// Std C++ Headers
#include <string>
#include <ctime>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "config.h"
#include "botsock.h"
#include "watch.h"
#include "cmdparser.h"
#include "botclient.h"


class DCC : public BotClient
{
public:
  static void init(void);

  DCC(const std::string & nick = "", const std::string & userhost = "",
    const BotSock::Address userIp = INADDR_NONE);
  DCC(DCC *listener);

  bool connect(const BotSock::Address address, const BotSock::Port port,
    const std::string & nick, const std::string & userhost,
    const BotSock::Address ircIp);
  bool listen(const std::string & nick, const std::string & userhost,
    const BotSock::Address ircIp, const BotSock::Port port,
    const int backlog = 1);

  void motd(void);

  void chat(const std::string & text);
  void send(const std::string & message, const UserFlags flags,
    const WatchSet & watches = WatchSet());

  virtual void send(const std::string & message);
  virtual UserFlags flags(void) const { return this->flags_; }
  virtual std::string handle(void) const { return this->handle_; }
  virtual std::string bot(void) const { return config.nickname(); }
  virtual std::string id(void) const { return this->id_; }

  void flags(const UserFlags f) { this->flags_ = f; }
  void handle(const std::string & h) { this->handle_ = h; }
  std::string humanReadableFlags(void) const;
  void who(class BotClient * client) const;
  bool statsP(StrList & output) const;

  std::string userhost(void) const { return this->userhost_; }
  std::string nick(void) const { return this->nick_; }

  bool isOper(void) const
    { return (this->flags().has(UserFlags::OPER)); }
  bool isAuthed(void) const
    { return (this->flags().has(UserFlags::AUTHED)); }

  bool onConnect(void);

  BotSock::Port getLocalPort(void) const { return this->sock_.getLocalPort(); }
  bool isConnected(void) const { return this->sock_.isConnected(); }
  bool process(const fd_set & readset, const fd_set & writeset)
    { return this->sock_.process(readset, writeset); }
  void setFD(fd_set & readset, fd_set & writeset)
    { this->sock_.setFD(readset, writeset); }
  std::time_t idleTime(void) const { return this->sock_.getIdle(); }

protected:
  bool onRead(std::string text);

private:
  BotSock sock_;
  bool echoMyChatter_;
  std::string userhost_;
  BotSock::Address ircIp_;
  std::string nick_;
  WatchSet watches_;
  CommandParser parser_;
  std::string handle_;
  std::string id_;
  UserFlags flags_;

  static bool ignoreUnknownCommand;
  static bool statspShowIdle;
  static bool statspShowUserhost;
  static bool unauthedMayChat;

  bool parse(std::string text);

  void addCommands(void);

  void addCommand(const std::string & command,
    void (DCC::*)(class BotClient *from, const std::string & command,
    std::string parameters), const UserFlags flags,
    const int options = CommandParser::NONE);

  void cmdHelp(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdAuth(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdQuit(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdEcho(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdWatch(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdChat(class BotClient *from, const std::string & command,
    std::string parameters);

  void cmdLocops(class BotClient *from, const std::string & command,
    std::string parameters);

  void loadConfig(void);

  class quit : public OOMon::oomon_error
  {
  public:
    quit(const std::string & arg) : oomon_error(arg) { };
  };
};


#endif /* __DCC_H__ */

