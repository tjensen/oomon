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

// Std C++ Headers
#include <string>

// Std C Headers
#include <time.h>

// OOMon Headers
#include "strtype"
#include "jupe.h"
#include "util.h"
#include "main.h"
#include "log.h"
#include "vars.h"
#include "userhash.h"


JupeJoinList jupeJoiners;


void
JupeJoinList::checkJupe(const std::string & nick,
  const JupeJoinList::JupeJoinEntry & entry) const
{
  if (entry.count > vars[VAR_JUPE_JOIN_MAX_COUNT]->getInt())
  {
    std::string msg;

    if (vars[VAR_JUPE_JOIN_IGNORE_CHANNEL]->getBool())
    {
      msg = "Possible juped channel auto-joiner: " + nick + " (" +
	entry.userhost + ")";
    }
    else
    {
      msg = "Possible juped channel (" + entry.channel + ") auto-joiner: " +
	nick + " (" + entry.userhost + ")";
    }
    ::SendAll(msg, UF_OPER);
    Log::Write(msg);

    BotSock::Address ip = users.getIP(nick, entry.userhost);

    if (!Config::IsOper(entry.userhost, ip) &&
      !Config::IsOKHost(entry.userhost, ip))
    {
      doAction(nick, entry.userhost, ip,
        vars[VAR_JUPE_JOIN_ACTION]->getAction(),
        vars[VAR_JUPE_JOIN_ACTION]->getInt(),
        vars[VAR_JUPE_JOIN_REASON]->getString(), false);
    }
  }
}


void
JupeJoinList::add(const std::string & nick, const std::string & userhost,
  const std::string & channel, time_t now)
{
  bool foundEntry = false;

  for (std::list<JupeJoinEntry>::iterator pos = this->list.begin();
    pos != this->list.end(); ++pos)
  {
    if (Same(pos->userhost, userhost) &&
      (vars[VAR_JUPE_JOIN_IGNORE_CHANNEL]->getBool() ||
      Same(pos->channel, channel)))
    {
      foundEntry = true;
  
      if ((pos->last + vars[VAR_JUPE_JOIN_MAX_TIME]->getInt()) < now)
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
      if ((pos->last + vars[VAR_JUPE_JOIN_MAX_TIME]->getInt()) < now)
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
    pos.last = now;
    pos.count = 1;

    this->list.push_back(pos);

    this->checkJupe(nick, pos);
  }
}


void
JupeJoinList::onNotice(const std::string & notice)
{
  std::string text = notice;

  // User ToastTEst (toast@Plasma.Toast.PC) is attempting to join locally juped channel #jupedchan

  if (!Same("User", FirstWord(text)))
    return;

  std::string nick = FirstWord(text);
  std::string userhost = FirstWord(text);

  if (userhost.length() >= 2)
  {
    userhost = userhost.substr(1, userhost.length() - 2);
  }

  if (!Same("is", FirstWord(text)))
    return;
  if (!Same("attempting", FirstWord(text)))
    return;
  if (!Same("to", FirstWord(text)))
    return;
  if (!Same("join", FirstWord(text)))
    return;
  if (!Same("locally", FirstWord(text)))
    return;
  if (!Same("juped", FirstWord(text)))
    return;
  if (!Same("channel", FirstWord(text)))
    return;

  std::string channel = FirstWord(text);

  ::SendAll("*** " + notice, UF_OPER, WATCH_JUPES);
  Log::Write(notice);

  this->add(nick, userhost, channel, time(NULL));
}


void
JupeJoinList::status(StrList & output) const
{
  output.push_back("Juped channel joiners: " + IntToStr(this->list.size()));
}

