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

// OOMon Headers
#include "botsock.h"


class UserEntry
{
public:
  UserEntry(const std::string & aNick, const std::string & aUser,
    const std::string & aHost, const std::string & aUserClass,
    const std::string & aGecos, const BotSock::Address anIp,
    const std::time_t aConnectTime, const bool oper);
  virtual ~UserEntry() {};

  void setNick(const std::string & aNick);
  void setOper(const bool oper) { this->isOper = oper; };
  void setReportTime(const std::time_t t) { this->reportTime = t; };
  void incLinkCount() { ++this->linkCount; };
  void decLinkCount() { --this->linkCount; };
  void version(void);
  void hasVersion(const std::string & version);

  void checkVersionTimeout(const std::time_t now, const std::time_t timeout);

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
  std::time_t getConnectTime() const { return this->connectTime; };
  std::time_t getReportTime() const { return this->reportTime; };
  std::time_t getLinkCount() const { return this->linkCount; };

  std::string output(const std::string & format) const;

private:
  std::string nick;
  std::string user;
  std::string host;
  std::string domain;
  std::string userClass;
  std::string gecos;
  BotSock::Address ip;
  std::time_t connectTime;
  std::time_t reportTime;
  std::time_t versioned;
  bool isOper;
  int randScore;
  int linkCount;
};


#endif /* __USERENTRY_H__ */

