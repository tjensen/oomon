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
#include <boost/bind.hpp>

// OOMon Headers
#include "autoaction.h"
#include "userhash.h"
#include "irc.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "botsock.h"
#include "botclient.h"
#include "vars.h"


std::string
AutoAction::get(const AutoAction * value)
{
  std::string dur;
  if (value->duration() > 0)
  {
    dur += ' ';
    dur += boost::lexical_cast<std::string>(value->duration());
  }

  std::string server;
  if (!value->server_.empty())
  {
    server += '@';
    server += StrJoin(',', value->server_);
  }

  switch (value->type())
  {
    case AutoAction::KILL:
      return "KILL";
    case AutoAction::KLINE:
      return "KLINE" + server + dur;
    case AutoAction::KLINE_HOST:
      return "KLINE_HOST" + server + dur;
    case AutoAction::KLINE_DOMAIN:
      return "KLINE_DOMAIN" + server + dur;
    case AutoAction::KLINE_IP:
      return "KLINE_IP" + server + dur;
    case AutoAction::KLINE_USERNET:
      return "KLINE_USERNET" + server + dur;
    case AutoAction::KLINE_NET:
      return "KLINE_NET" + server + dur;
    case AutoAction::SMART_KLINE:
      return "SMART_KLINE" + server + dur;
    case AutoAction::SMART_KLINE_HOST:
      return "SMART_KLINE_HOST" + server + dur;
    case AutoAction::SMART_KLINE_IP:
      return "SMART_KLINE_IP" + server + dur;
    case AutoAction::DLINE_IP:
      return "DLINE_IP" + dur;
    case AutoAction::DLINE_NET:
      return "DLINE_NET" + dur;
    default:
      return "NOTHING";
  }
}


std::string
AutoAction::set(AutoAction * value, const std::string & newValue)
{
  std::string copy(newValue);
  std::string action(FirstWord(copy));

  std::string::size_type at = action.find('@');
  std::string text;
  if (at == std::string::npos)
  {
    text = UpCase(action);
  }
  else
  {
    text = UpCase(action.substr(0, at));

    StrSplit(value->server_, action.substr(at + 1), ",", true);
  }

  if (partialCompare(text, "DLINE_IP", 7))
  {
    value->type_ = AutoAction::DLINE_IP;
  }
  else if (partialCompare(text, "DLINE_NET", 7))
  {
    value->type_ = AutoAction::DLINE_NET;
  }
  else if (partialCompare(text, "KILL", 2))
  {
    value->type_ = AutoAction::KILL;
  }
  else if ((0 == text.compare("KLINE")) ||
      partialCompare(text, "KLINE_USERDOMAIN", 11))
  {
    value->type_ = AutoAction::KLINE;
  }
  else if (partialCompare(text, "KLINE_DOMAIN", 7))
  {
    value->type_ = AutoAction::KLINE_DOMAIN;
  }
  else if (partialCompare(text, "KLINE_HOST", 7))
  {
    value->type_ = AutoAction::KLINE_HOST;
  }
  else if (partialCompare(text, "KLINE_IP", 7))
  {
    value->type_ = AutoAction::KLINE_IP;
  }
  else if (partialCompare(text, "KLINE_NET", 7))
  {
    value->type_ = AutoAction::KLINE_NET;
  }
  else if (partialCompare(text, "KLINE_USERNET", 11))
  {
    value->type_ = AutoAction::KLINE_USERNET;
  }
  else if (partialCompare(text, "NOTHING", 1))
  {
    value->type_ = AutoAction::NOTHING;
  }
  else if (0 == text.compare("SMART_KLINE"))
  {
    value->type_ = AutoAction::SMART_KLINE;
  }
  else if (partialCompare(text, "SMART_KLINE_HOST", 13))
  {
    value->type_ = AutoAction::SMART_KLINE_HOST;
  }
  else if (partialCompare(text, "SMART_KLINE_IP", 13))
  {
    value->type_ = AutoAction::SMART_KLINE_IP;
  }
  else
  {
    return "*** NOTHING, KILL, KLINE, KLINE_HOST, KLINE_DOMAIN, KLINE_IP, KLINE_USERNET, KLINE_NET, SMART_KLINE, SMART_KLINE_HOST, SMART_KLINE_IP, DLINE_IP, or DLINE_NET expected!";
  }

  std::string dur = FirstWord(copy);
  try
  {
    value->duration_ = boost::lexical_cast<unsigned int>(dur);
  }
  catch (boost::bad_lexical_cast)
  {
    value->duration_ = 0;
  }

  return "";
}


::Setting
AutoAction::Setting(AutoAction & value)
{
  return ::Setting(boost::bind(AutoAction::get, &value),
      boost::bind(AutoAction::set, &value, _1));
}


static void
doKill(const std::string & nick, const std::string & reason)
{
  if (autoPilot())
  {
    server.kill("Auto-Kill", nick, reason);
  }
  else
  {
    ::SendAll(".kill " + nick + " " + reason, UserFlags::OPER);
  }
}


