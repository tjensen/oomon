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

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "config.h"
#include "botsock.h"
#include "watch.h"
#include "cmdparser.h"
#include "botclient.h"


class DCC : public BotSock
{
public:
  DCC(const std::string & nick = "", const std::string & userhost = "");
  DCC(DCC *listener, const std::string & nick = "",
    const std::string & userhost = "");

  bool connect(const BotSock::Address address, const BotSock::Port port,
    const std::string nick, const std::string userhost);
  bool listen(const std::string nick, const std::string userhost,
    const BotSock::Port port = 0, const int backlog = 1);

  void motd(void);

  void chat(const std::string & text);
  void send(const std::string & message,
    const UserFlags flags = UserFlags::NONE,
    const WatchSet & watches = WatchSet());
  void send(StrList & message, const UserFlags flags = UserFlags::NONE,
    const WatchSet & watches = WatchSet());

  std::string getUserhost(void) const { return UserHost; }
  std::string getNick(void) const { return Nick; }

  bool isOper(void) const
    { return (this->client->flags().has(UserFlags::OPER)); }
  bool isAuthed(void) const
    { return (this->client->flags().has(UserFlags::AUTHED)); }
  std::string getFlags(void) const;
  std::string getHandle(void) const { return this->client->handle(); }

  virtual bool onConnect(void);

protected:
  virtual bool onRead(std::string text);

  class Client : public BotClient
  {
  public:
    Client(DCC *owner) : _owner(owner), _handle(), _flags(UserFlags::NONE)
    {
      _id = ptrToStr(owner);
    }
    ~Client(void) { }

    virtual void send(const std::string & text)
    {
      this->_owner->write(text + '\n');
    }
    virtual UserFlags flags(void) const { return this->_flags; }
    virtual std::string handle(void) const { return this->_handle; }
    virtual std::string bot(void) const { return Config::GetNick(); }
    virtual std::string id(void) const { return this->_id; }

    void flags(const UserFlags f) { this->_flags = f; }
    void handle(const std::string & h) { this->_handle = h; }

  private:
    DCC *_owner;
    std::string _handle;
    std::string _id;
    UserFlags _flags;
  };
  typedef boost::shared_ptr<Client> ClientPtr;

private:
  bool echoMyChatter;
  std::string UserHost, Nick;
  WatchSet watches;
  CommandParser parser;
  ClientPtr client;

  bool parse(std::string text);

  void addCommand(const std::string & command,
    void (DCC::*)(BotClient::ptr from, const std::string & command,
    std::string parameters), const UserFlags flags,
    const int options = CommandParser::NONE);

  void cmdHelp(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdAuth(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdQuit(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdEcho(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdWatch(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdChat(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void cmdLocops(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void loadConfig(void);

  class quit : public OOMon::oomon_error
  {
  public:
    quit(const std::string & arg) : oomon_error(arg) { };
  };
};


#endif /* __DCC_H__ */

