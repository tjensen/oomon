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

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// OOMon Headers
#include "autoaction.h"
#include "userhash.h"
#include "irc.h"
#include "util.h"
#include "vars.h"
#include "main.h"
#include "botsock.h"
#include "botclient.h"


static void
doKill(const std::string & nick, const std::string & reason)
{
  if (vars[VAR_AUTO_PILOT]->getBool())
  {
    server.kill("Auto-Kill", nick, reason);
  }
  else
  {
    ::SendAll(".kill " + nick + " " + reason, UserFlags::OPER);
  }
}


static void
doKline(const std::string & mask, int duration, const std::string & reason)
{
  if (vars[VAR_AUTO_PILOT]->getBool())
  {
    server.kline("Auto-Kline", duration, mask, reason);
  }
  else
  {
    if (duration > 0)
    {
      ::SendAll(".kline " + boost::lexical_cast<std::string>(duration) + " " +
	mask + " " + reason, UserFlags(UserFlags::OPER, UserFlags::KLINE));
    }
    else
    {
      ::SendAll(".kline " + mask + " " + reason,
	UserFlags(UserFlags::OPER, UserFlags::KLINE));
    }
  }
}


static void
doDline(const std::string & mask, const int duration,
  const std::string & reason)
{
  if (vars[VAR_AUTO_PILOT]->getBool())
  {
    server.dline("Auto-Dline", duration, mask, reason);
  }
  else
  {
    if (duration > 0)
    {
      ::SendAll(".dline " + boost::lexical_cast<std::string>(duration) + " " +
        mask + " " + reason, UserFlags(UserFlags::OPER, UserFlags::DLINE));
    }
    else
    {
      ::SendAll(".dline " + mask + " " + reason,
	UserFlags(UserFlags::OPER, UserFlags::DLINE));
    }
  }
}


void
doAction(const std::string & nick, const std::string & userhost,
  BotSock::Address ip, const AutoAction & action, int duration,
  const std::string & reason, bool suggestKlineAfterKill)
{
  std::string::size_type at = userhost.find('@');
  if (std::string::npos == at)
  {
    // user@host is invalid format! just give up.
    return;
  }

  const std::string user = userhost.substr(0, at);
  const std::string host = userhost.substr(at + 1);
  std::string ident = user;
  std::string domain = getDomain(host, true);

  if ((ident.length() > 1) && (ident[0] == '~'))
  {
    ident = ident.substr(1);
  }
  else if (ident.length() > 8)
  {
    ident = ident.substr(ident.length() - 8);
  }

  if ((domain.length() > 1) && (domain[domain.length() - 1] == '.'))
  {
    domain = domain + "*";
  }
  else
  {
    domain = "*" + domain;
  }

  if (ip == INADDR_NONE)
  {
    if (isNumericIPv4(host))
    {
      ip = BotSock::inet_addr(host);
    }
    else
    {
      ip = users.getIP(nick, userhost);
    }
  }

  switch (action)
  {
  case ACTION_NOTHING:
    // Do nothing!
    break;

  case ACTION_KILL:
    doKill(nick, reason);
    if (suggestKlineAfterKill && vars[VAR_AUTO_PILOT]->getBool())
    {
      std::string mask("*" + ident + "@" + domain);
      ::SendAll(".kline " + mask + " " + reason,
	UserFlags(UserFlags::OPER, UserFlags::KLINE));
    }
    break;

  case ACTION_KLINE:
    doKline("*" + ident + "@" + domain, duration, reason);
    break;

  case ACTION_KLINE_HOST:
    doKline("*@" + host, duration, reason);
    break;

  case ACTION_KLINE_DOMAIN:
    doKline("*@" + domain, duration, reason);
    break;

  case ACTION_KLINE_IP:
    if (ip != INADDR_NONE)
    {
      doKline("*@" + BotSock::inet_ntoa(ip), duration, reason);
    }
    else
    {
      doKline("*@" + host, duration, reason);
    }
    break;

  case ACTION_KLINE_USERNET:
    if (ip != INADDR_NONE)
    {
      doKline("*" + ident + "@" + classCMask(BotSock::inet_ntoa(ip)), duration,
	reason);
    }
    else
    {
      doKline("*" + ident + "@" + domain, duration, reason);
    }
    break;

  case ACTION_KLINE_NET:
    if (ip != INADDR_NONE)
    {
      doKline("*@" + classCMask(BotSock::inet_ntoa(ip)), duration, reason);
    }
    else
    {
      doKline("*@" + domain, duration, reason);
    }
    break;

  case ACTION_SMART_KLINE:
    if (isDynamic(user, host))
    {
      doKline("*@" + host, duration, reason);
    }
    else if (INADDR_NONE == ip)
    {
      doKline("*" + ident + "@" + domain, duration, reason);
    }
    else
    {
      doKline("*" + ident + "@" + classCMask(BotSock::inet_ntoa(ip)),
        duration, reason);
    }
    break;

  case ACTION_SMART_KLINE_HOST:
    if (isDynamic(user, host))
    {
      doKline("*@" + host, duration, reason);
    }
    else
    {
      doKline("*" + ident + "@" + domain, duration, reason);
    }
    break;

  case ACTION_SMART_KLINE_IP:
    if (ip != INADDR_NONE)
    {
      if (isDynamic(user, host))
      {
        doKline("*@" + BotSock::inet_ntoa(ip), duration, reason);
      }
      else
      {
        doKline("*" + ident + "@" + classCMask(BotSock::inet_ntoa(ip)),
	  duration, reason);
      }
    }
    else
    {
      if (isDynamic(user, host))
      {
        doKline("*@" + host, duration, reason);
      }
      else
      {
        doKline("*" + ident + "@" + domain, duration, reason);
      }
    }
    break;

  case ACTION_DLINE_IP:
    if ((ip != INADDR_NONE) && (ip != INADDR_ANY))
    {
      doDline(BotSock::inet_ntoa(ip), duration, reason);
    }
    break;

  case ACTION_DLINE_NET:
    if ((ip != INADDR_NONE) && (ip != INADDR_ANY))
    {
      doDline(classCMask(BotSock::inet_ntoa(ip)), duration, reason);
    }
    break;
  }
}

