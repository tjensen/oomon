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
#include <iostream>
#include <string>
#include <list>
#include <algorithm>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

#include "strtype"
#include "klines.h"
#include "irc.h"
#include "util.h"
#include "main.h"
#include "log.h"
#include "vars.h"
#include "pattern.h"
#include "botclient.h"
#include "defaults.h"


#ifdef DEBUG
# define KLINES_DEBUG
#endif


bool KlineList::extraInfo(DEFAULT_EXTRA_KLINE_INFO);


void KlineList::Clear()
{
  Klines.clear();
}

void KlineList::Add(const std::string & userhost, const std::string & reason,
  const bool temporary)
{
  if (!userhost.empty())
  {
#ifdef KLINES_DEBUG
    std::cout << std::string("Adding ") + lineType + "-line: " << userhost <<
      " (" << reason << ')' << std::endl;
#endif

    Klines.push_back(KlineItem(userhost, reason, temporary));
  }
}


void
KlineList::find(BotClient * client, const Pattern *pattern, const bool count,
  const bool searchPerms, const bool searchTemps, const bool searchReason) const
{
  int matches = 0;
  for (std::list<KlineItem>::const_iterator pos = Klines.begin();
    pos != Klines.end(); ++pos)
  {
    if (searchReason ? pattern->match(pos->getReason()) :
      pattern->match(pos->getUserhost()))
    {
      if ((searchPerms && !pos->isTemporary()) ||
	(searchTemps && pos->isTemporary()))
      {
        if (!count)
        {
          client->send("  " + pos->getUserhost() + " (" + pos->getReason() +
	    ')');
        }
        matches++;
      }
    }
  }
  client->send(std::string("End of FIND") + lineType + " " + pattern->get() +
    " (" + boost::lexical_cast<std::string>(Klines.size()) + " " + lineType +
    "-lines, " + boost::lexical_cast<std::string>(matches) + " matches)");
}


int
KlineList::findAndRemove(const std::string & from, const Pattern *pattern,
  const bool searchPerms, const bool searchTemps, const bool searchReason)
{
  int matches = 0;

  for (std::list<KlineItem>::iterator pos = Klines.begin();
    pos != Klines.end(); ++pos)
  {
    if (searchReason ? pattern->match(pos->getReason()) :
      pattern->match(pos->getUserhost()))
    {
      if ((searchPerms && !pos->isTemporary()) ||
	(searchTemps && pos->isTemporary()))
      {
        if (lineType == 'K')
        {
          server.unkline(from, pos->getUserhost());
        }
        else if (lineType == 'D')
        {
          server.undline(from, pos->getUserhost());
        }
        matches++;
      }
    }
  }

  return matches;
}


void KlineList::Remove(const std::string & userhost)
{
  KlineItem removeMe(userhost);

#ifdef KLINES_DEBUG
  std::cout << std::string("Removing ") + lineType + "-line: " << userhost <<
    std::endl;
#endif

  Klines.remove(removeMe);
}

// LarzMON added K-Line for [*khknkfg@*.sprint.ca] [Clones are prohibited [Toast@LarzMON]]
bool
KlineList::parseAndAdd(std::string text)
{
  std::string who = FirstWord(text); // kline adder

  if (0 != server.downCase(FirstWord(text)).compare("added"))
    return false;

  bool temporary = false;
  std::string time;

  if (0 == server.downCase(FirstWord(text)).compare("temporary"))
  {
    temporary = true;

    time = FirstWord(text);

    if (0 != server.downCase(FirstWord(text)).compare("min."))
      return false;

    std::string type = FirstWord(text);

    if ((lineType == 'K') && (0 != server.downCase(type).compare("k-line")))
      return false;
    else if ((lineType == 'D') &&
      (0 != server.downCase(type).compare("d-line")))
      return false;
  }

  if (0 != server.downCase(FirstWord(text)).compare("for"))
    return false;

  // This won't work all the time, since a badly entered kline might
  // contain spaces. *shrug*
  std::string UserHost = FirstWord(text);
  std::string Reason = text;

  // Remove brackets
  UserHost.erase((std::string::size_type) 0, 1);
  UserHost.erase(UserHost.length() - 1, 1);
  Reason.erase((std::string::size_type) 0, 1);
  Reason.erase(Reason.length() - 1, 1);

  std::string msg;
  if (temporary)
  {
    msg = getNick(who) + " added " + time + " min. " + lineType + "-Line: " +
      UserHost + " (" + Reason + ")";
  }
  else
  {
    msg = getNick(who) + " added " + lineType + "-Line: " + UserHost + " (" +
      Reason + ")";
  }

  Log::Write(msg);
  if (lineType == 'K')
  {
    ::SendAll(msg, UserFlags::OPER, WATCH_KLINES);
  }
  else if (lineType == 'D')
  {
    ::SendAll(msg, UserFlags::OPER, WATCH_DLINES);
  }
  else
  {
    ::SendAll(msg, UserFlags::OPER);
  }

  if (KlineList::extraInfo)
  {
    if (temporary)
    {
      Reason = std::string("Temporary ") + lineType + "-line " + time +
	" min. - " + Reason;
    }
    Reason = Reason + " (" + ::timeStamp(TIMESTAMP_KLINE) + ')';
  }

  if (!temporary || ((lineType == 'K') && IRC::trackTempKlines()) ||
      ((lineType == 'D') && IRC::trackTempDlines()))
  {
    Add(UserHost, Reason, temporary);
  }

  return true;
}


