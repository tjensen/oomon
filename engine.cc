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

// Std C++ headers
#include <string>
#include <map>
#include <ctime>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// Std C headers
#include <stdio.h>
#include <netdb.h>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "engine.h"
#include "flood.h"
#include "log.h"
#include "main.h"
#include "irc.h"
#include "util.h"
#include "config.h"
#include "proxylist.h"
#include "trap.h"
#include "dcclist.h"
#include "seedrand.h"
#include "vars.h"
#include "jupe.h"
#include "userhash.h"
#include "notice.h"
#include "botexcept.h"
#include "botclient.h"
#include "dnsbl.h"
#include "format.h"


#ifdef DEBUG
# define ENGINE_DEBUG
#endif



class NickChangeEntry
{
public:
  std::string	userhost;
  std::string	lastNick;
  int		nickChangeCount;
  std::time_t	firstNickChange;
  std::time_t	lastNickChange;
  bool		noticed;
};

static std::list<NickChangeEntry> nickChanges;


static FloodList linkLookers("LINKS", VAR_LINKS_FLOOD_ACTION,
  VAR_LINKS_FLOOD_REASON, VAR_LINKS_FLOOD_MAX_COUNT, VAR_LINKS_FLOOD_MAX_TIME,
  WATCH_LINKS);

static FloodList traceLookers("TRACE", VAR_TRACE_FLOOD_ACTION,
  VAR_TRACE_FLOOD_REASON, VAR_TRACE_FLOOD_MAX_COUNT, VAR_TRACE_FLOOD_MAX_TIME,
  WATCH_TRACES);

static FloodList motdLookers("MOTD", VAR_MOTD_FLOOD_ACTION,
  VAR_MOTD_FLOOD_REASON, VAR_MOTD_FLOOD_MAX_COUNT, VAR_MOTD_FLOOD_MAX_TIME,
  WATCH_MOTDS);

static FloodList infoLookers("INFO", VAR_INFO_FLOOD_ACTION,
  VAR_INFO_FLOOD_REASON, VAR_INFO_FLOOD_MAX_COUNT, VAR_INFO_FLOOD_MAX_TIME,
  WATCH_INFOS);

static FloodList statsLookers("STATS", VAR_STATS_FLOOD_ACTION,
  VAR_STATS_FLOOD_REASON, VAR_STATS_FLOOD_MAX_COUNT, VAR_STATS_FLOOD_MAX_TIME,
  WATCH_STATS);

static NoticeList<FloodNoticeEntry> possibleFlooders;

static NoticeList<SpambotNoticeEntry> spambots;

static NoticeList<TooManyConnNoticeEntry> tooManyConn;

static NoticeList<ConnectEntry> connects;

static NoticeList<OperFailNoticeEntry> operfails;


static std::string
makeKline(const std::string & mask, const std::string & reason,
  const int minutes = 0)
{
  std::string temp;

  if (minutes > 0)
  {
    temp = boost::lexical_cast<std::string>(minutes) + " ";
  }

  return ".kline " + temp + mask + " " + reason;
}


