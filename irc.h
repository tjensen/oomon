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

// Boost C++ Headers
#include <boost/function.hpp>

// Std C Headers
#include <time.h>

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


class IRC : public BotSock
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
  void kline(const std::string &, const int, const std::string &,
    const std::string &);
  void unkline(const std::string &, const std::string &);
  void dline(const std::string &, const int, const std::string &,
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
  void findK(StrList &, const Pattern *, const bool count = false,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false) const;
  int findAndRemoveK(const std::string & from, const Pattern *userhost,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false);
  void findD(StrList &, const Pattern *, const bool count = false,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false) const;
  int findAndRemoveD(const std::string & from, const Pattern *userhost,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false);
  void subSpamTrap(const bool sub);

  std::string getServerName(void) const { return serverName; };
  time_t getWriteIdle(void) const { return time(NULL) - this->lastWrite; };

  void checkUserDelta(void);

  void status(StrList & output) const;

  virtual bool onConnect();

  char upCase(const char c) const;
  std::string upCase(const std::string & text) const;
  char downCase(const char c) const;
  std::string downCase(const std::string & text) const;
  bool same(const std::string & text1, const std::string & text2) const;

  void onServerNotice(const std::string & text);

protected:
  virtual bool onRead(std::string text);

  void addServerNoticeParser(const std::string & pattern,
    const ParserFunction func);

private:
  class Parser
  {
  public:
    Parser(const std::string & pattern, const ParserFunction func)
      : _pattern(pattern), _func(func) { }
    bool match(std::string text) const;
  private:
    ClusterPattern _pattern;
    ParserFunction _func;
  };
  typedef std::vector<Parser> ParserVector;

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
  time_t lastUserDeltaCheck;
  time_t lastWrite;

  void onCtcp(const std::string & From, const std::string & UserHost,
    const std::string & To, std::string Text);
  void onPrivmsg(const std::string &, const std::string &,
    const std::string &, std::string);
  void onNotice(const std::string &, const std::string &,
    const std::string &, std::string);

  static IRCCommand getCommand(const std::string & text);
};


extern IRC server;


#endif /* __IRC_H__ */