// Temporary K-line for [*chalupa@*.palupa] expired
bool
KlineList::onExpireNotice(std::string text)
{
  if (0 != server.downCase(FirstWord(text)).compare("temporary"))
    return false;
  if ((lineType == 'K') &&
    (0 != server.downCase(FirstWord(text)).compare("k-line")))
    return false;
  if ((lineType == 'D') &&
    (0 != server.downCase(FirstWord(text)).compare("d-line")))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("for"))
    return false;

  std::string mask = FirstWord(text);
  if (mask.length() > 2)
  {
    mask = mask.substr(1, mask.length() - 2);
  }

  if (0 != server.downCase(FirstWord(text)).compare("expired"))
    return false;

  Remove(mask);

  return true;
}


// Toast has removed the K-Line for: [*@*.monkeys.org] (1 matches)
bool
KlineList::parseAndRemove(std::string text)
{
  std::string who = FirstWord(text); // kline adder

  if (0 != server.downCase(FirstWord(text)).compare("has"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("removed"))
    return false;
  if (0 != server.downCase(FirstWord(text)).compare("the"))
    return false;

  std::string temp = FirstWord(text);
  bool temporary = false;

  if (0 == server.downCase(temp).compare("temporary"))
  {
    temporary = true;

    if (lineType == 'K')
    {
      if (0 != server.downCase(FirstWord(text)).compare("k-line"))
        return false;
    }
    else if (lineType == 'D')
    {
      if (0 != server.downCase(FirstWord(text)).compare("d-line"))
        return false;
    }
  }
  else if ((lineType == 'K') && (0 != server.downCase(temp).compare("k-line")))
    return false;
  else if ((lineType == 'D') && (0 != server.downCase(temp).compare("d-line")))
    return false;

  if (0 != server.downCase(FirstWord(text)).compare("for:"))
    return false;

  // Again, spaces in the kline mask will screw this up.
  std::string UserHost = FirstWord(text);

  // Remove brackets
  UserHost.erase((std::string::size_type) 0, 1);
  UserHost.erase(UserHost.length() - 1, 1);

  std::string msg;
  if (temporary)
  {
    msg = getNick(who) + " removed temp. " + lineType + "-Line: " +
      UserHost;
  }
  else
  {
    msg = getNick(who) + " removed " + lineType + "-Line: " + UserHost;
  }

  Log::Write(msg);
  if (lineType == 'K')
  {
    ::SendAll(msg, UserFlags::OPER, WATCH_KLINES);
  }
  else if (lineType == 'D')
  {
    ::SendAll(msg, UserFlags::OPER, WATCH_DLINES);
  }
  else
  {
    ::SendAll(msg, UserFlags::OPER);
  }

  if (!temporary || ((lineType == 'K') && IRC::trackTempKlines()) ||
      ((lineType == 'D') && IRC::trackTempDlines()))
  {
    Remove(UserHost);
  }

  return true;
}


KlineList::size_type
KlineList::permSize() const
{
  KlineList::size_type count = 0;

  for (std::list<KlineItem>::const_iterator pos = Klines.begin();
    pos != Klines.end(); ++pos)
  {
    if (!pos->isTemporary())
    {
      ++count;
    }
  }
  return count;
}


void
KlineList::init(void)
{
  vars.insert("EXTRA_KLINE_INFO",
      Setting::BooleanSetting(KlineList::extraInfo));
}

