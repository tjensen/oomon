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
#include "defaults.h"


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
  std::time_t	nextNotice;
};

static std::list<NickChangeEntry> nickChanges;


static bool autoKlineHost(DEFAULT_AUTO_KLINE_HOST);
static std::string autoKlineHostReason(DEFAULT_AUTO_KLINE_HOST_REASON);
static int autoKlineHostTime(DEFAULT_AUTO_KLINE_HOST_TIME);
static bool autoKlineNet(DEFAULT_AUTO_KLINE_NET);
static std::string autoKlineNetReason(DEFAULT_AUTO_KLINE_NET_REASON);
static int autoKlineNetTime(DEFAULT_AUTO_KLINE_NET_TIME);
static bool autoKlineNoident(DEFAULT_AUTO_KLINE_NOIDENT);
static std::string autoKlineNoidentReason(DEFAULT_AUTO_KLINE_NOIDENT_REASON);
static int autoKlineNoidentTime(DEFAULT_AUTO_KLINE_NOIDENT_TIME);
static bool autoKlineUserhost(DEFAULT_AUTO_KLINE_USERHOST);
static std::string autoKlineUserhostReason(DEFAULT_AUTO_KLINE_USERHOST_REASON);
static int autoKlineUserhostTime(DEFAULT_AUTO_KLINE_USERHOST_TIME);
static bool autoKlineUsernet(DEFAULT_AUTO_KLINE_USERNET);
static std::string autoKlineUsernetReason(DEFAULT_AUTO_KLINE_USERNET_REASON);
static int autoKlineUsernetTime(DEFAULT_AUTO_KLINE_USERNET_TIME);
static bool autoPilot_(DEFAULT_AUTO_PILOT);
static bool checkForSpoofs(DEFAULT_CHECK_FOR_SPOOFS);
static AutoAction fakeIpSpoofAction(DEFAULT_FAKE_IP_SPOOF_ACTION,
    DEFAULT_FAKE_IP_SPOOF_ACTION_TIME);
static std::string fakeIpSpoofReason(DEFAULT_FAKE_IP_SPOOF_REASON);
static AutoAction illegalCharSpoofAction(DEFAULT_ILLEGAL_CHAR_SPOOF_ACTION,
    DEFAULT_ILLEGAL_CHAR_SPOOF_ACTION_TIME);
static std::string illegalCharSpoofReason(DEFAULT_ILLEGAL_CHAR_SPOOF_REASON);
static AutoAction illegalTldSpoofAction(DEFAULT_ILLEGAL_TLD_SPOOF_ACTION,
    DEFAULT_ILLEGAL_TLD_SPOOF_ACTION_TIME);
static std::string illegalTldSpoofReason(DEFAULT_ILLEGAL_TLD_SPOOF_REASON);
static AutoAction invalidUsernameAction(DEFAULT_INVALID_USERNAME_ACTION,
    DEFAULT_INVALID_USERNAME_ACTION_TIME);
static std::string invalidUsernameReason(DEFAULT_INVALID_USERNAME_REASON);
static int nickChangeT1Time(DEFAULT_NICK_CHANGE_T1_TIME);
static int nickChangeT2Time(DEFAULT_NICK_CHANGE_T2_TIME);
static int nickChangeMaxCount(DEFAULT_NICK_CHANGE_MAX_COUNT);
static AutoAction nickFloodAction(DEFAULT_NICK_FLOOD_ACTION,
    DEFAULT_NICK_FLOOD_ACTION_TIME);
static std::string nickFloodReason(DEFAULT_NICK_FLOOD_REASON);
static bool statspCaseInsensitive(DEFAULT_STATSP_CASE_INSENSITIVE);
static std::string statspMessage(DEFAULT_STATSP_MESSAGE);
static bool statspReply(DEFAULT_STATSP_REPLY);
static bool watchFlooderNotices(DEFAULT_WATCH_FLOODER_NOTICES);
static bool watchInfoNotices(DEFAULT_WATCH_INFO_NOTICES);
static bool watchJupeNotices(DEFAULT_WATCH_JUPE_NOTICES);
static bool watchLinksNotices(DEFAULT_WATCH_LINKS_NOTICES);
static bool watchMotdNotices(DEFAULT_WATCH_MOTD_NOTICES);
static bool watchOperfailNotices(DEFAULT_WATCH_OPERFAIL_NOTICES);
static bool watchSpambotNotices(DEFAULT_WATCH_SPAMBOT_NOTICES);
static bool watchStatsNotices(DEFAULT_WATCH_STATS_NOTICES);
static bool watchToomanyNotices(DEFAULT_WATCH_TOOMANY_NOTICES);
static bool watchTraceNotices(DEFAULT_WATCH_TRACE_NOTICES);

