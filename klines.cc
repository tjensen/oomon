// ===========================================================================
// OOMon - Objected Oriented Monitor Bot
// Copyright (C) 2003  Timothy L. Jensen
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

// C++ headers
#include <iostream>
#include <string>
#include <list>
#include <algorithm>

#include "strtype"
#include "klines.h"
#include "irc.h"
#include "util.h"
#include "main.h"
#include "log.h"
#include "vars.h"
#include "pattern.h"


#ifdef DEBUG
# define KLINES_DEBUG
#endif


void KlineList::Clear()
{
  Klines.clear();
}

void KlineList::Add(const std::string & userhost, const std::string & reason,
  const bool temporary)
{
  if (userhost != "")
  {
    if (Klines.end() != std::find(Klines.begin(), Klines.end(),
      KlineItem(userhost)))
    {
      // If the k-line already exists, we want to replace it!
      this->Remove(userhost);
    }

#ifdef KLINES_DEBUG
    std::cout << std::string("Adding ") + lineType + "-line: " << userhost <<
      " (" << reason << ')' << std::endl;
#endif

    Klines.push_back(KlineItem(userhost, reason, temporary));
  }
}


void
KlineList::find(StrList & Output, const Pattern *pattern, const bool count,
  const bool searchPerms, const bool searchTemps, const bool searchReason) const
{
  int matches = 0;
  Output.clear();
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
          Output.push_back("  " + pos->getUserhost() + " (" +
	    pos->getReason() + ')');
        }
        matches++;
      }
    }
  }
  Output.push_back(std::string("End of FIND") + lineType + " " +
    pattern->get() + " (" + IntToStr(Klines.size()) + " " + lineType +
    "-lines, " + IntToStr(matches) + " matches)");
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
void
KlineList::ParseAndAdd(std::string Text)
{
  std::string who = FirstWord(Text); // kline adder

  if (!Same("added", FirstWord(Text)))
    return;

  bool temporary = false;
  std::string time;

  if (Same("temporary", FirstWord(Text)))
  {
    temporary = true;

    time = FirstWord(Text);

    if (!Same("min.", FirstWord(Text)))
      return;

    std::string type = FirstWord(Text);

    if ((lineType == 'K') && !Same("K-Line", type))
      return;
    else if ((lineType == 'D') && !Same("D-Line", type))
      return;
  }

  if (!Same("for", FirstWord(Text)))
    return;

  // This won't work all the time, since a badly entered kline might
  // contain spaces. *shrug*
  std::string UserHost = FirstWord(Text);
  std::string Reason = Text;

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
    ::SendAll(msg, UF_OPER, WATCH_KLINES, NULL);
  }
  else if (lineType == 'D')
  {
    ::SendAll(msg, UF_OPER, WATCH_DLINES, NULL);
  }
  else
  {
    ::SendAll(msg, UF_OPER);
  }

  if (vars[VAR_EXTRA_KLINE_INFO]->getBool())
  {
    if (temporary)
    {
      Reason = std::string("Temporary ") + lineType + "-line " + time +
	" min. - " + Reason;
    }
    Reason = Reason + " (" + ::timeStamp(TIMESTAMP_KLINE) + ')';
  }

  if (!temporary ||
    ((lineType == 'K') && vars[VAR_TRACK_TEMP_KLINES]->getBool()) ||
    ((lineType == 'D') && vars[VAR_TRACK_TEMP_DLINES]->getBool()))
  {
    Add(UserHost, Reason, temporary);
  }
}


// Temporary K-line for [*chalupa@*.palupa] expired
void
KlineList::onExpireNotice(std::string text)
{
  if (!Same("Temporary", FirstWord(text)))
    return;
  if ((lineType == 'K') && !Same("K-line", FirstWord(text)))
    return;
  if ((lineType == 'D') && !Same("D-line", FirstWord(text)))
    return;
  if (!Same("for", FirstWord(text)))
    return;

  std::string mask = FirstWord(text);
  if (mask.length() > 2)
  {
    mask = mask.substr(1, mask.length() - 2);
  }

  if (!Same("expired", FirstWord(text)))
    return;

  Remove(mask);
}


// Toast has removed the K-Line for: [*@*.monkeys.org] (1 matches)
void
KlineList::ParseAndRemove(std::string Text)
{
  std::string who = FirstWord(Text); // kline adder

  if (!Same("has", FirstWord(Text)))
    return;
  if (!Same("removed", FirstWord(Text)))
    return;
  if (!Same("the", FirstWord(Text)))
    return;

  std::string temp = FirstWord(Text);
  bool temporary = false;

  if (Same("temporary", temp))
  {
    temporary = true;

    if (lineType == 'K')
    {
      if (!Same("K-Line", FirstWord(Text)))
        return;
    }
    else if (lineType == 'D')
    {
      if (!Same("D-Line", FirstWord(Text)))
        return;
    }
  }
  else if ((lineType == 'K') && !Same("K-Line", temp))
    return;
  else if ((lineType == 'D') && !Same("D-Line", temp))
    return;

  if (!Same("for:", FirstWord(Text)))
    return;

  // Again, spaces in the kline mask will screw this up.
  std::string UserHost = FirstWord(Text);

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
    ::SendAll(msg, UF_OPER, WATCH_KLINES, NULL);
  }
  else if (lineType == 'D')
  {
    ::SendAll(msg, UF_OPER, WATCH_DLINES, NULL);
  }
  else
  {
    ::SendAll(msg, UF_OPER);
  }

  if (!temporary ||
    ((lineType == 'K') && vars[VAR_TRACK_TEMP_KLINES]->getBool()) ||
    ((lineType == 'D') && vars[VAR_TRACK_TEMP_DLINES]->getBool()))
  {
    Remove(UserHost);
  }
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