void
klineClones(const bool kline, const std::string & rate,
  const std::string & User, const std::string & Host,
  const BotSock::Address & ip, const bool differentUser, const bool differentIp,
  const bool identd)
{
  std::string Notice;
  std::string UserAtHost = User + "@" + Host;
  std::string UserAtIp = User + "@" + BotSock::inet_ntoa(ip);
  std::string Net = classCMask(BotSock::inet_ntoa(ip));

  if (std::string::npos != UserAtHost.find_first_of("*?"))
  {
    ::SendAll("Bogus DNS spoofed host " + UserAtHost, UserFlags::OPER);
    return;
  }

  if (Config::IsOKHost(UserAtHost, ip) && Config::IsOper(UserAtHost, ip))
  {
    // Don't k-line our friendlies!
    return;
  }

  Format reason;
  reason.setStringToken('r', rate);

  if (differentIp && (ip != INADDR_NONE))
  {
    if (differentUser)
    {
      if (identd)
      {
	std::string text(reason.format(
	  vars[VAR_AUTO_KLINE_NET_REASON]->getString()));

        if (vars[VAR_AUTO_KLINE_NET]->getBool() &&
	  vars[VAR_AUTO_PILOT]->getBool() && kline)
        {
          Notice = "Adding auto-kline for *@" + Net + " :" + text;
          server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_NET_TIME]->getInt(),
            "*@" + Net, text);
        }
        else
        {
          Notice = makeKline("*@" + Net, text,
	    vars[VAR_AUTO_KLINE_NET_TIME]->getInt());
        }
      }
      else
      {
	std::string text(reason.format(
	  vars[VAR_AUTO_KLINE_NOIDENT_REASON]->getString()));

        if (vars[VAR_AUTO_KLINE_NOIDENT]->getBool() &&
          vars[VAR_AUTO_PILOT]->getBool() && kline)
        {
          Notice = "Adding auto-kline for ~*@" + Net + " :" + text;
          server.kline("Auto-Kline",
	    vars[VAR_AUTO_KLINE_NOIDENT_TIME]->getInt(), "~*@" + Net, text);
        }
        else
        {
          Notice = makeKline("~*@" + Net, text,
	    vars[VAR_AUTO_KLINE_NOIDENT_TIME]->getInt());
        }
      }
    }
    else
    {
      std::string text(reason.format(
	vars[VAR_AUTO_KLINE_USERNET_REASON]->getString()));

      if (vars[VAR_AUTO_KLINE_USERNET]->getBool() &&
        vars[VAR_AUTO_PILOT]->getBool() && kline)
      {
        Notice = "Adding auto-kline for *" + User + "@" + Net + " :" + text;

        server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_USERNET_TIME]->getInt(),
	  "*" + User + "@" + Net, text);
      }
      else
      {
        Notice = makeKline("*" + User + "@" + Net, text,
          vars[VAR_AUTO_KLINE_USERNET_TIME]->getInt());
      }
    }
  }
  else
  {
  if (isDynamic("", Host))
  {
    std::string text(reason.format(
      vars[VAR_AUTO_KLINE_HOST_REASON]->getString()));

    if (vars[VAR_AUTO_KLINE_HOST]->getBool() &&
      vars[VAR_AUTO_PILOT]->getBool() && kline)
    {
      Notice = "Adding auto-kline for *@" + Host + " :" + text;
      server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_HOST_TIME]->getInt(),
        "*@" + Host, text);
    }
    else
    {
      Notice = makeKline("*@" + Host, text,
        vars[VAR_AUTO_KLINE_HOST_TIME]->getInt());
    }
  }
  else
  {
    std::string suggestedHost;
    if (INADDR_NONE == ip)
    {
      if (isIP(Host))
      {
	// Get subnet from hostname
        suggestedHost = classCMask(Host);
      }
      else
      {
	// Get domain from hostname
        suggestedHost = "*" + getDomain(Host, true);
      }
    }
    else
    {
      // Get subnet from IP
      suggestedHost = classCMask(BotSock::inet_ntoa(ip));
    }

    if (identd)
    {
      if (differentUser)
      {
	std::string text(reason.format(
	  vars[VAR_AUTO_KLINE_HOST_REASON]->getString()));

        if (vars[VAR_AUTO_KLINE_HOST]->getBool() &&
          vars[VAR_AUTO_PILOT]->getBool() && kline)
        {
          Notice = "Adding auto-kline for *@" + Host + " :" + text;

          server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_HOST_TIME]->getInt(),
            "*@" + Host, text);
        }
        else
        {
          Notice = makeKline("*@" + Host, text,
            vars[VAR_AUTO_KLINE_HOST_TIME]->getInt());
        }
      }
      else
      {
	std::string text(reason.format(
	  vars[VAR_AUTO_KLINE_USERHOST_REASON]->getString()));

        if (vars[VAR_AUTO_KLINE_USERHOST]->getBool() &&
          vars[VAR_AUTO_PILOT]->getBool() && kline)
        {
          Notice = "Adding auto-kline for *" + User + "@" + suggestedHost +
            " :" + text;
          server.kline("Auto-Kline",
	    vars[VAR_AUTO_KLINE_USERHOST_TIME]->getInt(),
            "*" + User + "@" + suggestedHost, text);
        }
        else
        {
          Notice = makeKline("*" + User + "@" + suggestedHost, text,
            vars[VAR_AUTO_KLINE_USERHOST_TIME]->getInt());
        }
      }
    }
    else
    {
      std::string text(reason.format(
	vars[VAR_AUTO_KLINE_NOIDENT_REASON]->getString()));

      if (vars[VAR_AUTO_KLINE_NOIDENT]->getBool() &&
        vars[VAR_AUTO_PILOT]->getBool() && kline)
      {
        Notice = "Adding auto-kline for ~*@" + suggestedHost + " :" + text;

        server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_NOIDENT_TIME]->getInt(),
          "~*@" + suggestedHost, text);
      }
      else
      {
        Notice = makeKline("~*@" + suggestedHost, text,
          vars[VAR_AUTO_KLINE_NOIDENT_TIME]->getInt());
      }
    }
  }
  }

  if (Notice.length() > 0)
  {
    ::SendAll(Notice, UserFlags::OPER, WATCH_KLINES);
  }
}


