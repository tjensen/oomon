#ifndef __TRAP_H__
#define __TRAP_H__
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

// Std C++ Headers
#include <fstream>
#include <string>
#include <list>

// OOMon Headers
#include "strtype"
#include "botsock.h"
#include "pattern.h"


enum TrapAction
{
  TRAP_ECHO, TRAP_KILL, TRAP_KLINE, TRAP_KLINE_HOST, TRAP_KLINE_DOMAIN,
  TRAP_KLINE_IP, TRAP_KLINE_USERNET, TRAP_KLINE_NET, TRAP_DLINE_IP,
  TRAP_DLINE_NET
};


class Trap
{
public:
  Trap(const TrapAction action, const long timeout, const std::string & line);
  Trap(const Trap & copy);
  virtual ~Trap(void);

  bool matches(const std::string & nick, const std::string & userhost,
    const std::string & ip, const std::string & gecos) const;
  bool operator==(const Trap & other) const;
  bool operator==(const std::string & other) const;
  void doAction(const std::string & nick, const std::string & userhost,
    const std::string & ip, const std::string & gecos) const;

  TrapAction getAction() const { return this->_action; }
  long getTimeout() const { return this->_timeout; }
  std::string getPattern() const;
  std::string getReason() const { return this->_reason; }
  std::string getString() const;

private:
  TrapAction	_action;
  long		_timeout;	// For K-Lines only
  Pattern	*_nick;
  Pattern	*_userhost;
  Pattern	*_gecos;
  RegExPattern	*_rePattern;
  std::string	_reason;		// For Kills, K-Lines, and D-Lines only

  static void split(const std::string & pattern, std::string & nick,
    std::string & userhost);
  static bool parsePattern(std::string & pattern, Pattern* & nick,
    Pattern* & userhost, Pattern* & gecos);
};


class TrapList
{
public:
  static void cmd(StrList & output, std::string line,
    const std::string & handle = "");
  static bool remove(const Trap & pattern);
  static bool remove(const std::string & pattern);
  static void clear(void) { TrapList::traps.clear(); };

  static void match(const std::string & nick, const std::string & userhost,
    const std::string & ip, const std::string & gecos);
  static void match(const std::string & nick, const std::string & userhost,
    const BotSock::Address & ip, const std::string & gecos)
  {
    TrapList::match(nick, userhost, BotSock::inet_ntoa(ip), gecos);
  };

  static void list(StrList & output);

  static void save(std::ofstream & file);

  static TrapAction actionType(const std::string & text);
  static std::string actionString(const TrapAction & action);

private:
  static std::list<Trap> traps;

  static Trap add(const TrapAction action, const long timeout,
    const std::string & line);
};

#endif /* __TRAP_H__ */

