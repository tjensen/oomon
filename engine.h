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


void initFloodTables(void);

void onTraceUser(const std::string &, const std::string &, const std::string &,
  std::string, std::string);
void onTraceUser(const std::string &, const std::string &, const std::string &,
  std::string);
void onETraceUser(const std::string & Type, const std::string & Class,
  const std::string & Nick, std::string User, const std::string & Host,
  const std::string & IP, const std::string & Gecos);
void onTraceClass();
bool onClientConnect(std::string text);
bool onClientExit(std::string text);
bool onNickChange(std::string text);
bool onLinksNotice(std::string text);
bool onTraceNotice(std::string text);
bool onMotdNotice(std::string text);
bool onInfoNotice(std::string text);
bool onStatsNotice(std::string text);
bool onFlooderNotice(const std::string & text);
bool onSpambotNotice(const std::string & text);
bool onTooManyConnNotice(const std::string & text);
bool onOperNotice(std::string text);
bool onOperFailNotice(const std::string & text);
bool onInvalidUsername(std::string text);
bool onClearTempKlines(std::string text);
bool onClearTempDlines(std::string text);
bool onGlineRequest(std::string text);
bool onLineActive(std::string text);
bool onJupeJoinNotice(const std::string & text);
bool onMaskHostNotice(std::string text);

bool onBotReject(std::string text);
bool onCsClones(std::string text);
bool onCsNickFlood(std::string text);

bool onKillNotice(std::string text);

void klineClones(const bool kline, const std::string & rate,
  const std::string & User, const std::string & Host,
  const BotSock::Address & ip, const std::string & userClass,
  const bool differentUser, const bool differentIp,
  const bool identd);

void checkProxy(const UserEntryPtr user);

bool checkForSpoof(const std::string & nick, const std::string & user, 
  const std::string & host, const std::string & ip,
  const std::string & userClass);

void status(class BotClient * client);

#endif /* __ENGINE_H__ */