void
Init_Nick_Change_Table()
{
  nickChanges.clear();
}


static void
addToNickChangeList(const std::string & userhost, const std::string & oldNick,
  const std::string & lastNick)
{
  std::time_t currentTime = std::time(NULL);
   
  bool foundEntry = false;
   
  for (std::list<NickChangeEntry>::iterator ncp = nickChanges.begin();
    ncp != nickChanges.end(); ++ncp)
  {         
    std::time_t timeDifference = currentTime - ncp->lastNickChange;

    /* is it stale ? */
    if (timeDifference >= vars[VAR_NICK_CHANGE_T2_TIME]->getInt())
    {
      nickChanges.erase(ncp);
      --ncp;
    }
    else
    {
      /* how many 10 second intervals do we have? */
      int timeTicks = timeDifference / vars[VAR_NICK_CHANGE_T1_TIME]->getInt();

      /* is it stale? */
      if (timeTicks >= ncp->nickChangeCount)
      {
	nickChanges.erase(ncp);
	--ncp;
      }
      else
      {
        /* just decrement 10 second units of nick changes */
        ncp->nickChangeCount -= timeTicks;
        if (0 == ncp->userhost.compare(userhost))
        {
          ncp->lastNickChange = currentTime;
          ncp->lastNick = lastNick;
          ncp->nickChangeCount++;
          foundEntry = true;
        }

        /* now, check for a nick flooder */

        if ((ncp->nickChangeCount >= vars[VAR_NICK_CHANGE_MAX_COUNT]->getInt())
	  && !ncp->noticed)
        {
	  std::string rate(boost::lexical_cast<std::string>(ncp->nickChangeCount));
	  rate += " in ";
	  rate += boost::lexical_cast<std::string>(ncp->lastNickChange -
	    ncp->firstNickChange);
	  rate += " second";
	  if (1 != (ncp->lastNickChange - ncp->firstNickChange))
	  {
	    rate += 's';
	  }

	  std::string notice("Nick flood ");
	  notice += ncp->userhost;
	  notice += " (";
	  notice += ncp->lastNick;
	  notice += ") ";
	  notice += rate;

          ::SendAll(notice, UserFlags::OPER);
	  Log::Write(notice);

	  BotSock::Address ip = users.getIP(oldNick, userhost);

	  Format reason;
	  reason.setStringToken('n', lastNick);
	  reason.setStringToken('u', userhost);
	  reason.setStringToken('i', BotSock::inet_ntoa(ip));
	  reason.setStringToken('r', rate);

          if (!Config::IsOKHost(userhost, ip) && !Config::IsOper(userhost, ip))
	  {
            doAction(lastNick, userhost, ip,
              vars[VAR_NICK_FLOOD_ACTION]->getAction(),
              vars[VAR_NICK_FLOOD_ACTION]->getInt(),
	      reason.format(vars[VAR_NICK_FLOOD_REASON]->getString()), true);
          }

          ncp->noticed = true;
        }
      }
    }
  }

  if (!foundEntry)
  {
    NickChangeEntry ncp;

    ncp.userhost = userhost;
    ncp.firstNickChange = currentTime;
    ncp.lastNickChange = currentTime;
    ncp.nickChangeCount = 1;
    ncp.noticed = false;

    nickChanges.push_back(ncp);
  }
}


void Init_Link_Look_Table()
{
  linkLookers.clear();
}


// onTraceUser(Type, Class, Nick, UserHost, IP)
//
//
//
void
onTraceUser(const std::string & Type, const std::string & Class,
  const std::string & Nick, std::string UserHost,
  std::string IP = "")
{
  if (Type.empty() || Nick.empty() || UserHost.empty())
    return;

  // Remove the brackets surrounding the user@host
  if (UserHost.length() >= 2)
  {
    UserHost = UserHost.substr(1, UserHost.length() - 2);
  }

  // This might not be necessary, but we'll do it just in case...
  if ((UserHost.length() >= 3) && (0 == UserHost.substr(0, 3).compare("(+)")))
  {
    UserHost = UserHost.substr(3, std::string::npos);
  }

  // Remove the parentheses surrounding the IP
  if (IP.length() >= 2)
  {
    IP = IP.substr(1, IP.length() - 2);
  }

  users.add(Nick, UserHost, IP, true, (0 == Type.compare("Oper")), Class);
}


