#ifndef __IRC_H__
#define __IRC_H__
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

// Boost C++ Headers
#include <boost/function.hpp>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "strtype"
#include "botsock.h"
#include "klines.h"
#include "pattern.h"


enum IRCCommand
{
  IRC_UNKNOWN, IRC_PING, IRC_NICK, IRC_JOIN, IRC_PART, IRC_KICK, IRC_INVITE,
  IRC_NOTICE, IRC_PRIVMSG, IRC_WALLOPS, IRC_ERROR
};

enum CaseMapping
{
  CASEMAP_RFC1459, CASEMAP_STRICT_RFC1459, CASEMAP_ASCII
};


class IRC
{
public:
  IRC();

  typedef boost::function<bool(std::string)> ParserFunction;

  bool process(const fd_set & readset, const fd_set & writeset);
  int write(const std::string & text);

  void quit(const std::string & Message = "Normal termination");
  void notice(const std::string &, const std::string &);
  void notice(const std::string &, const StrList &);
  void msg(const std::string &, const std::string &);
  void ctcp(const std::string &, const std::string &);
  void ctcpReply(const std::string &, const std::string &);
  void isOn(const std::string &);
  void kline(const std::string &, const unsigned int, const std::string &,
    const std::string &);
  void unkline(const std::string &, const std::string &);
  void dline(const std::string &, const unsigned int, const std::string &,
    const std::string &);
  void undline(const std::string &, const std::string &);
  void kill(const std::string &, const std::string &, const std::string &);
  void trace(const std::string & Target = "");
  void statsL(const std::string &);
  void retrace(const std::string &);
  void join(const std::string & channel, const std::string & key);
  void join(const std::string & channel);
  void part(const std::string & channel);
  void knock(const std::string & channel);
  void whois(const std::string & nick);
  void op(const std::string &, const std::string &);
  void umode(const std::string &);
  void locops(const std::string &);

  void reloadKlines(void);
  void reloadKlines(const std::string &);
  void reloadDlines(void);
  void reloadDlines(const std::string &);
  void findK(class BotClient * client, const Pattern *,
    const bool count = false, const bool searchPerms = true,
    const bool searchTemps = true, const bool searchReason = false) const;
  int findAndRemoveK(const std::string & from, const Pattern *userhost,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false);
  void findD(class BotClient * client, const Pattern *,
    const bool count = false, const bool searchPerms = true,
    const bool searchTemps = true, const bool searchReason = false) const;
  int findAndRemoveD(const std::string & from, const Pattern *userhost,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false);
  void subSpamTrap(const bool sub);

  std::string getServerName(void) const { return serverName; };

  void checkUserDelta(void);

  void status(class BotClient * client) const;

  bool onConnect(void);

  char upCase(const char c) const;
  std::string upCase(const std::string & text) const;
  char downCase(const char c) const;
  std::string downCase(const std::string & text) const;
  bool same(const std::string & text1, const std::string & text2) const;

  void onServerNotice(const std::string & text);

  bool opered(void) const { return this->amIAnOper; }

  bool isConnected(void) const { return this->sock_.isConnected(); }
  bool isConnecting(void) const { return this->sock_.isConnecting(); }
  std::time_t getIdle(void) const { return this->sock_.getIdle(); }
  std::time_t getWriteIdle(void) const { return this->sock_.getWriteIdle(); }
  std::time_t getTimeout(void) const { return this->sock_.getTimeout(); }
  void setTimeout(const std::time_t value) { this->sock_.setTimeout(value); }
  std::string getUptime(void) const { return this->sock_.getUptime(); }
  BotSock::Address getLocalAddress(void) const
  {
    return this->sock_.getLocalAddress();
  }
  BotSock::Address getRemoteAddress(void) const
  {
    return this->sock_.getRemoteAddress();
  }
  void bindTo(const std::string & name) { this->sock_.bindTo(name); }
  bool connect(const std::string & address, const BotSock::Port port)
  {
    return this->sock_.connect(address, port);
  }
  void setFD(fd_set & r, fd_set & w) const { this->sock_.setFD(r, w); }
  void reset(void) { this->sock_.reset(); }

  static bool validNick(const std::string & nick);

  static bool trackTempKlines(void) { return IRC::trackTempKlines_; }
  static bool trackTempDlines(void) { return IRC::trackTempDlines_; }

  static void init(void);

protected:
  bool onRead(std::string text);

  void addServerNoticeParser(const std::string & pattern,
    const ParserFunction func);

private:
  class Parser
  {
  public:
    Parser(const std::string & pattern, const ParserFunction func);
    bool match(std::string text) const;
  private:
    PatternPtr pattern_;
    ParserFunction func_;
  };
  typedef std::vector<Parser> ParserVector;

  BotSock sock_;
  ParserVector serverNotices;
  bool amIAnOper;
  bool gettingTrace;
  bool gettingKlines;
  bool gettingTempKlines;
  bool gettingDlines;
  bool supportETrace;
  bool supportKnock;
  CaseMapping caseMapping;
  std::string myNick;
  std::string serverName;
  KlineList klines;
  KlineList dlines;
  std::time_t lastUserDeltaCheck;
  std::time_t lastCtcpVersionTimeoutCheck;

  static bool operNickInReason_;
  static bool relayMsgsToLocops_;
  static int serverTimeout_;
  static bool trackPermDlines_;
  static bool trackPermKlines_;
  static bool trackTempDlines_;
  static bool trackTempKlines_;
  static std::string umode_;
  static int userCountDeltaMax_;

  void onCtcp(const std::string & from, const std::string & userhost,
    const std::string & to, std::string text);
  void onCtcpReply(const std::string & from, const std::string & userhost,
    const std::string & to, std::string text);
  void onPrivmsg(const std::string & from, const std::string & userhost,
    const std::string & to, std::string text);
  void onNotice(const std::string & from, const std::string & userhost,
    const std::string & to, std::string text);

  static std::string getServerTimeout(void);
  static std::string setServerTimeout(const std::string & newValue);

  static IRCCommand getCommand(const std::string & text);
};


extern IRC server;


#endif /* __IRC_H__ */