static void
doKline(const StrVector & remote, const std::string & mask,
    unsigned int duration, const std::string & reason)
{
  if (autoPilot())
  {
    if (remote.empty())
    {
      server.kline("Auto-Kline", duration, mask, reason);
    }
    else
    {
      for (StrVector::const_iterator pos = remote.begin(); pos != remote.end();
          ++pos)
      {
        server.remoteKline("Auto-Kline", *pos, duration, mask, reason);
      }
    }
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
doDline(const std::string & mask, const unsigned int duration,
  const std::string & reason)
{
  if (autoPilot())
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
  BotSock::Address ip, const AutoAction & action, const std::string & reason,
  bool suggestKlineAfterKill)
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
    if (isStrictIPv4(host))
    {
      ip = BotSock::inet_addr(host);
    }
    else
    {
      ip = users.getIP(nick, userhost);
    }
  }

  switch (action.type())
  {
  case AutoAction::NOTHING:
    // Do nothing!
    break;

  case AutoAction::KILL:
    doKill(nick, reason);
    if (suggestKlineAfterKill && autoPilot())
    {
      std::string mask("*" + ident + "@" + domain);
      ::SendAll(".kline " + mask + " " + reason,
	UserFlags(UserFlags::OPER, UserFlags::KLINE));
    }
    break;

  case AutoAction::KLINE:
    doKline(action.server(), "*" + ident + "@" + domain, action.duration(),
        reason);
    break;

  case AutoAction::KLINE_HOST:
    doKline(action.server(), "*@" + host, action.duration(), reason);
    break;

  case AutoAction::KLINE_DOMAIN:
    doKline(action.server(), "*@" + domain, action.duration(), reason);
    break;

  case AutoAction::KLINE_IP:
    if (ip != INADDR_NONE)
    {
      doKline(action.server(), "*@" + BotSock::inet_ntoa(ip), action.duration(),
          reason);
    }
    else
    {
      doKline(action.server(), "*@" + host, action.duration(), reason);
    }
    break;

  case AutoAction::KLINE_USERNET:
    if (ip != INADDR_NONE)
    {
      doKline(action.server(),
          "*" + ident + "@" + classCMask(BotSock::inet_ntoa(ip)),
          action.duration(), reason);
    }
    else
    {
      doKline(action.server(), "*" + ident + "@" + domain, action.duration(),
          reason);
    }
    break;

  case AutoAction::KLINE_NET:
    if (ip != INADDR_NONE)
    {
      doKline(action.server(), "*@" + classCMask(BotSock::inet_ntoa(ip)),
          action.duration(),
          reason);
    }
    else
    {
      doKline(action.server(), "*@" + domain, action.duration(), reason);
    }
    break;

  case AutoAction::SMART_KLINE:
    if (isDynamic(user, host))
    {
      doKline(action.server(), "*@" + host, action.duration(), reason);
    }
    else if (INADDR_NONE == ip)
    {
      doKline(action.server(), "*" + ident + "@" + domain, action.duration(),
          reason);
    }
    else
    {
      doKline(action.server(),
          "*" + ident + "@" + classCMask(BotSock::inet_ntoa(ip)),
          action.duration(), reason);
    }
    break;

  case AutoAction::SMART_KLINE_HOST:
    if (isDynamic(user, host))
    {
      doKline(action.server(), "*@" + host, action.duration(), reason);
    }
    else
    {
      doKline(action.server(), "*" + ident + "@" + domain, action.duration(),
          reason);
    }
    break;

  case AutoAction::SMART_KLINE_IP:
    if (ip != INADDR_NONE)
    {
      if (isDynamic(user, host))
      {
        doKline(action.server(), "*@" + BotSock::inet_ntoa(ip),
            action.duration(), reason);
      }
      else
      {
        doKline(action.server(),
            "*" + ident + "@" + classCMask(BotSock::inet_ntoa(ip)),
            action.duration(), reason);
      }
    }
    else
    {
      if (isDynamic(user, host))
      {
        doKline(action.server(), "*@" + host, action.duration(), reason);
      }
      else
      {
        doKline(action.server(), "*" + ident + "@" + domain, action.duration(),
            reason);
      }
    }
    break;

  case AutoAction::DLINE_IP:
    if ((ip != INADDR_NONE) && (ip != INADDR_ANY))
    {
      doDline(BotSock::inet_ntoa(ip), action.duration(), reason);
    }
    break;

  case AutoAction::DLINE_NET:
    if ((ip != INADDR_NONE) && (ip != INADDR_ANY))
    {
      doDline(classCMask(BotSock::inet_ntoa(ip)), action.duration(), reason);
    }
    break;
  }
}


void
doAction(const UserEntryPtr user, const AutoAction & action,
    const std::string & reason, bool suggestKlineAfterKill)
{
  // If we're about to perform a KILL action, make sure the user is still
  // connected to prevent ourselves from accidentally killing someone
  // else who just took the same nickname!
  if (user->connected() || (AutoAction::KILL != action.type()))
  {
    doAction(user->getNick(), user->getUserHost(), user->getIP(), action,
        reason, suggestKlineAfterKill);
  }
}