// onTraceUser(Type, Class, NUH, IP)
//
//
//
void
onTraceUser(const std::string & Type, const std::string & Class,
  const std::string & NUH, std::string IP = "")
{
  std::string Nick;
  std::string UserHost;

  if (Type.empty() || NUH.empty())
    return;

  // Split the nick and user@host from the nick[user@host]
  Nick = chopNick(NUH);
  UserHost = chopUserhost(NUH);

  // hybrid-6 borked trace :P
  if ((UserHost.length() >= 3) && (0 == UserHost.substr(0, 3).compare("(+)")))
  {
    UserHost = UserHost.substr(3);
  }

  // Remove the parentheses surrounding the IP
  if (IP.length() >= 2)
  {
    IP = IP.substr(1, IP.length() - 2);
  }

  users.add(Nick, UserHost, IP, true, (0 == Type.compare("Oper")), Class);
}


// onETraceUser(Type, Class, Nick, User, Host, IP, Gecos)
//
//
//
void
onETraceUser(const std::string & Type, const std::string & Class,
  const std::string & Nick, std::string User, const std::string & Host,
  const std::string & IP, const std::string & Gecos)
{
  if (Type.empty() || Nick.empty() || User.empty() || Host.empty())
    return;

  users.add(Nick, User + '@' + Host, IP, true, (0 == Type.compare("Oper")),
    Class, Gecos);
}


bool
onClientConnect(std::string text)
{
// Client connecting: nick (user@host) [ip] {class} [gecos]
  text.erase(static_cast<std::string::size_type>(0), 19);

  std::string copy(text);

  std::string nick(FirstWord(text));

  std::string userhost(FirstWord(text));
  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  std::string ipString(FirstWord(text));
  if (ipString.length() >= 2)
  {
    ipString.erase(ipString.begin());
    ipString.erase(ipString.end() - 1);
  }

  std::string classString(FirstWord(text));
  if (classString.length() >= 2)
  {
    classString.erase(classString.begin());
    classString.erase(classString.end() - 1);
  }

  std::string gecos(text);
  if (gecos.length() >= 2)
  {
    gecos.erase(gecos.begin());
    gecos.erase(gecos.end() - 1);
  }

  connects.onNotice(nick + ' ' + userhost + ' ' + ipString);

  users.add(nick, userhost, ipString, false, false, classString, gecos);

  ::SendAll("Connected: " + copy, UserFlags::OPER, WATCH_CONNECTS);

  return true;
}


bool
onClientExit(std::string text)
{
  // Client exiting: cihuyy (ngocol@p6.nas2.is4.u-net.net) [Leaving] [195.102.196.13]
  text.erase(static_cast<std::string::size_type>(0), 16);

  std::string copy(text);

  std::string nick(FirstWord(text));

  std::string userhost(FirstWord(text));
  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  BotSock::Address ip(INADDR_ANY);

  std::string ipString;
  std::string::size_type ipFind(text.rfind(' '));
  if (std::string::npos != ipFind)
  {
    ipString = text.substr(ipFind + 1);
    if (ipString.length() >= 2)
    {
      ipString.erase(ipString.begin());
      ipString.erase(ipString.end() - 1);
    }
    ip = BotSock::inet_addr(ipString);
  }

  users.remove(nick, userhost, ip);
  ::SendAll("Disconnected: " + copy, UserFlags::CONN, WATCH_DISCONNECTS);

  return true;
}


bool
onBotReject(std::string text)
{
  return false;
}


bool
onCsClones(std::string text)
{
  bool result = false;

  // Rejecting clonebot: nick (user@host)
  // Clonebot killed: nick (user@host)
  std::string first(server.downCase(FirstWord(text)));
  std::string second(server.downCase(FirstWord(text)));

  std::string nick(FirstWord(text));

  std::string userhost(FirstWord(text));

  // Remove the brackets surrounding the user@host
  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  std::string::size_type at = userhost.find('@');
  if (std::string::npos != at)
  {
    std::string user(userhost.substr(0, at));
    std::string host(userhost.substr(at + 1));

    bool identd = (0 != user.find('~'));

    if (identd)
    {
      user.erase(user.begin());
    }

    ::SendAll("CS clones: " + userhost, UserFlags::OPER);
    Log::Write("CS clones: " + userhost);

    klineClones(false, "CS detected", user, host, users.getIP(nick, userhost),
      false, false, identd);

    result = true;
  }

  return result;
}