static AutoAction linksFloodAction(DEFAULT_LINKS_FLOOD_ACTION,
    DEFAULT_LINKS_FLOOD_ACTION_TIME);
static std::string linksFloodReason(DEFAULT_LINKS_FLOOD_REASON);
static int linksFloodMaxCount(DEFAULT_LINKS_FLOOD_MAX_COUNT);
static int linksFloodMaxTime(DEFAULT_LINKS_FLOOD_MAX_TIME);
static FloodList linkLookers("LINKS", linksFloodAction, linksFloodReason,
    linksFloodMaxCount, linksFloodMaxTime, WATCH_LINKS);

static AutoAction traceFloodAction(DEFAULT_TRACE_FLOOD_ACTION,
    DEFAULT_TRACE_FLOOD_ACTION_TIME);
static std::string traceFloodReason(DEFAULT_TRACE_FLOOD_REASON);
static int traceFloodMaxCount(DEFAULT_TRACE_FLOOD_MAX_COUNT);
static int traceFloodMaxTime(DEFAULT_TRACE_FLOOD_MAX_TIME);
static FloodList traceLookers("TRACE", traceFloodAction, traceFloodReason,
    traceFloodMaxCount, traceFloodMaxTime, WATCH_TRACES);

static AutoAction motdFloodAction(DEFAULT_MOTD_FLOOD_ACTION,
    DEFAULT_MOTD_FLOOD_ACTION_TIME);
static std::string motdFloodReason(DEFAULT_MOTD_FLOOD_REASON);
static int motdFloodMaxCount(DEFAULT_MOTD_FLOOD_MAX_COUNT);
static int motdFloodMaxTime(DEFAULT_MOTD_FLOOD_MAX_TIME);
static FloodList motdLookers("MOTD", motdFloodAction, motdFloodReason,
    motdFloodMaxCount, motdFloodMaxTime, WATCH_MOTDS);

static AutoAction infoFloodAction(DEFAULT_INFO_FLOOD_ACTION,
    DEFAULT_INFO_FLOOD_ACTION_TIME);
static std::string infoFloodReason(DEFAULT_INFO_FLOOD_REASON);
static int infoFloodMaxCount(DEFAULT_INFO_FLOOD_MAX_COUNT);
static int infoFloodMaxTime(DEFAULT_INFO_FLOOD_MAX_TIME);
static FloodList infoLookers("INFO", infoFloodAction, infoFloodReason,
    infoFloodMaxCount, infoFloodMaxTime, WATCH_INFOS);

static AutoAction statsFloodAction(DEFAULT_STATS_FLOOD_ACTION,
    DEFAULT_STATS_FLOOD_ACTION_TIME);
static std::string statsFloodReason(DEFAULT_STATS_FLOOD_REASON);
static int statsFloodMaxCount(DEFAULT_STATS_FLOOD_MAX_COUNT);
static int statsFloodMaxTime(DEFAULT_STATS_FLOOD_MAX_TIME);
static FloodList statsLookers("STATS", statsFloodAction, statsFloodReason,
    statsFloodMaxCount, statsFloodMaxTime, WATCH_STATS);

AutoAction FloodNoticeEntry::flooderAction(DEFAULT_FLOODER_ACTION,
    DEFAULT_FLOODER_ACTION_TIME);
std::string FloodNoticeEntry::flooderReason(DEFAULT_FLOODER_REASON);
int FloodNoticeEntry::flooderMaxCount(DEFAULT_FLOODER_MAX_COUNT);
int FloodNoticeEntry::flooderMaxTime(DEFAULT_FLOODER_MAX_TIME);
static NoticeList<FloodNoticeEntry> possibleFlooders;

AutoAction SpambotNoticeEntry::spambotAction(DEFAULT_SPAMBOT_ACTION,
    DEFAULT_SPAMBOT_ACTION_TIME);
std::string SpambotNoticeEntry::spambotReason(DEFAULT_SPAMBOT_REASON);
int SpambotNoticeEntry::spambotMaxCount(DEFAULT_SPAMBOT_MAX_COUNT);
int SpambotNoticeEntry::spambotMaxTime(DEFAULT_SPAMBOT_MAX_TIME);
static NoticeList<SpambotNoticeEntry> spambots;

AutoAction TooManyConnNoticeEntry::toomanyAction(DEFAULT_TOOMANY_ACTION,
    DEFAULT_TOOMANY_ACTION_TIME);
