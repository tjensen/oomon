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

// OOMon Headers
#include "flood.h"
#include "config.h"
#include "util.h"
#include "main.h"
#include "irc.h"
#include "log.h"
#include "vars.h"
#include "userhash.h"


void
FloodList::addFlood(const std::string & nick, const std::string & userhost,
  std::time_t now, bool local)
{
  bool foundEntry = false;

  for (std::list<FloodEntry>::iterator pos = this->list.begin();
    pos != this->list.end(); ++pos)
  {
    if (pos->userhost == userhost)
    {
      foundEntry = true;
  
      /* if its an old old entry, let it drop to 0, then start counting
         (this should be very unlikely case)
       */
      if ((pos->last + this->getMaxTime()) < now)
      {
        pos->count = 0;
      }

      pos->count++;
      pos->last = now;
      
      if (pos->count >= this->getMaxCount())
      {
	std::string msg("Possible " + this->getType() + " flooder: " +
	  nick + " (" + userhost + ")");
        ::SendAll(msg, UserFlags::OPER);
	Log::Write(msg);

        bool excluded(false);
        BotSock::Address ip(INADDR_NONE);
        UserEntryPtr find(users.findUser(nick, userhost));
        if (find)
        {
          ip = find->getIP();
          excluded = config.isExcluded(find) ||
            config.isOper(userhost, find->getIP());
        }
        else
        {
          // If we can't find the user in our userlist, we won't be able to find
          // its IP address either
          excluded = config.isExcluded(userhost) || config.isOper(userhost);
        }

        if (local && !excluded)
        {
	  doAction(nick, userhost, ip, vars[this->getActionVar()]->getAction(),
	    vars[this->getActionVar()]->getInt(),
	    vars[this->getReasonVar()]->getString(), true);
        }
      }       
    }         
    else
    {         
      if ((pos->last + this->getMaxTime()) < now)
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
    FloodEntry pos;

    pos.userhost = userhost;
    pos.last = now;
    pos.count = 0;

    this->list.push_back(pos);
  }
}


bool
FloodList::onNotice(const std::string & notice, std::string text,
  std::time_t now)
{
  if (server.downCase(FirstWord(text)) != "requested")
  {
    // broken.  ignore it.
    return false;
  }

  if (server.downCase(FirstWord(text)) != "by")
  {
    // broken.  ignore it.
    return false;
  }

  std::string nick = FirstWord(text);
  std::string userhost = FirstWord(text);
  std::string serverName = FirstWord(text);

  if ((nick == "") || (userhost == ""))
  {
    // broken.  ignore it.
    return false;
  }

  // Remove brackets/parentheses from userhost
  if ((userhost.length() > 0) &&
    ((userhost[0] == '[') && (userhost[userhost.length() - 1] == ']')) ||
    ((userhost[0] == '(') && (userhost[userhost.length() - 1] == ')')))
  {
    userhost = server.downCase(userhost.substr(1, userhost.length() - 2));
  }

  // Remove brackets/parentheses from serverName
  if ((serverName.length() > 0) &&
    ((serverName[0] == '[') && (serverName[serverName.length() - 1] == ']')) ||
    ((serverName[0] == '(') && (serverName[serverName.length() - 1] == ')')))
  {
    serverName = server.downCase(serverName.substr(1, serverName.length() - 2));
  }

  /* Don't complain about opers */
  if (!config.isOper(userhost, users.getIP(nick, userhost)) &&
    !users.isOper(nick, userhost))
  {
    ::SendAll(notice, UserFlags::OPER, this->getWatch());

    this->addFlood(nick, userhost, now, server.same(serverName,
      server.getServerName()));
  }

  return true;
}