bool
onCsNickFlood(std::string text)
{
  // Nick flooding detected by: nick (user@host)
  text.erase(static_cast<std::string::size_type>(0), 27);

  std::string notice("CS Nick Flood: ");
  notice += text;

  std::string nick(FirstWord(text));
  std::string userhost(FirstWord(text));

  // Remove the brackets surrounding the user@host
  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);

  BotSock::Address ip = users.getIP(nick, userhost);

  if ((!Config::IsOKHost(userhost, ip)) && (!Config::IsOper(userhost, ip)))
  {
    doAction(nick, userhost, ip, vars[VAR_NICK_FLOOD_ACTION]->getAction(),
      vars[VAR_NICK_FLOOD_ACTION]->getInt(),
      vars[VAR_NICK_FLOOD_REASON]->getString(), true);
  }

  return true;
}     


bool
onNickChange(std::string text)
{
// Nick change: From ToastTEST to Toast1234 [toast@Plasma.Toast.PC]
  text.erase(static_cast<std::string::size_type>(0), 13);

  if (FirstWord(text) != "From")
    return false;

  std::string nick1 = FirstWord(text);

  if (FirstWord(text) != "to")
    return false;

  std::string nick2 = FirstWord(text);

  std::string userhost = FirstWord(text);

  // Remove the brackets surrounding the user@host
  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  ::SendAll("Nick change: " + nick1 + " -> " + nick2 + " (" + userhost + ")",
    UserFlags::NICK, WATCH_NICK_CHANGES);

  addToNickChangeList(server.downCase(userhost), nick1, nick2);
  users.updateNick(nick1, userhost, nick2);

  return true;
}


bool
onKillNotice(std::string text)
{
// Received KILL message for target. From killer Path: plasma.engr.Arizona.EDU!killer (reason)
#ifdef ENGINE_DEBUG
  std::cout << "::onKillNotice(" << text << ")" << std::endl;
#endif /* ENGINE_DEBUG */

  text.erase(static_cast<std::string::size_type>(0), 26);

  // Killed nick
  std::string target(FirstWord(text));
  if (!target.empty() && (target.find('.') == (target.length() - 1)))
  {
    // Remove the trailing '.' character
    target.erase(target.end() - 1);
  }

#ifdef ENGINE_DEBUG
  std::cout << "target = " << target << std::endl;
#endif /* ENGINE_DEBUG */

  if (users.have(target))
  {
    // We only want to report kills on local users
    if (server.downCase(FirstWord(text)) != "from")
      return false;

    // Killer's nick
    std::string killer(FirstWord(text));

#ifdef ENGINE_DEBUG
  std::cout << "killer = " << killer << std::endl;
#endif /* ENGINE_DEBUG */

    bool global(!users.have(killer));

    if ((std::string::npos != killer.find('@')) || 
      (std::string::npos != killer.find('!')) || 
      (std::string::npos == killer.find('.')))
    {
      // We don't want to see server kills :P

      if (server.downCase(FirstWord(text)) != "path:")
	return false;

      // Kill path - we don't use this
      std::string path(FirstWord(text));

      // Kill reason
      std::string reason(text);

      std::string notice(global ? "Global kill for " : "Local kill for ");
      notice += target;
      notice += " by ";
      notice += killer;
      if (!reason.empty())
      {
        notice += ' ';
        notice += reason;
      }

      ::SendAll(notice, UserFlags::OPER, WATCH_KILLS);
      Log::Write(notice);

      return true;
    }
  }
  return false;
}


bool
onLinksNotice(std::string text)
{
  bool result = false;

  if (vars[VAR_WATCH_LINKS_NOTICES]->getBool())
  {
    std::string notice("[");
    notice += text;
    notice += ']';

    if (0 == server.downCase(FirstWord(text)).compare("links"))
    {
      if (!text.empty() && (text[0] == '\''))
      {
        // This is a +th server, skip the '...' part

        std::string::size_type end = text.find('\'', 1);

        if (std::string::npos != end)
        {
          std::string::size_type next = text.find_first_not_of(' ', end + 1);

          if (std::string::npos != next)
          {
            result = linkLookers.onNotice(notice, text.substr(next));
          }
        }
      }
      else
     {
        result = linkLookers.onNotice(notice, text);
      }
    }
  }

  return result;
}


bool
onTraceNotice(std::string text)
{
  bool result = false;

  if (vars[VAR_WATCH_TRACE_NOTICES]->getBool())
  {
    std::string notice("[");
    notice += text;
    notice += ']';

    if (0 == server.downCase(FirstWord(text)).compare("trace"))
    {
      result = traceLookers.onNotice(notice, text);
    }
  }

  return result;
}