std::string TooManyConnNoticeEntry::toomanyReason(DEFAULT_TOOMANY_REASON);
int TooManyConnNoticeEntry::toomanyMaxCount(DEFAULT_TOOMANY_MAX_COUNT);
int TooManyConnNoticeEntry::toomanyMaxTime(DEFAULT_TOOMANY_MAX_TIME);
bool TooManyConnNoticeEntry::toomanyIgnoreUsername(DEFAULT_TOOMANY_IGNORE_USERNAME);
static NoticeList<TooManyConnNoticeEntry> tooManyConn;

AutoAction ConnectEntry::connectFloodAction(DEFAULT_CONNECT_FLOOD_ACTION,
    DEFAULT_CONNECT_FLOOD_ACTION_TIME);
std::string ConnectEntry::connectFloodReason(DEFAULT_CONNECT_FLOOD_REASON);
int ConnectEntry::connectFloodMaxCount(DEFAULT_CONNECT_FLOOD_MAX_COUNT);
int ConnectEntry::connectFloodMaxTime(DEFAULT_CONNECT_FLOOD_MAX_TIME);
static NoticeList<ConnectEntry> connects;

AutoAction OperFailNoticeEntry::operfailAction(DEFAULT_OPERFAIL_ACTION,
    DEFAULT_OPERFAIL_ACTION_TIME);
