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

#ifndef __USERENTRY_H__
#define __USERENTRY_H__


// Std C++ Headers
#include <string>
#include <ctime>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>

// OOMon Headers
#include "botsock.h"


class UserEntry
{
public:
  UserEntry(const std::string & aNick, const std::string & aUser,
    const std::string & aHost, const std::string & aFakeHost,
    const std::string & aUserClass, const std::string & aGecos,
    const BotSock::Address anIp, const std::time_t aConnectTime,
    const bool oper);
  virtual ~UserEntry(void);

  void setNick(const std::string & aNick);
  void setOper(const bool oper) { this->isOper = oper; };
  void setReportTime(const std::time_t t) { this->reportTime = t; };
  void version(void);
  void hasVersion(const std::string & version);

  bool matches(const std::string & lowercaseNick) const;
  bool matches(const std::string & lowercaseNick,
    const std::string & lowercaseUser, const std::string & lowercaseHost) const;

  bool same(const std::string & nick, const std::string & user,
    const std::string & host) const;

  std::time_t checkVersionTimeout(const std::time_t now,
      const std::time_t timeout);

  std::string getNick(void) const { return this->nick; };
  std::string getUser(void) const { return this->user; };
  std::string getHost(void) const { return this->host; };
  std::string getFakeHost(void) const { return this->fakeHost; };
  std::string getDomain(void) const { return this->domain; };
  std::string getClass(void) const { return this->userClass; };
  std::string getGecos(void) const { return this->gecos; };
  BotSock::Address getIP(void) const { return this->ip; };
  std::string getTextIP(void) const
  {
    return BotSock::inet_ntoa(this->ip);
  }
  bool getOper(void) const { return this->isOper; };
  int getScore(void) const { return this->randScore; };
  std::string getUserHost(void) const
  {
    std::string result(this->user);
    result += '@';
    result += this->host;
    return result;
  }
  std::string getUserIP(void) const
  {
    std::string result(this->user);
    result += '@';
    result += BotSock::inet_ntoa(this->ip);
    return result;
  }
  std::string getNickUserHost(void) const
  {
    std::string result(this->nick);
    result += '!';
    result += this->user;
    result += '@';
    result += this->host;
    return result;
  }
  std::string getNickUserIP(void) const
  {
    std::string result(this->nick);
    result += '!';
    result += this->user;
    result += '@';
    result += BotSock::inet_ntoa(this->ip);
    return result;
  }
  std::string getNickUserHostGecos(void) const
  {
    std::string result(this->nick);
    result += '!';
    result += this->user;
    result += '@';
    result += this->host;
    result += '#';
    result += this->gecos;
    return result;
  }
  std::string getNickUserIPGecos(void) const
  {
    std::string result(this->nick);
    result += '!';
    result += this->user;
    result += '@';
    result += BotSock::inet_ntoa(this->ip);
    result += '#';
    result += this->gecos;
    return result;
  }
  std::time_t getConnectTime(void) const { return this->connectTime; };
  std::time_t getReportTime(void) const { return this->reportTime; };

  std::string output(const std::string & format) const;

private:
  std::string nick;
  std::string user;
  std::string host;
  std::string fakeHost;
  std::string domain;
  std::string userClass;
  std::string gecos;
  BotSock::Address ip;
  std::time_t connectTime;
  std::time_t reportTime;
  std::time_t versioned;
  bool isOper;
  int randScore;
};


typedef boost::shared_ptr<UserEntry> UserEntryPtr;


#endif /* __USERENTRY_H__ */

