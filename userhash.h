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
#include "userentry.h"


#define HASHTABLESIZE 3001


class UserHash
{
public:
  enum ListAction { LIST_VIEW, LIST_COUNT, LIST_KILL };

  UserHash() { this->userCount = this->previousCount = 0; };
  virtual ~UserHash() { this->clear(); };

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

  int listUsers(class BotClient * client, const PatternPtr userhost,
    std::string className, const ListAction action = LIST_VIEW,
    const std::string & from = "", const std::string & reason = "") const;
  int listNicks(class BotClient * client, const PatternPtr nick,
    std::string className, const ListAction action = LIST_VIEW,
    const std::string & from = "", const std::string & reason = "") const;
  int listGecos(class BotClient * client, const PatternPtr gecos,
    std::string className, const bool count = false) const;

  void reportClasses(class BotClient * client, const std::string & className);
  void reportSeedrand(class BotClient * client, const PatternPtr mask,
    const int threshold, const bool count = false) const;
  void reportDomains(class BotClient * client, const int num);
  void reportNets(class BotClient * client, const int num);
  void reportClones(class BotClient * client);
  void reportMulti(class BotClient * client, const int minimum);
  void reportUMulti(class BotClient * client, const int minimum);
  void reportHMulti(class BotClient * client, const int minimum);
  void reportVMulti(class BotClient * client, const int minimum);

  void status(class BotClient * client);

  int getUserCountDelta(void);
  void resetUserCountDelta(void);

private:
  class HashRec
  {
  public:
    UserEntryPtr info;
    HashRec * collision;
  };

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

  class SortEntry
  {
  public:
    UserEntryPtr rec;
    int count;
  };

  static int hashFunc(const BotSock::Address & key);
  static int hashFunc(const std::string & key);

  static void addToHash(HashRec *table[], const BotSock::Address & key,
    UserEntryPtr item);
  static void addToHash(HashRec *table[], const std::string & key,
    UserEntryPtr item);

  static bool removeFromHashEntry(HashRec * table[], const int index,
    const std::string & host, const std::string & user,
    const std::string & nick);
  static bool removeFromHash(HashRec * table[], const std::string & key,
    const std::string & host, const std::string & user,
    const std::string & nick);
  static bool removeFromHash(HashRec * table[], const BotSock::Address & key,
    const std::string & host, const std::string & user,
    const std::string & nick);

  static void clearHash(HashRec * table[]);

  HashRec *hosttable[HASHTABLESIZE];
  HashRec *domaintable[HASHTABLESIZE];
  HashRec *usertable[HASHTABLESIZE];
  HashRec *iptable[HASHTABLESIZE];
  std::string maskNick, maskRealHost, maskFakeHost;
  int userCount, previousCount;
};


extern UserHash users;


#endif /* __USERHASH_H__ */

