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
#include <ctime>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// OOMon Headers
#include "strtype"
#include "jupe.h"
#include "util.h"
#include "main.h"
#include "log.h"
#include "irc.h"
#include "vars.h"
#include "userhash.h"
#include "botclient.h"
#include "config.h"
#include "format.h"
#include "defaults.h"


AutoAction JupeJoinList::action(DEFAULT_JUPE_JOIN_ACTION,
    DEFAULT_JUPE_JOIN_ACTION_TIME);
bool JupeJoinList::ignoreChannel(DEFAULT_JUPE_JOIN_IGNORE_CHANNEL);
int JupeJoinList::maxCount(DEFAULT_JUPE_JOIN_MAX_COUNT);
int JupeJoinList::maxTime(DEFAULT_JUPE_JOIN_MAX_TIME);
std::string JupeJoinList::reason(DEFAULT_JUPE_JOIN_REASON);


JupeJoinList jupeJoiners;


void
JupeJoinList::checkJupe(const std::string & nick,
  const JupeJoinList::JupeJoinEntry & entry) const
{
  if (entry.count > JupeJoinList::maxCount)
  {
    std::string msg;

    if (JupeJoinList::ignoreChannel)
    {
      msg = "Possible juped channel auto-joiner: " + nick + " (" +
	entry.userhost + ")";
    }
    else
    {
      msg = "Possible juped channel (" + entry.channel + ") auto-joiner: " +
	nick + " (" + entry.userhost + ")";
    }
    ::SendAll(msg, UserFlags::OPER, WATCH_JUPES);
    Log::Write(msg);

    bool exempt(false);
    BotSock::Address ip(INADDR_NONE);
    UserEntryPtr find(users.findUser(nick, entry.userhost));
    if (find)
    {
      ip = find->getIP();
      exempt = find->getOper() || config.isExempt(find, Config::EXEMPT_JUPE) ||
        config.isOper(find);
    }
    else
    {
      // If we can't find the user in our userlist, we won't be able to find
      // its IP address either
      exempt = config.isExempt(entry.userhost, Config::EXEMPT_JUPE) ||
        config.isOper(entry.userhost);
    }

    if (!exempt)
    {
      Format reason;
      reason.setStringToken('n', nick);
      reason.setStringToken('u', entry.userhost);
      reason.setStringToken('i', BotSock::inet_ntoa(ip));
      reason.setStringToken('c', entry.channel);
      reason.setStringToken('r', entry.reason);

      doAction(nick, entry.userhost, ip, JupeJoinList::action,
          reason.format(JupeJoinList::reason), false);
    }
  }
}


void
JupeJoinList::add(const std::string & nick, const std::string & userhost,
  const std::string & channel, const std::string & reason,
  std::time_t now)
{
  bool foundEntry = false;

  for (std::list<JupeJoinEntry>::iterator pos = this->list.begin();
    pos != this->list.end(); ++pos)
  {
    if (server.same(pos->userhost, userhost) &&
        (JupeJoinList::ignoreChannel || server.same(pos->channel, channel)))
    {
      foundEntry = true;

      pos->channel = channel;
      pos->reason = reason;
  
      if ((pos->last + JupeJoinList::maxTime) < now)
      {
	// Old entry -- drop count to 1
        pos->count = 1;
      }
      else
      {
	// Current entry -- increase count
        pos->count++;
      }
      pos->last = now;
      
      this->checkJupe(nick, *pos);
    }
    else
    {
      if ((pos->last + JupeJoinList::maxTime) < now)
      {     
	this->list.erase(pos);
	--pos;
      }
    }
  }

/*
   If this is a new entry, then foundEntry will still be false
*/
  if (!foundEntry)
  {
    JupeJoinEntry pos;

    pos.userhost = userhost;
    pos.channel = channel;
    pos.reason = reason;
    pos.last = now;
    pos.count = 1;

    this->list.push_back(pos);

    this->checkJupe(nick, pos);
  }
}


bool
JupeJoinList::onNotice(const std::string & notice)
{
  std::string text = notice;

  // User ToastTEst (toast@Plasma.Toast.PC) is attempting to join locally juped channel #jupedchan

  if (0 != server.downCase(FirstWord(text)).compare("user"))
    return false;

  std::string nick = FirstWord(text);
  std::string userhost = FirstWord(text);

  if (userhost.length() >= 2)
  {
    userhost.erase(userhost.begin());
    userhost.erase(userhost.end() - 1);
  }

  if (0 != server.downCase(FirstWord(text)).compare("is"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("attempting"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("to"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("join"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("locally"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("juped"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("channel"))
    return false;

  std::string channel = FirstWord(text);

  std::string reason(text);
  if (reason.length() > 2)
  {
    reason.erase(reason.begin());
    reason.erase(reason.end() - 1);
  }

  ::SendAll("*** " + notice, UserFlags::OPER, WATCH_JUPES);
  Log::Write(notice);

  this->add(nick, userhost, channel, reason, std::time(NULL));

  return true;
}


void
JupeJoinList::status(BotClient * client) const
{
  client->send("Juped channel joiners: " +
    boost::lexical_cast<std::string>(this->list.size()));
}


void
JupeJoinList::init(void)
{
  vars.insert("JUPE_JOIN_ACTION",
      AutoAction::Setting(JupeJoinList::action));
  vars.insert("JUPE_JOIN_IGNORE_CHANNEL",
      Setting::BooleanSetting(JupeJoinList::ignoreChannel));
  vars.insert("JUPE_JOIN_MAX_COUNT",
      Setting::IntegerSetting(JupeJoinList::maxCount, 0));
  vars.insert("JUPE_JOIN_MAX_TIME",
      Setting::IntegerSetting(JupeJoinList::maxTime, 0));
  vars.insert("JUPE_JOIN_REASON",
      Setting::StringSetting(JupeJoinList::reason));
}

