#ifndef __ENGINE_H__
#define __ENGINE_H__
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

// C++ Headers
#include <string>

// OOMon Headers
#include "strtype"
#include "autoaction.h"


void Init_Link_Look_Table();
void Init_Nick_Change_Table();

void onTraceUser(const std::string &, const std::string &, const std::string &,
  std::string, std::string);
void onTraceUser(const std::string &, const std::string &, const std::string &,
  std::string);
void onETraceUser(const std::string & Type, const std::string & Class,
  const std::string & Nick, std::string User, const std::string & Host,
  const std::string & IP, const std::string & Gecos);
void onTraceClass();
void onClientConnect(std::string);
void onClientExit(std::string);
void onNickChange(std::string);
void onLinksNotice(std::string text);
void onTraceNotice(const std::string & text);
void onMotdNotice(const std::string & text);
void onInfoNotice(const std::string & text);
void onStatsNotice(std::string text);
void onFlooderNotice(const std::string & text);
void onSpambotNotice(const std::string & text);
void onTooManyConnNotice(const std::string & text);
void onOperNotice(std::string text);
void onOperFailNotice(const std::string & text);
void onInvalidUsername(std::string text);
void onClearTempKlines(const std::string & text);
void onClearTempDlines(const std::string & text);
void onGlineRequest(const std::string & text);

void Bot_Reject(std::string);
void CS_Clones(std::string);
void CS_Nick_Flood(std::string);

void Kill_Add_Report(std::string);

void klineClones(const bool kline, const std::string & User,
  const std::string & Host, const BotSock::Address & ip,
  const bool differentUser, const bool differentIp, const bool identd);

void onDetectDNSBLOpenProxy(const std::string & ip, const std::string & nick,
  const std::string & userhost);
void CheckProxy(const std::string &, const std::string &, const std::string &,
  const std::string &);

bool checkForSpoof(const std::string & nick, const std::string & user, 
  const std::string & host, const std::string & ip);

void status(StrList & output);


#define REASON_CLONES	"Clones are prohibited"
#define REASON_FLOODING	"Flooding is prohibited"
#define REASON_BOTS	"Bots are prohibited"
#define REASON_SPAM	"Spamming is prohibited"
#define REASON_LINKS	"Links lookers are prohibited"
#define REASON_TRACE	"TRACE flooding is prohibited"
#define REASON_MOTD	"MOTD flooding is prohibited"
#define REASON_INFO	"INFO flooding is prohibited"
#define REASON_STATS	"STATS flooding is prohibited"
#define REASON_PROXY	"Open Proxy"

#endif /* __ENGINE_H__ */