bool
onMotdNotice(std::string text)
{
  bool result = false;

  if (vars[VAR_WATCH_MOTD_NOTICES]->getBool())
  {
    std::string notice("[");
    notice += text;
    notice += ']';

    if (0 == server.downCase(FirstWord(text)).compare("motd"))
    {
      result = motdLookers.onNotice(notice, text);
    }
  }

  return result;
}


bool
onInfoNotice(std::string text)
{
  bool result = false;

  if (vars[VAR_WATCH_INFO_NOTICES]->getBool())
  {
    std::string notice("[");
    notice += text;
    notice += ']';

    if (0 == server.downCase(FirstWord(text)).compare("info"))
    {
      result = infoLookers.onNotice(notice, text);
    }
  }

  return result;
}


bool
onStatsNotice(std::string text)
{
  bool result = false;

  std::string notice("[");
  notice += text;
  notice += ']';

  if (0 == server.downCase(FirstWord(text)).compare("stats"))
  {
    std::string statsType = FirstWord(text);

    if (vars[VAR_STATSP_REPLY]->getBool() && ((0 == statsType.compare("p")) ||
      (vars[VAR_STATSP_CASE_INSENSITIVE]->getBool() &&
      (0 == statsType.compare("P")))))
    {
      std::string copy = text;

      if (server.downCase(FirstWord(copy)) != "requested")
      {
        // broken.  ignore it.
        return false;
      }

      if (server.downCase(FirstWord(copy)) != "by")
      {
        // broken.  ignore it.
        return false;
      }

      std::string nick = FirstWord(copy);

      if (nick.empty())
      {
        // broken.  ignore it.
        return false;
      }

      if (!vars[VAR_WATCH_STATS_NOTICES]->getBool())
      {
        ::SendAll(notice, UserFlags::OPER);
      }

      // Assume this is a server with "/stats p" support.  Only report
      // bot DCC connections with +O status
      StrList output;
      clients.statsP(output);

      std::string message = vars[VAR_STATSP_MESSAGE]->getString();
      if (message.length() > 0)
      {
        output.push_back(message);
      }

      server.notice(std::string(nick), output);

      result = true;
    }

    if (vars[VAR_WATCH_STATS_NOTICES]->getBool())
    {
      result = statsLookers.onNotice(notice, text);
    }
  }

  return result;
}


bool
onFlooderNotice(const std::string & text)
{
  bool result = false;

  if (vars[VAR_WATCH_FLOODER_NOTICES]->getBool())
  {
    result = possibleFlooders.onNotice(text);
  }

  return result;
}


bool
onSpambotNotice(const std::string & text)
{
  bool result = false;

  if (vars[VAR_WATCH_SPAMBOT_NOTICES]->getBool())
  {
    result = spambots.onNotice(text);
  }

  return result;
}


bool
onTooManyConnNotice(const std::string & text)
{
  bool result = false;

  if (vars[VAR_WATCH_TOOMANY_NOTICES]->getBool())
  {
    result = tooManyConn.onNotice(text);
  }

  return result;
}


void
CheckProxy(const std::string & ip, const std::string & host,
  const std::string & nick, const std::string & userhost)
{
  // If the IP is listed by the DNSBL, there's no reason to do a Wingate
  // or SOCKS proxy check of our own!
  if (!dnsbl.check(BotSock::inet_addr(ip), nick, userhost))
  {
    if (vars[VAR_SCAN_FOR_PROXIES]->getBool())
    {
      proxies.check(ip, host, nick, userhost);
    }
  }
}


void
status(BotClient * client)
{
  client->send("Nick changers: " +
    boost::lexical_cast<std::string>(nickChanges.size()));
  client->send("Connect flooders: " +
    boost::lexical_cast<std::string>(connects.size()));
  if (vars[VAR_WATCH_LINKS_NOTICES]->getBool())
  {
    client->send("Links lookers: " +
      boost::lexical_cast<std::string>(linkLookers.size()));
  }
  if (vars[VAR_WATCH_TRACE_NOTICES]->getBool())
  {
    client->send("Trace lookers: " +
      boost::lexical_cast<std::string>(traceLookers.size()));
  }
  if (vars[VAR_WATCH_MOTD_NOTICES]->getBool())
  {
    client->send("Motd lookers: " +
      boost::lexical_cast<std::string>(motdLookers.size()));
  }
  if (vars[VAR_WATCH_INFO_NOTICES]->getBool())
  {
    client->send("Info lookers: " +
      boost::lexical_cast<std::string>(infoLookers.size()));
  }
  if (vars[VAR_WATCH_STATS_NOTICES]->getBool())
  {
    client->send("Stats lookers: " +
      boost::lexical_cast<std::string>(statsLookers.size()));
  }
  if (vars[VAR_WATCH_FLOODER_NOTICES]->getBool())
  {
    client->send("Possible flooders: " +
      boost::lexical_cast<std::string>(possibleFlooders.size()));
  }
  if (vars[VAR_WATCH_SPAMBOT_NOTICES]->getBool())
  {
    client->send("Possible spambots: " +
      boost::lexical_cast<std::string>(spambots.size()));
  }
  if (vars[VAR_WATCH_TOOMANY_NOTICES]->getBool())
  {
    client->send("Too many connections: " +
      boost::lexical_cast<std::string>(tooManyConn.size()));
  }
  if (vars[VAR_WATCH_OPERFAIL_NOTICES]->getBool())
  {
    client->send("Oper fails: " +
      boost::lexical_cast<std::string>(operfails.size()));
  }

  if (vars[VAR_WATCH_JUPE_NOTICES]->getBool())
  {
    jupeJoiners.status(client);
  }

  client->send("Up Time: " + ::getUptime());
}


