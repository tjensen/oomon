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
#include "userentry.h"
#include "vars.h"


class AutoAction
{
  public:
    enum Type
    {
      NOTHING, KILL, KLINE, KLINE_HOST, KLINE_DOMAIN, KLINE_IP, KLINE_USERNET,
      KLINE_NET, SMART_KLINE, SMART_KLINE_HOST, SMART_KLINE_IP, DLINE_IP,
      DLINE_NET
    };

    AutoAction(const AutoAction::Type type, const unsigned int duration = 0)
      : type_(type), duration_(duration) { }

    AutoAction::Type type(void) const { return this->type_; }
    unsigned int duration(void) const { return this->duration_; }

    static std::string get(const AutoAction * value);
    static std::string set(AutoAction * value, const std::string & value);

    static ::Setting Setting(AutoAction & action);

  private:
    AutoAction::Type type_;
    unsigned int duration_;
};


void doAction(const std::string & nick, const std::string & userhost,
    BotSock::Address ip, const AutoAction & action, const std::string & reason,
    bool suggestKlineAfterKill);
void doAction(const UserEntryPtr & user, const AutoAction & action,
    const std::string & reason, bool suggestKlineAfterKill);


#endif /* __AUTOACTION_H__ */

