#ifndef __TRAP_H__
#define __TRAP_H__
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
#include <fstream>
#include <string>
#include <map>
#include <ctime>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>

// OOMon Headers
#include "strtype"
#include "botsock.h"
#include "pattern.h"
#include "userentry.h"


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

  bool matches(const UserEntryPtr user, const std::string & version,
    const std::string & privmsg, const std::string & notice) const;
  bool operator==(const Trap & other) const;
  bool operator==(const std::string & other) const;
  void doAction(const UserEntryPtr user) const;

  void updateStats(void);

  TrapAction getAction(void) const { return this->action_; }
  long getTimeout(void) const { return this->timeout_; }
  std::string getPattern(void) const;
  std::string getReason(void) const { return this->reason_; }
  std::string getString(bool showCount = false, bool showTime = false) const;
  std::time_t getLastMatch(void) const { return this->lastMatch_; };
  unsigned long getMatchCount(void) const { return this->matchCount_; };

private:
  typedef boost::shared_ptr<Pattern> PatternPtr;
  typedef boost::shared_ptr<RegExPattern> RegExPatternPtr;
  TrapAction	action_;
  long		timeout_;	// For K-Lines only
  PatternPtr	nick_;
  PatternPtr	userhost_;
  PatternPtr	gecos_;
  PatternPtr	version_;
  PatternPtr	privmsg_;
  PatternPtr	notice_;
  RegExPatternPtr	rePattern_;
  std::string	reason_;	// For Kills, K-Lines, and D-Lines only
  std::time_t	lastMatch_;
  unsigned long	matchCount_;

  static void split(const std::string & pattern, std::string & nick,
    std::string & userhost);
  static bool parsePattern(std::string & pattern, PatternPtr & nick,
    PatternPtr & userhost, PatternPtr & gecos, PatternPtr & version,
    PatternPtr & privmsg, PatternPtr & notice);
};


class TrapList
{
public:
  static void cmd(class BotClient * client, std::string line);
  static bool remove(const unsigned int key);
  static bool remove(const std::string & pattern);
  static void clear(void) { TrapList::traps.clear(); };

  static void match(const UserEntryPtr user, const std::string & version,
    const std::string & privmsg, const std::string & notice);
  static void match(const UserEntryPtr user);
  static void matchCtcpVersion(const UserEntryPtr user,
    const std::string & version);
  static void matchPrivmsg(const UserEntryPtr user,
    const std::string & privmsg);
  static void matchNotice(const UserEntryPtr user, const std::string & notice);

  static void list(class BotClient * client, bool showCounts, bool showTimes);

  static void save(std::ofstream & file);

  static TrapAction actionType(const std::string & text);
  static std::string actionString(const TrapAction & action);

private:
  typedef std::multimap<unsigned int, Trap> TrapMap;
  static TrapMap traps;

  static Trap add(const unsigned int key, const TrapAction action,
    const long timeout, const std::string & line);
  static unsigned int getMaxKey(void);
};

#endif /* __TRAP_H__ */