bool
onOperNotice(std::string text)
{
  std::string nick(FirstWord(text));
  std::string userhost(FirstWord(text));

  if (userhost.length() > 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  if ("is" != server.downCase(FirstWord(text)))
    return false;
  if ("now" != server.downCase(FirstWord(text)))
    return false;
  if ("an" != server.downCase(FirstWord(text)))
    return false;;
  if ("operator" != server.downCase(FirstWord(text)))
    return false;;

  users.updateOper(nick, userhost, true);

  return true;
}

bool
onOperFailNotice(const std::string & text)
{
  return operfails.onNotice(text);
}


bool
checkForSpoof(const std::string & nick, const std::string & user, 
  const std::string & host, const std::string & ip)
{
  if (vars[VAR_CHECK_FOR_SPOOFS]->getBool())
  {
    std::string userhost = user + '@' + host;

    if (!Config::IsOper(userhost, ip) && !Config::IsOKHost(userhost, ip) &&
      !Config::IsSpoofer(ip))
    {
      if (isIP(host))
      {
        // If we're dealing with an IP that that doesn't reverse-resolve,
        // make sure the "hostname" and ip match up.
        if ((ip != "") && isIP(host))
        {
          if ((host != ip))
          {
            std::string notice("Fake IP Spoof: " + nick + " (" + userhost +
	      ") [" + ip + "]");
	    Log::Write(notice);
	    ::SendAll(notice, UserFlags::OPER);
	    doAction(nick, userhost, BotSock::inet_addr(ip),
	      vars[VAR_FAKE_IP_SPOOF_ACTION]->getAction(),
	      vars[VAR_FAKE_IP_SPOOF_ACTION]->getInt(),
	      vars[VAR_FAKE_IP_SPOOF_REASON]->getString(), false);
            return true;
          }
        }
      }
      else
      {
        std::string::size_type lastDot = host.rfind('.');
        if (std::string::npos != lastDot)
        {
          std::string tld = host.substr(lastDot + 1);

          std::string::size_type len = tld.length();

          bool legal_top_level = false;

          if (2 == len)
	  {
	    legal_top_level = true;
	  }
          else if (3 == len)
          {
            // Don't forget .ARPA !!!! :P
            if (0 == tld.compare("net")) legal_top_level = true;
            if (0 == tld.compare("com")) legal_top_level = true;
            if (0 == tld.compare("org")) legal_top_level = true;
            if (0 == tld.compare("gov")) legal_top_level = true;
            if (0 == tld.compare("edu")) legal_top_level = true;
            if (0 == tld.compare("mil")) legal_top_level = true;
            if (0 == tld.compare("int")) legal_top_level = true;
            if (0 == tld.compare("biz")) legal_top_level = true;
          }
	  else if (4 == len)
	  {
            if (0 == tld.compare("arpa")) legal_top_level = true;
            if (0 == tld.compare("info")) legal_top_level = true;
            if (0 == tld.compare("name")) legal_top_level = true;
	  }

          if (!legal_top_level)
          {
	    std::string notice("Illegal TLD Spoof: " + nick + " (" + userhost +
	      ") [" + ip + "]");
	    Log::Write(notice);
	    ::SendAll(notice, UserFlags::OPER);
	    doAction(nick, userhost, BotSock::inet_addr(ip),
	      vars[VAR_ILLEGAL_TLD_SPOOF_ACTION]->getAction(),
	      vars[VAR_ILLEGAL_TLD_SPOOF_ACTION]->getInt(),
	      vars[VAR_ILLEGAL_TLD_SPOOF_REASON]->getString(), false);
            return true;
          }
        }
        if (std::string::npos != host.find_first_of("@*?"))
        {
	  std::string notice("Illegal Character Spoof: " + nick + " (" +
	    userhost + ") [" + ip + "]");
	  Log::Write(notice);
	  ::SendAll(notice, UserFlags::OPER);
	  doAction(nick, userhost, BotSock::inet_addr(ip),
	    vars[VAR_ILLEGAL_CHAR_SPOOF_ACTION]->getAction(),
	    vars[VAR_ILLEGAL_CHAR_SPOOF_ACTION]->getInt(),
	    vars[VAR_ILLEGAL_CHAR_SPOOF_REASON]->getString(), false);
          return true;
        }
      }
    }
  }
  return false;
}


bool
onInvalidUsername(std::string text)
{
  text.erase(static_cast<std::string::size_type>(0), 18);

  std::string nick(FirstWord(text));

  std::string userhost(FirstWord(text));
  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  doAction(nick, userhost, INADDR_NONE,
    vars[VAR_INVALID_USERNAME_ACTION]->getAction(),
    vars[VAR_INVALID_USERNAME_ACTION]->getInt(),
    vars[VAR_INVALID_USERNAME_REASON]->getString(), false);

  return true;
}


bool
onClearTempKlines(std::string text)
{
  bool result = false;

 if (vars[VAR_TRACK_TEMP_KLINES]->getBool())
 {
    std::string notice("*** ");
    notice += text;

    std::string nick(FirstWord(text));

    if (0 == text.compare("is clearing temp klines"))
    {
      ::SendAll(notice, UserFlags::OPER, WATCH_KLINES);
      Log::Write(notice);
      server.reloadKlines();
      result = true;
    }
  }

  return result;
}


bool
onClearTempDlines(std::string text)
{
  bool result = false;

  if (vars[VAR_TRACK_TEMP_DLINES]->getBool())
  {
    std::string notice("*** ");
    notice += text;

    std::string nick(FirstWord(text));

    if (0 == text.compare("is clearing temp dlines"))
    {
      ::SendAll(notice, UserFlags::OPER, WATCH_KLINES);
      Log::Write(notice);
      server.reloadDlines();
      result = true;
    }
  }

  return result;
}


bool
onGlineRequest(std::string text)
{
  std::string nuh(FirstWord(text));

  if ("on" != server.downCase(FirstWord(text)))
    return false;

  std::string serverName(FirstWord(text));

  std::string ishas(FirstWord(text));

  std::string voodoo;

  if (0 == server.downCase(ishas).compare("is"))
  {
    if ("requesting" != server.downCase(FirstWord(text)))
      return false;

    voodoo = " requested ";
  }
  else if (0 == server.downCase(ishas).compare("has"))
  {
    if ("triggered" != server.downCase(FirstWord(text)))
      return false;
    
    voodoo = " triggered ";
  }
  else
  {
    return false;
  }

  if ("gline" != server.downCase(FirstWord(text)))
    return false;

  if ("for" != server.downCase(FirstWord(text)))
    return false;

  std::string mask(FirstWord(text));
  if (mask.length() > 2)
  {
    mask.erase(mask.begin());
    mask.erase(mask.end() - 1);
  }

  std::string reason(text);
  if (reason.length() > 2)
  {
    reason.erase(reason.begin());
    reason.erase(reason.end() - 1);
  }

  std::string msg(getNick(nuh));
  msg += voodoo;
  msg += "G-line: ";
  msg += mask;
  msg += " (";
  msg += reason;
  msg += ')';

  ::SendAll(msg, UserFlags::OPER, WATCH_GLINES);
  Log::Write(msg);

  return true;
}


bool
onLineActive(std::string text)
{
  std::string notice(text);

  std::string lineType(server.downCase(FirstWord(text)));

  Watch w;

  if (0 == lineType.compare("kline"))
  {
    w = WATCH_KLINE_MATCHES;
  }
  else if (0 == lineType.compare("dline"))
  {
    w = WATCH_DLINE_MATCHES;
  }
  else if (0 == lineType.compare("gline"))
  {
    w = WATCH_GLINE_MATCHES;
  }
  else
  {
    return false;
  }

  ::SendAll(notice, UserFlags::OPER, w);
  Log::Write(notice);

  return true;
}


bool
onJupeJoinNotice(const std::string & text)
{
  bool result = false;

  if (vars[VAR_WATCH_JUPE_NOTICES]->getBool())
  {
    result = jupeJoiners.onNotice(text);
  }

  return result;
}

