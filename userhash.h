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

// Std C Headers
#include <time.h>

// OOMon Headers
#include "strtype"
#include "botsock.h"
#include "pattern.h"


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

  void updateNick(const std::string & oldNick, const std::string & userhost,
    const std::string & newNick);
  void updateOper(const std::string & nick, const std::string & userhost,
    bool isOper);

  void remove(const std::string & nick, const std::string & userhost,
    const BotSock::Address & ip);

  bool have(std::string nick) const;

  void checkHostClones(const std::string & host);
  void checkIpClones(const BotSock::Address & ip);

  int listUsers(class BotClient * client, const Pattern *userhost,
    std::string className, const ListAction action = LIST_VIEW,
    const std::string & from = "", const std::string & reason = "") const;
  int listNicks(class BotClient * client, const Pattern *nick,
    std::string className, const ListAction action = LIST_VIEW,
    const std::string & from = "", const std::string & reason = "") const;
  int listGecos(class BotClient * client, const Pattern *gecos,
    std::string className, const bool count = false) const;

  void reportClasses(class BotClient * client, const std::string & className);
  void reportSeedrand(class BotClient * client, const Pattern *mask,
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
  class UserEntry
  {
  public:
    UserEntry(const std::string & aNick, const std::string & aUser,
      const std::string & aHost, const std::string & aUserClass,
      const std::string & aGecos, const BotSock::Address anIp,
      const time_t aConnectTime, const bool oper);
    virtual ~UserEntry() {};

    void setNick(const std::string & aNick);
    void setOper(const bool oper) { this->isOper = oper; };
    void setReportTime(const time_t t) { this->reportTime = t; };
    void incLinkCount() { ++this->linkCount; };
    void decLinkCount() { --this->linkCount; };

    std::string getNick() const { return this->nick; };
    std::string getUser() const { return this->user; };
    std::string getHost() const { return this->host; };
    std::string getDomain() const { return this->domain; };
    std::string getClass() const { return this->userClass; };
    std::string getGecos() const { return this->gecos; };
    BotSock::Address getIp() const { return this->ip; };
    bool getOper() const { return this->isOper; };
    int getScore() const { return this->randScore; };
    std::string getUserHost() const { return this->user + '@' + this->host; };
    std::string getUserIP() const
      { return this->user + '@' + BotSock::inet_ntoa(this->ip); };
    time_t getConnectTime() const { return this->connectTime; };
    time_t getReportTime() const { return this->reportTime; };
    time_t getLinkCount() const { return this->linkCount; };

    std::string output(const std::string & format) const;

  private:
    std::string nick;
    std::string user;
    std::string host;
    std::string domain;
    std::string userClass;
    std::string gecos;
    BotSock::Address ip;
    time_t connectTime;
    time_t reportTime;
    bool isOper;
    int randScore;
    int linkCount;
  };

  class HashRec
  {
  public:
    UserEntry *info;
    HashRec *collision;
  };

  class ScoreNode
  {
  public:
    ScoreNode(UserEntry *info, int score)
    {
      this->info = info;
      this->score = score;
    };
    UserEntry *getInfo() const { return this->info; };
    int getScore() const { return this->score; };
    bool operator < (const ScoreNode & compare) const
      { return (this->score < compare.score); };
  private:
    UserEntry *info;
    int score;
  };

  class SortEntry
  {
  public:
    UserEntry *rec;
    int count;
  };

  static int hashFunc(const BotSock::Address & key);
  static int hashFunc(const std::string & key);

  static void addToHash(HashRec *table[], const BotSock::Address & key,
    UserEntry *item);
  static void addToHash(HashRec *table[], const std::string & key,
    UserEntry *item);

  static bool removeFromHashEntry(HashRec *table[], const int index,
    const std::string & host, const std::string & user,
    const std::string & nick);
  static bool removeFromHash(HashRec *table[], const std::string & key,
    const std::string & host, const std::string & user,
    const std::string & nick);
  static bool removeFromHash(HashRec *table[], const BotSock::Address & key,
    const std::string & host, const std::string & user,
    const std::string & nick);

  static void clearHash(HashRec *table[]);

  HashRec *hosttable[HASHTABLESIZE];
  HashRec *domaintable[HASHTABLESIZE];
  HashRec *usertable[HASHTABLESIZE];
  HashRec *iptable[HASHTABLESIZE];
  int userCount, previousCount;
};


extern UserHash users;


#endif /* __USERHASH_H__ */

