#ifndef __USERHASH_H__
#define __USERHASH_H__
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
#include "botsock.h"
#include "pattern.h"
#include "filter.h"
#include "userentry.h"
#include "autoaction.h"
#include "action.h"


#define HASHTABLESIZE 3001


class UserHash
{
public:
  static void init(void);

  enum ListAction { LIST_VIEW, LIST_COUNT, LIST_KILL };

  UserHash(void);
  virtual ~UserHash(void);

  void clear();

  void add(const std::string & nick, const std::string & userhost,
    const std::string & ip, bool fromTrace, bool isOper,
    const std::string & userClass, const std::string & Gecos = "");

  BotSock::Address getIP(std::string nick, const std::string & userhost) const;
  bool isOper(std::string nick, const std::string & userhost) const;

  void setMaskHost(const std::string & nick, const std::string & realHost,
    const std::string & fakeHost);

  void updateNick(const std::string & oldNick, const std::string & userhost,
    const std::string & newNick);
  void updateOper(const std::string & nick, const std::string & userhost,
    bool isOper);

  void onVersionReply(const std::string & nick, const std::string & userhost,
    const std::string & version);
  void onPrivmsg(const std::string & nick, const std::string & userhost,
    const std::string & privmsg);
  void onNotice(const std::string & nick, const std::string & userhost,
    const std::string & notice);

  void checkVersionTimeout(void);

  void remove(const std::string & nick, const std::string & userhost,
    const BotSock::Address & ip);

  bool have(std::string nick) const;

  UserEntryPtr findUser(const std::string & nick) const;
  UserEntryPtr findUser(const std::string & nick,
    const std::string & userhost) const;

  void checkHostClones(const std::string & host);
  void checkIpClones(const BotSock::Address & ip);

  int findUsers(class BotClient * client, const Filter & filter,
    ActionPtr action) const;

  void reportClasses(class BotClient * client, const std::string & className)
    const;
  void reportSeedrand(class BotClient * client, const PatternPtr mask,
    const int threshold, const bool count = false) const;
  void reportDomains(class BotClient * client, const int minimum) const;
  void reportNets(class BotClient * client, const int minimum) const;
  void reportClones(class BotClient * client) const;
  void reportVClones(class BotClient * client) const;
  void reportMulti(class BotClient * client, const int minimum) const;
  void reportUMulti(class BotClient * client, const int minimum) const;
  void reportHMulti(class BotClient * client, const int minimum) const;
  void reportVMulti(class BotClient * client, const int minimum) const;

  void status(class BotClient * client);

  int getUserCountDelta(void);
  void resetUserCountDelta(void);

private:
  typedef std::list<UserEntryPtr> UserEntryList;
  typedef std::vector<UserEntryList> UserEntryTable;

  class ScoreNode
  {
  public:
    ScoreNode(UserEntryPtr info, int score) : info_(info),
      score_(score) { }
    UserEntryPtr getInfo() const { return this->info_; };
    int getScore() const { return this->score_; };
    bool operator < (const ScoreNode & compare) const
      { return (this->score_ < compare.score_); };
  private:
    UserEntryPtr info_;
    int score_;
  };

  static unsigned int hashFunc(const BotSock::Address & key);
  static unsigned int hashFunc(const std::string & key);

  static void addToHash(UserEntryTable & table, const BotSock::Address & key,
    const UserEntryPtr & item);
  static void addToHash(UserEntryTable & table, const std::string & key,
    const UserEntryPtr & item);

  static bool removeFromHashEntry(UserEntryList & list,
    const std::string & host, const std::string & user,
    const std::string & nick);
  static bool removeFromHash(UserEntryTable & table, const std::string & key,
    const std::string & host, const std::string & user,
    const std::string & nick);
  static bool removeFromHash(UserEntryTable & table,
    const BotSock::Address & key, const std::string & host,
    const std::string & user, const std::string & nick);

  static void clearHash(UserEntryTable & table);

#ifdef USERHASH_DEBUG
  static void debugStatus(BotClient * client, const UserEntryTable & table,
      const std::string & label, const int userCount);
#endif /* USERHASH_DEBUG */

  UserEntryTable hosttable;
  UserEntryTable domaintable;
  UserEntryTable usertable;
  UserEntryTable iptable;
  std::string maskNick, maskRealHost, maskFakeHost;
  int userCount, previousCount;

  static bool brokenHostnameMunging;
  static int cloneMaxTime;
  static int cloneMinCount;
  static std::string cloneReportFormat;
  static int cloneReportInterval;
  static bool ctcpversionEnable;
  static int ctcpversionTimeout;
  static AutoAction ctcpversionTimeoutAction;
  static std::string ctcpversionTimeoutReason;
  static int multiMin;
  static bool operInMulti;
  static AutoAction seedrandAction;
  static std::string seedrandFormat;
  static std::string seedrandReason;
  static int seedrandReportMin;
  static bool trapConnects;
  static bool trapCtcpVersions;
  static bool trapNickChanges;
  static bool trapNotices;
  static bool trapPrivmsgs;
};


extern UserHash users;


#endif /* __USERHASH_H__ */

