#ifndef __AUTOACTION_H__
#define __AUTOACTION_H__
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

// OOMon Headers
#include "botsock.h"


enum AutoAction
{
  ACTION_NOTHING, ACTION_KILL, ACTION_KLINE, ACTION_KLINE_HOST,
  ACTION_KLINE_DOMAIN, ACTION_KLINE_IP, ACTION_KLINE_USERNET, ACTION_KLINE_NET,
  ACTION_SMART_KLINE, ACTION_SMART_KLINE_HOST, ACTION_SMART_KLINE_IP,
  ACTION_DLINE_IP, ACTION_DLINE_NET
};


void
doAction(const std::string & nick, const std::string & userhost,
  BotSock::Address ip, const AutoAction & action, int duration,
  const std::string & reason, bool suggestKlineAfterKill);


#endif /* __AUTOACTION_H__ */