std::string OperFailNoticeEntry::operfailReason(DEFAULT_OPERFAIL_REASON);
int OperFailNoticeEntry::operfailMaxCount(DEFAULT_OPERFAIL_MAX_COUNT);
int OperFailNoticeEntry::operfailMaxTime(DEFAULT_OPERFAIL_MAX_TIME);
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
  const BotSock::Address & ip, const std::string & userClass,
  const bool differentUser, const bool differentIp, const bool identd)
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

  if (config.isExempt(UserAtHost, ip, Config::EXEMPT_SPOOF) ||
      config.isOper(UserAtHost, ip) || (!userClass.empty() &&
        config.isExemptClass(userClass, Config::EXEMPT_SPOOF)))
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
	std::string text(reason.format(autoKlineNetReason));

        if (autoKlineNet && autoPilot_ && kline)
        {
          Notice = "Adding auto-kline for *@" + Net + " :" + text;
          server.kline("Auto-Kline", autoKlineNetTime, "*@" + Net, text);
        }
        else
        {
          Notice = makeKline("*@" + Net, text, autoKlineNetTime);
        }
      }
      else
      {
	std::string text(reason.format(autoKlineNoidentReason));

        if (autoKlineNoident && autoPilot_ && kline)
        {
          Notice = "Adding auto-kline for ~*@" + Net + " :" + text;
          server.kline("Auto-Kline", autoKlineNoidentTime, "~*@" + Net, text);
        }
        else
        {
          Notice = makeKline("~*@" + Net, text, autoKlineNoidentTime);
        }
      }
    }
    else
    {
      std::string text(reason.format(autoKlineUsernetReason));

      if (autoKlineUsernet && autoPilot_ && kline)
      {
        Notice = "Adding auto-kline for *" + User + "@" + Net + " :" + text;

        server.kline("Auto-Kline", autoKlineUsernetTime, "*" + User + "@" + Net,
            text);
      }
      else
      {
        Notice = makeKline("*" + User + "@" + Net, text, autoKlineUsernetTime);
      }
    }
  }
  else
  {
  if (isDynamic("", Host))
  {
    std::string text(reason.format(autoKlineHostReason));

    if (autoKlineHost && autoPilot_ && kline)
    {
      Notice = "Adding auto-kline for *@" + Host + " :" + text;
      server.kline("Auto-Kline", autoKlineHostTime, "*@" + Host, text);
    }
    else
    {
      Notice = makeKline("*@" + Host, text, autoKlineHostTime);
    }
  }
  else
  {
    std::string suggestedHost;
    if (INADDR_NONE == ip)
    {
      if (isStrictIPv4(Host))
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
	std::string text(reason.format(autoKlineHostReason));

        if (autoKlineHost && autoPilot_ && kline)
        {
          Notice = "Adding auto-kline for *@" + Host + " :" + text;

          server.kline("Auto-Kline", autoKlineHostTime, "*@" + Host, text);
        }
        else
        {
          Notice = makeKline("*@" + Host, text, autoKlineHostTime);
        }
      }
      else
      {
	std::string text(reason.format(autoKlineUserhostReason));

        if (autoKlineUserhost && autoPilot_ && kline)
        {
          Notice = "Adding auto-kline for *" + User + "@" + suggestedHost +
            " :" + text;
          server.kline("Auto-Kline", autoKlineUserhostTime,
              "*" + User + "@" + suggestedHost, text);
        }
        else
        {
          Notice = makeKline("*" + User + "@" + suggestedHost, text,
              autoKlineUserhostTime);
        }
      }
    }
    else
    {
      std::string text(reason.format(autoKlineNoidentReason));

      if (autoKlineNoident && autoPilot_ && kline)
      {
        Notice = "Adding auto-kline for ~*@" + suggestedHost + " :" + text;

        server.kline("Auto-Kline", autoKlineNoidentTime, "~*@" + suggestedHost,
            text);
      }
      else
      {
        Notice = makeKline("~*@" + suggestedHost, text, autoKlineNoidentTime);
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
initFloodTables()
{
  nickChanges.clear();
  linkLookers.clear();
  traceLookers.clear();
  motdLookers.clear();
  infoLookers.clear();
  statsLookers.clear();
  spambots.clear();
  possibleFlooders.clear();
  tooManyConn.clear();
  connects.clear();
  operfails.clear();
}


static bool
t1expired(const NickChangeEntry & entry, std::time_t now, std::time_t max)
{
  std::time_t ticks = (now - entry.lastNickChange) / max;

  return (ticks >= entry.nickChangeCount);
}


static bool
t2expired(const NickChangeEntry & entry, std::time_t now, std::time_t max)
{
  std::time_t diff = now - entry.lastNickChange;

  return (diff >= max);
}


static bool
userhostMatch(const NickChangeEntry & entry, const std::string & userhost)
{
  return (0 == entry.userhost.compare(userhost));
}


static void
addToNickChangeList(const std::string & userhost, const std::string & oldNick,
  const std::string & lastNick)
{
  std::time_t currentTime = std::time(NULL);
   
  // expire stale entries
  nickChanges.remove_if(boost::bind(&::t2expired, _1, currentTime,
        nickChangeT2Time));
  nickChanges.remove_if(boost::bind(&::t1expired, _1, currentTime,
      nickChangeT1Time));

  std::list<NickChangeEntry>::iterator ncp = std::find_if(nickChanges.begin(),
    nickChanges.end(), boost::bind(&::userhostMatch, _1, userhost));

  if (nickChanges.end() == ncp)
  {
    NickChangeEntry tmp;

    tmp.userhost = userhost;
    tmp.firstNickChange = currentTime;
    tmp.lastNickChange = currentTime;
    tmp.nickChangeCount = 1;
    tmp.nextNotice = 0;

    nickChanges.push_back(tmp);
  }
  else
  {
    // get time difference
    std::time_t timeDifference = currentTime - ncp->lastNickChange;

    // how many T1 intervals do we have?
    int timeTicks = timeDifference / nickChangeT1Time;

#ifdef ENGINE_DEBUG
    std::cout << "lastNick=" << ncp->lastNick << ", userhost=" <<
      ncp->userhost << ", count=" << ncp->nickChangeCount << ", diff=" <<
      timeDifference << ", ticks=" << timeTicks << ", next=" <<
      ncp->nextNotice << std::endl;
#endif /* ENGINE_DEBUG */

    // just decrement T1 units of nick changes
    ncp->nickChangeCount = (ncp->nickChangeCount - timeTicks) + 1;
    ncp->lastNickChange = currentTime;
    ncp->lastNick = lastNick;

    // now, check for a nick flooder
    if ((ncp->nickChangeCount >= nickChangeMaxCount) &&
        (currentTime >= ncp->nextNotice))
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

      bool exempt(false);
      BotSock::Address ip(INADDR_NONE);
      UserEntryPtr user(users.findUser(oldNick, userhost));
      if (user)
      {
        ip = user->getIP();
        exempt = user->getOper() ||
          config.isExempt(user, Config::EXEMPT_FLOOD) || config.isOper(user);
      }
      else
      {
        // If we can't find the user in our userlist, we won't be able to find
        // its IP address either
        exempt = config.isExempt(userhost, Config::EXEMPT_FLOOD) ||
          config.isOper(userhost);
      }

      Format reason;
      reason.setStringToken('n', lastNick);
      reason.setStringToken('u', userhost);
      reason.setStringToken('i', BotSock::inet_ntoa(ip));
      reason.setStringToken('r', rate);

      if (!exempt)
      {
	doAction(lastNick, userhost, ip, nickFloodAction,
            reason.format(nickFloodReason), true);
      }

      ncp->nextNotice = currentTime + 5;
    }
  }
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

  //connects.onNotice(nick + ' ' + userhost + ' ' + ipString);

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

    BotSock::Address ip(INADDR_NONE);
    std::string className;
    UserEntryPtr find(users.findUser(nick, userhost));
    if (find)
    {
      ip = find->getIP();
      className = find->getClass();
    }

    klineClones(false, "CS detected", user, host, ip, className, false, false,
        identd);

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

  bool exempt = false;

  BotSock::Address ip(INADDR_NONE);
  UserEntryPtr user(users.findUser(nick, userhost));
  if (user)
  {
    ip = user->getIP();
    exempt = user->getOper() || config.isExempt(user, Config::EXEMPT_FLOOD) ||
      config.isOper(user);
  }
  else
  {
    // If we can't find the user in our userlist, we won't be able to find
    // its IP address either
    exempt = config.isExempt(userhost, Config::EXEMPT_FLOOD) ||
      config.isOper(userhost);
  }

  if (!exempt)
  {
    doAction(nick, userhost, ip, nickFloodAction, nickFloodReason, true);
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

  if (watchLinksNotices)
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

  if (watchTraceNotices)
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

  if (watchMotdNotices)
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

  if (watchInfoNotices)
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

    if (statspReply && ((0 == statsType.compare("p")) ||
          (statspCaseInsensitive && (0 == statsType.compare("P")))))
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

      if (!watchStatsNotices)
      {
        ::SendAll(notice, UserFlags::OPER);
      }

      // Assume this is a server with "/stats p" support.  Only report
      // bot DCC connections with +O status
      StrList output;
      clients.statsP(output);

      if (!statspMessage.empty())
      {
        output.push_back(statspMessage);
      }

      server.notice(std::string(nick), output);

      result = true;
    }

    if (watchStatsNotices)
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

  if (watchFlooderNotices)
  {
    result = possibleFlooders.onNotice(text);
  }

  return result;
}


bool
onSpambotNotice(const std::string & text)
{
  bool result = false;

  if (watchSpambotNotices)
  {
    result = spambots.onNotice(text);
  }

  return result;
}


bool
onTooManyConnNotice(const std::string & text)
{
  bool result = false;

  if (watchToomanyNotices)
  {
    result = tooManyConn.onNotice(text);
  }

  return result;
}


void
notListedByDnsbl(const UserEntryPtr user)
{
  proxies.check(user);
}


void
checkProxy(const UserEntryPtr user)
{
  // If the IP is listed by the DNSBL, there's no reason to do a scan
  // of our own!
  dnsbl.check(user, notListedByDnsbl);
}


void
status(BotClient * client)
{
  client->send("Nick changers: " +
    boost::lexical_cast<std::string>(nickChanges.size()));
  client->send("Connect flooders: " +
    boost::lexical_cast<std::string>(connects.size()));
  if (watchLinksNotices)
  {
    client->send("Links lookers: " +
      boost::lexical_cast<std::string>(linkLookers.size()));
  }
  if (watchTraceNotices)
  {
    client->send("Trace lookers: " +
      boost::lexical_cast<std::string>(traceLookers.size()));
  }
  if (watchMotdNotices)
  {
    client->send("Motd lookers: " +
      boost::lexical_cast<std::string>(motdLookers.size()));
  }
  if (watchInfoNotices)
  {
    client->send("Info lookers: " +
      boost::lexical_cast<std::string>(infoLookers.size()));
  }
  if (watchStatsNotices)
  {
    client->send("Stats lookers: " +
      boost::lexical_cast<std::string>(statsLookers.size()));
  }
  if (watchFlooderNotices)
  {
    client->send("Possible flooders: " +
      boost::lexical_cast<std::string>(possibleFlooders.size()));
  }
  if (watchSpambotNotices)
  {
    client->send("Possible spambots: " +
      boost::lexical_cast<std::string>(spambots.size()));
  }
  if (watchToomanyNotices)
  {
    client->send("Too many connections: " +
      boost::lexical_cast<std::string>(tooManyConn.size()));
  }
  if (watchOperfailNotices)
  {
    client->send("Oper fails: " +
      boost::lexical_cast<std::string>(operfails.size()));
  }

  if (watchJupeNotices)
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
  const std::string & host, const std::string & ip,
  const std::string & userClass)
{
  if (checkForSpoofs)
  {
    std::string userhost = user + '@' + host;

    if (!config.isOper(userhost, ip) &&
        !config.spoofer(ip) &&
        !config.isExempt(userhost, ip, Config::EXEMPT_SPOOF) &&
        !config.isExemptClass(userClass, Config::EXEMPT_SPOOF))
    {
      if (isStrictIPv4(host))
      {
        // If we're dealing with an IP that that doesn't reverse-resolve,
        // make sure the "hostname" and ip match up.
	if ((host != ip))
	{
	  std::string notice("Fake IP Spoof: " + nick + " (" + userhost +
	    ") [" + ip + "]");
	  Log::Write(notice);
	  ::SendAll(notice, UserFlags::OPER);
	  doAction(nick, userhost, BotSock::inet_addr(ip), fakeIpSpoofAction,
              fakeIpSpoofReason, false);
	  return true;
	}
      }
      else
      {
        std::string::size_type lastDot = host.rfind('.');
        if (std::string::npos != lastDot)
        {
          std::string tld = server.downCase(host.substr(lastDot + 1));

          std::string::size_type len = tld.length();

          bool legal_top_level = false;

          if (2 == len)
	  {
	    legal_top_level = true;
	  }
          else
          {
            if (0 == tld.compare("aero")) legal_top_level = true;
            if (0 == tld.compare("arpa")) legal_top_level = true;
            if (0 == tld.compare("biz")) legal_top_level = true;
            if (0 == tld.compare("com")) legal_top_level = true;
            if (0 == tld.compare("coop")) legal_top_level = true;
            if (0 == tld.compare("edu")) legal_top_level = true;
            if (0 == tld.compare("gov")) legal_top_level = true;
            if (0 == tld.compare("info")) legal_top_level = true;
            if (0 == tld.compare("int")) legal_top_level = true;
            if (0 == tld.compare("mil")) legal_top_level = true;
            if (0 == tld.compare("museum")) legal_top_level = true;
            if (0 == tld.compare("name")) legal_top_level = true;
            if (0 == tld.compare("net")) legal_top_level = true;
            if (0 == tld.compare("org")) legal_top_level = true;
            if (0 == tld.compare("post")) legal_top_level = true;
            if (0 == tld.compare("pro")) legal_top_level = true;
            if (0 == tld.compare("travel")) legal_top_level = true;
          }

          if (!legal_top_level)
          {
	    std::string notice("Illegal TLD Spoof: " + nick + " (" + userhost +
	      ") [" + ip + "]");
	    Log::Write(notice);
	    ::SendAll(notice, UserFlags::OPER);
	    doAction(nick, userhost, BotSock::inet_addr(ip),
                illegalTldSpoofAction, illegalTldSpoofReason, false);
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
              illegalCharSpoofAction, illegalCharSpoofReason, false);
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

  if (!config.isExempt(userhost, Config::EXEMPT_INVALID))
  {
    doAction(nick, userhost, INADDR_NONE, invalidUsernameAction,
        invalidUsernameReason, false);
  }

  return true;
}


bool
onClearTempKlines(std::string text)
{
  bool result = false;

  if (IRC::trackTempKlines())
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

  if (IRC::trackTempDlines())
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

  if (watchJupeNotices)
  {
    result = jupeJoiners.onNotice(text);
  }

  return result;
}


bool
onMaskHostNotice(std::string text)
{
  bool result = false;

  std::string nick(FirstWord(text));

  if (0 == server.downCase(FirstWord(text)).compare("masking:"))
  {
    std::string realHost(FirstWord(text));

    if (0 == server.downCase(FirstWord(text)).compare("as"))
    {
      std::string fakeHost(FirstWord(text));

      users.setMaskHost(nick, realHost, fakeHost);

      result = true;
    }
  }

  return result;
}


bool
autoPilot(void)
{
  return autoPilot_;
}


void
engine_init(void)
{
  JupeJoinList::init();

  vars.insert("AUTO_PILOT", Setting::BooleanSetting(autoPilot_));
  vars.insert("AUTO_KLINE_HOST", Setting::BooleanSetting(autoKlineHost));
  vars.insert("AUTO_KLINE_HOST_REASON",
      Setting::StringSetting(autoKlineHostReason));
  vars.insert("AUTO_KLINE_HOST_TIME",
      Setting::IntegerSetting(autoKlineHostTime, 0));
  vars.insert("AUTO_KLINE_NET", Setting::BooleanSetting(autoKlineNet));
  vars.insert("AUTO_KLINE_NET_REASON",
      Setting::StringSetting(autoKlineNetReason));
  vars.insert("AUTO_KLINE_NET_TIME",
      Setting::IntegerSetting(autoKlineNetTime, 0));
  vars.insert("AUTO_KLINE_NOIDENT", Setting::BooleanSetting(autoKlineNoident));
  vars.insert("AUTO_KLINE_NOIDENT_REASON",
      Setting::StringSetting(autoKlineNoidentReason));
  vars.insert("AUTO_KLINE_NOIDENT_TIME",
      Setting::IntegerSetting(autoKlineNoidentTime, 0));
  vars.insert("AUTO_KLINE_USERHOST",
      Setting::BooleanSetting(autoKlineUserhost));
  vars.insert("AUTO_KLINE_USERHOST_REASON",
      Setting::StringSetting(autoKlineUserhostReason));
  vars.insert("AUTO_KLINE_USERHOST_TIME",
      Setting::IntegerSetting(autoKlineUserhostTime, 0));
  vars.insert("AUTO_KLINE_USERNET", Setting::BooleanSetting(autoKlineUsernet));
  vars.insert("AUTO_KLINE_USERNET_REASON",
      Setting::StringSetting(autoKlineUsernetReason));
  vars.insert("AUTO_KLINE_USERNET_TIME",
      Setting::IntegerSetting(autoKlineUsernetTime, 0));
  vars.insert("CHECK_FOR_SPOOFS", Setting::BooleanSetting(checkForSpoofs));
  vars.insert("FAKE_IP_SPOOF_ACTION", AutoAction::Setting(fakeIpSpoofAction));
  vars.insert("FAKE_IP_SPOOF_REASON",
      Setting::StringSetting(fakeIpSpoofReason));
  vars.insert("ILLEGAL_CHAR_SPOOF_ACTION",
      AutoAction::Setting(illegalCharSpoofAction));
  vars.insert("ILLEGAL_CHAR_SPOOF_REASON",
      Setting::StringSetting(illegalCharSpoofReason));
  vars.insert("ILLEGAL_TLD_SPOOF_ACTION",
      AutoAction::Setting(illegalTldSpoofAction));
  vars.insert("ILLEGAL_TLD_SPOOF_REASON",
      Setting::StringSetting(illegalTldSpoofReason));
  vars.insert("INVALID_USERNAME_ACTION",
      AutoAction::Setting(invalidUsernameAction));
  vars.insert("INVALID_USERNAME_REASON",
      Setting::StringSetting(invalidUsernameReason));
  vars.insert("NICK_CHANGE_MAX_COUNT",
      Setting::IntegerSetting(nickChangeMaxCount, 1));
  vars.insert("NICK_CHANGE_T1_TIME",
      Setting::IntegerSetting(nickChangeT1Time, 1));
  vars.insert("NICK_CHANGE_T2_TIME",
      Setting::IntegerSetting(nickChangeT2Time, 1));
  vars.insert("NICK_FLOOD_ACTION", AutoAction::Setting(nickFloodAction));
  vars.insert("NICK_FLOOD_REASON", Setting::StringSetting(nickFloodReason));
  vars.insert("STATSP_CASE_INSENSITIVE",
      Setting::BooleanSetting(statspCaseInsensitive));
  vars.insert("STATSP_MESSAGE",
      Setting::StringSetting(statspMessage));
  vars.insert("STATSP_REPLY",
      Setting::BooleanSetting(statspReply));
  vars.insert("WATCH_FLOODER_NOTICES",
      Setting::BooleanSetting(watchFlooderNotices));
  vars.insert("WATCH_INFO_NOTICES",
      Setting::BooleanSetting(watchInfoNotices));
  vars.insert("WATCH_JUPE_NOTICES",
      Setting::BooleanSetting(watchJupeNotices));
  vars.insert("WATCH_LINKS_NOTICES",
      Setting::BooleanSetting(watchLinksNotices));
  vars.insert("WATCH_MOTD_NOTICES",
      Setting::BooleanSetting(watchMotdNotices));
  vars.insert("WATCH_OPERFAIL_NOTICES",
      Setting::BooleanSetting(watchOperfailNotices));
  vars.insert("WATCH_SPAMBOT_NOTICES",
      Setting::BooleanSetting(watchSpambotNotices));
  vars.insert("WATCH_STATS_NOTICES",
      Setting::BooleanSetting(watchStatsNotices));
  vars.insert("WATCH_TOOMANY_NOTICES",
      Setting::BooleanSetting(watchToomanyNotices));
  vars.insert("WATCH_TRACE_NOTICES",
      Setting::BooleanSetting(watchTraceNotices));

  vars.insert("LINKS_FLOOD_ACTION", AutoAction::Setting(linksFloodAction));
  vars.insert("LINKS_FLOOD_REASON", Setting::StringSetting(linksFloodReason));
  vars.insert("LINKS_FLOOD_MAX_COUNT",
      Setting::IntegerSetting(linksFloodMaxCount));
  vars.insert("LINKS_FLOOD_MAX_TIME",
      Setting::IntegerSetting(linksFloodMaxTime));

  vars.insert("MOTD_FLOOD_ACTION", AutoAction::Setting(motdFloodAction));
  vars.insert("MOTD_FLOOD_REASON", Setting::StringSetting(motdFloodReason));
  vars.insert("MOTD_FLOOD_MAX_COUNT",
      Setting::IntegerSetting(motdFloodMaxCount));
  vars.insert("MOTD_FLOOD_MAX_TIME",
      Setting::IntegerSetting(motdFloodMaxTime));

  vars.insert("TRACE_FLOOD_ACTION", AutoAction::Setting(traceFloodAction));
  vars.insert("TRACE_FLOOD_REASON", Setting::StringSetting(traceFloodReason));
  vars.insert("TRACE_FLOOD_MAX_COUNT",
      Setting::IntegerSetting(traceFloodMaxCount));
  vars.insert("TRACE_FLOOD_MAX_TIME",
      Setting::IntegerSetting(traceFloodMaxTime));

  vars.insert("INFO_FLOOD_ACTION", AutoAction::Setting(infoFloodAction));
  vars.insert("INFO_FLOOD_REASON", Setting::StringSetting(infoFloodReason));
  vars.insert("INFO_FLOOD_MAX_COUNT",
      Setting::IntegerSetting(infoFloodMaxCount));
  vars.insert("INFO_FLOOD_MAX_TIME",
      Setting::IntegerSetting(infoFloodMaxTime));

  vars.insert("STATS_FLOOD_ACTION", AutoAction::Setting(statsFloodAction));
  vars.insert("STATS_FLOOD_REASON", Setting::StringSetting(statsFloodReason));
  vars.insert("STATS_FLOOD_MAX_COUNT",
      Setting::IntegerSetting(statsFloodMaxCount));
  vars.insert("STATS_FLOOD_MAX_TIME",
      Setting::IntegerSetting(statsFloodMaxTime));

  vars.insert("FLOODER_ACTION",
      AutoAction::Setting(FloodNoticeEntry::flooderAction));
  vars.insert("FLOODER_REASON",
      Setting::StringSetting(FloodNoticeEntry::flooderReason));
  vars.insert("FLOODER_MAX_COUNT",
      Setting::IntegerSetting(FloodNoticeEntry::flooderMaxCount, 0));
  vars.insert("FLOODER_MAX_TIME",
      Setting::IntegerSetting(FloodNoticeEntry::flooderMaxTime, 1));

  vars.insert("SPAMBOT_ACTION",
      AutoAction::Setting(SpambotNoticeEntry::spambotAction));
  vars.insert("SPAMBOT_REASON",
      Setting::StringSetting(SpambotNoticeEntry::spambotReason));
  vars.insert("SPAMBOT_MAX_COUNT",
      Setting::IntegerSetting(SpambotNoticeEntry::spambotMaxCount, 0));
  vars.insert("SPAMBOT_MAX_TIME",
      Setting::IntegerSetting(SpambotNoticeEntry::spambotMaxTime, 1));

  vars.insert("TOOMANY_ACTION",
      AutoAction::Setting(TooManyConnNoticeEntry::toomanyAction));
  vars.insert("TOOMANY_REASON",
      Setting::StringSetting(TooManyConnNoticeEntry::toomanyReason));
  vars.insert("TOOMANY_MAX_COUNT",
      Setting::IntegerSetting(TooManyConnNoticeEntry::toomanyMaxCount, 0));
  vars.insert("TOOMANY_MAX_TIME",
      Setting::IntegerSetting(TooManyConnNoticeEntry::toomanyMaxTime, 1));
  vars.insert("TOOMANY_IGNORE_USERNAME",
      Setting::BooleanSetting(TooManyConnNoticeEntry::toomanyIgnoreUsername));

  vars.insert("CONNECT_FLOOD_ACTION",
      AutoAction::Setting(ConnectEntry::connectFloodAction));
  vars.insert("CONNECT_FLOOD_REASON",
      Setting::StringSetting(ConnectEntry::connectFloodReason));
  vars.insert("CONNECT_FLOOD_MAX_COUNT",
      Setting::IntegerSetting(ConnectEntry::connectFloodMaxCount, 0));
  vars.insert("CONNECT_FLOOD_MAX_TIME",
      Setting::IntegerSetting(ConnectEntry::connectFloodMaxTime, 1));

  vars.insert("OPERFAIL_ACTION",
      AutoAction::Setting(OperFailNoticeEntry::operfailAction));
  vars.insert("OPERFAIL_REASON",
      Setting::StringSetting(OperFailNoticeEntry::operfailReason));
  vars.insert("OPERFAIL_MAX_COUNT",
      Setting::IntegerSetting(OperFailNoticeEntry::operfailMaxCount, 0));
  vars.insert("OPERFAIL_MAX_TIME",
      Setting::IntegerSetting(OperFailNoticeEntry::operfailMaxTime, 1));
}

