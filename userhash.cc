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
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <algorithm>

// Std C Headers
#include <stdio.h>
#include <time.h>

// OOMon Headers
#include "strtype"
#include "userhash.h"
#include "botsock.h"
#include "util.h"
#include "config.h"
#include "vars.h"
#include "autoaction.h"
#include "seedrand.h"
#include "trap.h"
#include "main.h"
#include "log.h"
#include "engine.h"
#include "pattern.h"
#include "botclient.h"


const static int CLONE_DETECT_INC = 15;


UserHash users;


int
UserHash::hashFunc(const std::string & key)
{
  std::string::size_type len = key.length();
  int i = 0;

  if (len > 0)
  {
    i = server.downCase(key[0]);

    if (len > 1)
    {
      i |= (server.downCase(key[1]) << 8);

      if (len > 2)
      {
        i |= (server.downCase(key[2]) << 16);

	if (len > 3)
	{
          i |= (server.downCase(key[3]) << 24);
	}
      }
    }
  }
  return i % HASHTABLESIZE;
}


int
UserHash::hashFunc(const BotSock::Address & key)
{
  return (key & BotSock::ClassCNetMask) % HASHTABLESIZE;
}


void
UserHash::add(const std::string & nick, const std::string & userhost,
  const std::string & ip, bool fromTrace, bool isOper,
  const std::string & userClass, const std::string & gecos)
{
  // Find '@' in user@host
  std::string::size_type at = userhost.find('@');

  if (std::string::npos != at)
  {
    // Get username and hostname from user@host
    std::string user = userhost.substr(0, at);
    std::string host = userhost.substr(at + 1);

    if (!checkForSpoof(nick, user, host, ip))
    {
      // If we made it this far, we'll just assume it isn't a spoofed
      // hostname.  Oh, happy day!

      UserEntry *newuser = new UserEntry(nick, user, host, userClass, gecos,
	(ip.length() > 0) ? BotSock::inet_addr(ip) : INADDR_NONE, 
	(fromTrace ? 0 : time(NULL)), isOper);

      if (newuser == NULL)
      {
        ::SendAll("Ran out of memory in UserHash::add()", UserFlags::OPER);
        Log::Write("Ran out of memory in UserHash::add()");
        gracefuldie(SIGABRT);
      }

      // Add it to the hash tables
      UserHash::addToHash(this->hosttable, newuser->getHost(), newuser);
      UserHash::addToHash(this->domaintable, newuser->getDomain(), newuser);
      UserHash::addToHash(this->usertable, newuser->getUser(), newuser);
      UserHash::addToHash(this->iptable, newuser->getIp(), newuser);
      this->userCount++;

      // Don't check for wingate or clones when doing a TRACE
      if (!fromTrace)
      {
        if (!Config::IsOKHost(userhost, ip) && !Config::IsOper(userhost, ip))
        {
          // Check if this client matches any traps
          if (vars[VAR_TRAP_CONNECTS]->getBool())
          {
            TrapList::match(nick, userhost, ip, gecos);
          }

          if (newuser->getScore() >= vars[VAR_SEEDRAND_REPORT_MIN]->getInt())
          {
	    std::string notice("Random (score: " +
	      IntToStr(newuser->getScore()) + ") nick connect: " + nick +
	      " (" + userhost + ")");
	    if ("" != ip)
	    {
	      notice = notice + " [" + ip + "]";
	    }

	    ::SendAll(notice, UserFlags::OPER, WATCH_SEEDRAND);
	    Log::Write(notice);

	    doAction(nick, userhost, BotSock::inet_addr(ip),
	      vars[VAR_SEEDRAND_ACTION]->getAction(),
	      vars[VAR_SEEDRAND_ACTION]->getInt(),
	      vars[VAR_SEEDRAND_REASON]->getString(), false);
          }

          if (ip != "")
          {
            CheckProxy(ip, host, nick, userhost);
          }

	  BotSock::Address ipAddr = BotSock::inet_addr(ip);

          // Clonebot check
	  if (INADDR_NONE == ipAddr)
	  {
            this->checkHostClones(host);
	  }
	  else
	  {
            this->checkIpClones(ipAddr);
	  }
        }
      }
    }
  }
}


void
UserHash::updateOper(const std::string & nick, const std::string & userhost,
  bool isOper)
{
  std::string::size_type at = userhost.find('@');

  if (std::string::npos != at)
  {
    std::string user = server.downCase(userhost.substr(0, at));
    std::string host = server.downCase(userhost.substr(at + 1));

    int index = UserHash::hashFunc(user);

    UserHash::HashRec *find = this->usertable[index];

    while (NULL != find)
    {
      if ((0 == nick.compare(find->info->getNick())) &&
	(0 == user.compare(server.downCase(find->info->getUser()))) &&
	(vars[VAR_BROKEN_HOSTNAME_MUNGING]->getBool() ||
	(0 == host.compare(server.downCase(find->info->getHost())))))
      {
        find->info->setOper(true);

        // There shouldn't be any more, so just return
        return;
      }
      find = find->collision;
    }
  }
}


void
UserHash::updateNick(const std::string & oldNick, const std::string & userhost,
  const std::string & newNick)
{
  std::string::size_type at = userhost.find('@');

  if (std::string::npos != at)
  {
    std::string user = userhost.substr(0, at);

    int index = UserHash::hashFunc(user);

    UserHash::HashRec *find = this->usertable[index];

    while (NULL != find)
    {
      if (0 == oldNick.compare(find->info->getNick()))
      {
        find->info->setNick(newNick);

        if (vars[VAR_TRAP_NICK_CHANGES]->getBool())
        {
          TrapList::match(find->info->getNick(), userhost, find->info->getIp(),
	    find->info->getGecos());
        }

        if ((find->info->getScore() >=
	  vars[VAR_SEEDRAND_REPORT_MIN]->getInt()) &&
	  !Config::IsOper(userhost, find->info->getIp()) &&
	  !Config::IsOKHost(userhost, find->info->getIp()) &&
	  !find->info->getOper())
        {
	  std::string notice("Random (score: " +
	    IntToStr(find->info->getScore()) + ") nick change: " +
	    find->info->getNick() + " (" + userhost + ")");
	  if (INADDR_NONE != find->info->getIp())
	  {
	    notice = notice + " [" + BotSock::inet_ntoa(find->info->getIp()) +
	      "]";
	  }
	  ::SendAll(notice, UserFlags::OPER, WATCH_SEEDRAND);
	  Log::Write(notice);

	  doAction(find->info->getNick(), userhost, find->info->getIp(),
	    vars[VAR_SEEDRAND_ACTION]->getAction(),
	    vars[VAR_SEEDRAND_ACTION]->getInt(),
	    vars[VAR_SEEDRAND_REASON]->getString(), false);
        }

        // There shouldn't be any more, so just return
        return;
      }
      find = find->collision;
    }
  }
}


void
UserHash::remove(const std::string & nick, const std::string & userhost,
  const BotSock::Address & ip)
{
  std::string::size_type at = userhost.find('@');

  if (std::string::npos != at)
  {
    std::string user = userhost.substr(0, at);
    std::string host = userhost.substr(at + 1);
    std::string domain = getDomain(host, false);

    // Set this to true if we are unable to remove the entry from one or
    // more of the tables.
    bool error = false;

    if (vars[VAR_BROKEN_HOSTNAME_MUNGING]->getBool())
    {
      host = "";
      domain = "";
    }

    this->userCount--;

    if (!UserHash::removeFromHash(this->hosttable, host, host, user, nick))
    {
      if (!UserHash::removeFromHash(this->hosttable, host, host, user, ""))
      {
        error = true;

        std::cerr << "Error removing user from host table: " << nick << " (" <<
	  user << "@" << host << ") [" << BotSock::inet_ntoa(ip) << "]" <<
	  std::endl;
      }
    }

    if (!UserHash::removeFromHash(this->domaintable, domain, host, user, nick))
    {
      if (!UserHash::removeFromHash(this->domaintable, domain, host, user, ""))
      {
        error = true;

        std::cerr << "Error removing user from domain table: " << nick <<
	  " (" << user << "@" << host << ") [" << BotSock::inet_ntoa(ip) <<
	  "]" << std::endl;
      }
    }

    if (!UserHash::removeFromHash(this->usertable, user, host, user, nick))
    {
      if (!UserHash::removeFromHash(this->usertable, user, host, user, ""))
      {
	error = true;

        std::cerr << "Error removing user from user table: " << nick << " (" <<
	  user << "@" << host << ") [" << BotSock::inet_ntoa(ip) << "]" <<
	  std::endl;
      }
    }

    if (!UserHash::removeFromHash(this->iptable, ip, host, user, nick))
    {
      if (!UserHash::removeFromHash(this->iptable, ip, host, user, ""))
      {
	error = true;

        std::cerr << "Error removing user from IP table: " << nick << " (" <<
	  user << "@" << host << ") [" << BotSock::inet_ntoa(ip) << "]" <<
	  std::endl;
      }
    }

    // Any errors occur?
    if (error)
    {
      Log::Write("Error removing user from table(s): " + nick + " (" + user +
	"@" + host + ") [" + BotSock::inet_ntoa(ip) + "]");
    }
  }
}


void
UserHash::addToHash(HashRec *table[], const std::string & key, UserEntry *item)
{
  int index = UserHash::hashFunc(key);

  UserHash::HashRec *newhashrec = new UserHash::HashRec;

  if (newhashrec == NULL)
  {
    ::SendAll("Ran out of memory in UserHash::addToHash()", UserFlags::OPER);
    Log::Write("Ran out of memory in UserHash::addToHash()");
    gracefuldie(SIGABRT);
  }

  newhashrec->info = item;
  if (NULL != newhashrec->info)
  {
    newhashrec->info->incLinkCount();
  }
  newhashrec->collision = table[index];
  table[index] = newhashrec;
}


void
UserHash::addToHash(HashRec *table[], const BotSock::Address & key,
  UserEntry *item)
{
  int index = UserHash::hashFunc(key);

  UserHash::HashRec *newhashrec = new UserHash::HashRec;

  if (newhashrec == NULL)
  {
    ::SendAll("Ran out of memory in UserHash::addToHash()", UserFlags::OPER);
    Log::Write("Ran out of memory in UserHash::addToHash()");
    gracefuldie(SIGABRT);
  }

  newhashrec->info = item;
  if (NULL != newhashrec->info)
  {
    newhashrec->info->incLinkCount();
  }
  newhashrec->collision = table[index];
  table[index] = newhashrec;
}


bool
UserHash::removeFromHashEntry(HashRec *table[], const int index,
  const std::string & host, const std::string & user, const std::string & nick)
{
  UserHash::HashRec *find = table[index];

  UserHash::HashRec *prev = NULL;

  while (find)
  {
    if ((host.empty() || (server.same(find->info->getHost(), host))) &&
      (user.empty() || (server.same(find->info->getUser(), user))) &&
      (nick.empty() || (server.same(find->info->getNick(), nick))))
    {
      if (prev)
	prev->collision = find->collision;
      else
	table[index] = find->collision;

      if (find->info->getLinkCount() > 0)
	find->info->decLinkCount();

      if (find->info->getLinkCount() == 0)
      {
	delete find->info;
      }
      delete find;
      return true;
    }
    prev = find;
    find = find->collision;
  }
  return false;
}


bool
UserHash::removeFromHash(HashRec *table[], const std::string & key,
  const std::string & host, const std::string & user, const std::string & nick)
{
  if (key.length() > 0)
  {
    int index = UserHash::hashFunc(key);

    return removeFromHashEntry(table, index, host, user, nick);
  }
  else
  {
    for (int index = 0; index < HASHTABLESIZE; ++index)
    {
      if (removeFromHashEntry(table, index, host, user, nick))
      {
	return true;
      }
    }
    return false;
  }
}


bool
UserHash::removeFromHash(HashRec *table[], const BotSock::Address & key,
  const std::string & host, const std::string & user, const std::string & nick)
{
  int index = UserHash::hashFunc(key);

  if (removeFromHashEntry(table, index, host, user, nick))
  {
    return true;
  }
  else
  {
    // Didn't find it, try it without an IP
    if (INADDR_NONE != key)
    {
      return removeFromHash(table, INADDR_NONE, host, user, nick);
    }
  }

  return false;
}


void
UserHash::clearHash(HashRec *table[])
{
  for (int i = 0; i < HASHTABLESIZE; ++i)
  {
    HashRec *hp = table[i];
    while (hp)
    {
      HashRec *next_hp = hp->collision;

      if (hp->info->getLinkCount() > 0)
      {
        hp->info->decLinkCount();
      }

      if(hp->info->getLinkCount() == 0)
      {
        delete hp->info;
      }
  
      delete hp;
      hp = next_hp;
    } 
    table[i] = NULL;
  }
}


void
UserHash::clear()
{
  UserHash::clearHash(this->usertable);
  UserHash::clearHash(this->hosttable);
  UserHash::clearHash(this->domaintable);
  UserHash::clearHash(this->iptable);
  this->userCount = this->previousCount = 0;
}


int
UserHash::listUsers(BotClient * client, const Pattern *userhost,
  std::string className, const ListAction action, const std::string & from,
  const std::string & reason) const
{
  int numfound = 0;

  if ((0 == userhost->get().compare("*")) ||
    (0 == userhost->get().compare("*@*")))
  {
    client->send("Listing all users is not recommended. To do it anyway,");
    client->send("use 'list ?*@*'.");
  }
  else
  {
    if (action == UserHash::LIST_KILL)
    {
      Log::Write("KILLLIST " + userhost->get() + " (" + reason + ") [" +
	from + "]");
    }

    // Convert our class name to uppercase now
    className = server.downCase(className);

    for (int index = 0; index < HASHTABLESIZE; ++index)
    {
      UserHash::HashRec *userptr = this->domaintable[index];

      while (userptr)
      {
	bool matchesUH = userhost->match(userptr->info->getUserHost());
	bool matchesUIP = false;
	if (INADDR_NONE != userptr->info->getIp())
	{
	  matchesUIP = userhost->match(userptr->info->getUserIP());
	}
	if ((matchesUH || matchesUIP) && ((0 == className.length()) ||
	  (0 == className.compare(userptr->info->getClass()))))
	{
	  ++numfound;
	  std::string outmsg(userptr->info->output(vars[VAR_LIST_FORMAT]->getString()));
	  if (action == UserHash::LIST_KILL)
	  {
	    server.kill(from, userptr->info->getNick(), reason);
	  }
	  if (action == UserHash::LIST_VIEW)
	  {
	    client->send(outmsg);
	  }
	}
	userptr = userptr->collision;
      }
    }

    std::string outmsg;
    if (numfound > 0)
    {
      outmsg = IntToStr(numfound) + " matches for " + userhost->get() +
	" found.";
    }
    else
    {
      outmsg = "No matches for " + userhost->get() + " found.";
    }
    client->send(outmsg);
  }

  return numfound;
}


int
UserHash::listNicks(BotClient * client, const Pattern *nick,
  std::string className, const ListAction action, const std::string & from,
  const std::string & reason) const
{
  int numfound = 0;

  if (action == UserHash::LIST_KILL)
  {
    Log::Write("KILLNFIND " + nick->get() + " (" + reason + ") [" + from + "]");
  }

  // Convert class name to uppercase now
  className = server.downCase(className);

  for (int index = 0; index < HASHTABLESIZE; ++index)
  {
    UserHash::HashRec *userptr = this->domaintable[index];

    while (userptr)
    {
      if (nick->match(userptr->info->getNick()) && (className.empty() ||
	(0 == className.compare(userptr->info->getClass()))))
      {
        ++numfound;
	if (action == UserHash::LIST_KILL)
	{
	  server.kill(from, userptr->info->getNick(), reason);
	}
	if (action == UserHash::LIST_VIEW)
	{
	  client->send(userptr->info->output(vars[VAR_NFIND_FORMAT]->getString()));
	}
      }
      userptr = userptr->collision;
    }
  }

  std::string outmsg;
  if (numfound > 0)
  {
    outmsg = IntToStr(numfound) + " matches for " + nick->get() + " found.";
  }
  else
  {
    outmsg = "No matches for " + nick->get() + " found.";
  }
  client->send(outmsg);

  return numfound;
}


int
UserHash::listGecos(BotClient * client, const Pattern *gecos,
  std::string className, const bool count) const
{
  int numfound = 0;

  // Convert class name to uppercase now
  className = server.downCase(className);

  for (int index = 0; index < HASHTABLESIZE; ++index)
  {
    UserHash::HashRec *userptr = this->domaintable[index];

    while (userptr)
    {
      if (gecos->match(userptr->info->getGecos()) && (className.empty() ||
	(0 == className.compare(userptr->info->getClass()))))
      {
        if (!numfound++)
        {
	  if (!count)
	  {
            client->send("The following clients match " + gecos->get() + ":");
	  }
        }
	if (!count)
	{
          client->send(userptr->info->output(vars[VAR_GLIST_FORMAT]->getString()));
	}
      }
      userptr = userptr->collision;
    }
  }

  std::string outmsg;
  if (numfound > 0)
  {
    outmsg = IntToStr(numfound) + " matches for " + gecos->get() + " found.";
  }
  else
  {
    outmsg = "No matches for " + gecos->get() + " found.";
  }
  client->send(outmsg);

  return numfound;
}


bool
UserHash::have(std::string nick) const
{
  nick = server.downCase(nick);

  for (int i = 0; i < HASHTABLESIZE; ++i)
  {
    UserHash::HashRec *find = this->hosttable[i];

    while (find)
    {
      if (0 == server.downCase(find->info->getNick()).compare(nick))
      {
        return true;
      }
      find = find->collision;
    }
  }
  return false;
}


void
UserHash::reportClasses(BotClient * client, const std::string & className)
{
  typedef std::map<std::string,int> ClassType;
  ClassType Classes;

  for (int i = 0; i < HASHTABLESIZE; ++i)
  {
    UserHash::HashRec *userptr = this->domaintable[i];
    while (userptr)
    {
      Classes[server.downCase(userptr->info->getClass())]++;
      userptr = userptr->collision;
    }
  }

  char outmsg[MAX_BUFF];
  snprintf(outmsg, sizeof(outmsg), "%-10s %-6s %s", "Class", "Count",
    "Description");
  client->send(outmsg);

  for (ClassType::iterator pos = Classes.begin(); pos != Classes.end();
    ++pos)
  {
    if (className.empty() ||
      (0 == pos->first.compare(server.downCase(className))))
    {
      snprintf(outmsg, sizeof(outmsg), "%-10s %-6d %s", pos->first.c_str(),
	pos->second, Config::GetYLineDescription(pos->first).c_str());
      client->send(outmsg);
    }
  }
}


void
UserHash::reportSeedrand(BotClient * client, const Pattern *mask,
  const int threshold, const bool count) const
{
  if (!count)
  {
    client->send("Searching " + mask->get() + ".  Threshold is " +
      IntToStr(threshold));
  }

  std::list<UserHash::ScoreNode> scores;

  for (int temp = 0; temp < HASHTABLESIZE; ++temp)
  {
    for (UserHash::HashRec *find = this->usertable[temp]; find;
      find = find->collision)
    {
      if (mask->match(find->info->getNick()))
      {
	if (find->info->getScore() >= threshold)
        {
	  UserHash::ScoreNode tmp(find->info, find->info->getScore());
	  scores.push_back(tmp);
        }
      }
    }
  }

  if (!count)
  {
    scores.sort();

    for (std::list<UserHash::ScoreNode>::iterator pos = scores.begin();
      pos != scores.end(); ++pos)
    {
      UserEntry *info = pos->getInfo();

      client->send(info->output(vars[VAR_SEEDRAND_FORMAT]->getString()));
    }
  }

  if (0 == scores.size())
  {
    client->send("No matches (score >= " + IntToStr(threshold) + ") for " +
      mask->get() + " found.");
  }
  else if (1 == scores.size())
  {
    client->send("1 match (score >= " + IntToStr(threshold) + ") for " +
      mask->get() + " found.");
  }
  else
  {
    client->send(IntToStr(scores.size()) + " matches (score >= " +
      IntToStr(threshold) + ") for " + mask->get() + " found.");
  }
}


void
UserHash::reportDomains(BotClient * client, const int num)
{
  std::list<UserHash::SortEntry> sort;
  int maxCount = 1;
  int foundany = 0;

  if (num < 1)
  {
    client->send("*** Parameter must be greater than 0!");
    return;
  }

  for (int i = 0; i < HASHTABLESIZE; ++i)
  {
    HashRec *userptr = this->hosttable[i];

    while (NULL != userptr)
    {
      bool found = false;

      for (std::list<UserHash::SortEntry>::iterator pos = sort.begin();
	pos != sort.end(); ++pos)
      {
        if (server.same(userptr->info->getDomain(), pos->rec->getDomain()))
	{
	  found = true;
          if (++(pos->count) > maxCount)
	  {
	    maxCount = pos->count;
	  }
	  break;
	}
      }
      if (!found)
      {
	UserHash::SortEntry newEntry;

        newEntry.rec = userptr->info;
        newEntry.count = 1;

	sort.push_back(newEntry);
      }

      userptr = userptr->collision;
    }
  }

  /* Print 'em out from highest to lowest */
  while (maxCount >= num)
  {
    int currentCount = maxCount;

    maxCount = 0;

    for (std::list<UserHash::SortEntry>::iterator pos = sort.begin();
      pos != sort.end(); ++pos)
    {
      if (pos->count >= currentCount)
      {
        if (!foundany++)
	{
          client->send("Domains with most users on the server:");
	}

        char outmsg[MAX_BUFF];            
        sprintf(outmsg, "  %-40s %3d user%s", pos->rec->getDomain().c_str(),
	  pos->count, (pos->count == 1) ? "" : "s");

        client->send(outmsg);

	sort.erase(pos);
	--pos;
      }
      else if (pos->count >= maxCount)
      {
	maxCount = pos->count;
      }
    }
  }

  if (!foundany)
  {  
    char outmsg[MAX_BUFF];            
    sprintf(outmsg, "No domains have %d or more users.", num);
    client->send(outmsg);
  }
}


void
UserHash::reportNets(BotClient * client, const int num)
{
  std::list<UserHash::SortEntry> sort;
  int maxCount = 1;
  int foundany = 0;

  if (num < 1)
  {
    client->send("*** Parameter must be greater than 0!");
    return;
  }

  for (int i = 0; i < HASHTABLESIZE; ++i)
  {
    HashRec *userptr = this->hosttable[i];

    while (NULL != userptr)
    {
      if (userptr->info->getIp() != INADDR_NONE)
      {
        bool found = false;

        for (std::list<UserHash::SortEntry>::iterator pos = sort.begin();
	  pos != sort.end(); ++pos)
        {
          if ((userptr->info->getIp() & BotSock::ClassCNetMask) ==
	    (pos->rec->getIp() & BotSock::ClassCNetMask))
	  {
	    found = true;
            if (++(pos->count) > maxCount)
	    {
	      maxCount = pos->count;
	    }
	    break;
	  }
        }
        if (!found)
        {
	  UserHash::SortEntry newEntry;

          newEntry.rec = userptr->info;
          newEntry.count = 1;

	  sort.push_back(newEntry);
        }
      }

      userptr = userptr->collision;
    }
  }

  /* Print 'em out from highest to lowest */
  while (maxCount >= num)
  {
    int currentCount = maxCount;

    maxCount = 0;

    for (std::list<UserHash::SortEntry>::iterator pos = sort.begin();
      pos != sort.end(); ++pos)
    {
      if (pos->count >= currentCount)
      {
        if (!foundany++)
	{
          client->send("Nets with most users on the server:");
	}

        char outmsg[MAX_BUFF];            
        sprintf(outmsg, "  %-40s %3d user%s",
	  classCMask(BotSock::inet_ntoa(pos->rec->getIp())).c_str(),
	  pos->count, (pos->count == 1) ? "" : "s");

        client->send(outmsg);

	sort.erase(pos);
	--pos;
      }
      else if (pos->count >= maxCount)
      {
	maxCount = pos->count;
      }
    }
  }

  if (!foundany)
  {  
    char outmsg[MAX_BUFF];            
    sprintf(outmsg, "No nets have %d or more users.", num);
    client->send(outmsg);
  }
}


void
UserHash::reportClones(BotClient * client)
{
  bool foundany = false;

  for (int i = 0; i < HASHTABLESIZE; ++i)
  {
    HashRec *top = this->hosttable[i];
    HashRec *userptr = this->hosttable[i];

    while (userptr)
    {
      HashRec *temp = top;

      while (temp != userptr)
      {
        if (server.same(temp->info->getHost(), userptr->info->getHost()))
	{
	  break;
	}
        else
	{
	  temp = temp->collision;
	}
      }

      if (temp == userptr)
      {
        std::vector<time_t> connfromhost;
	connfromhost.reserve(100);

        int numfound = 1;
        connfromhost.push_back(temp->info->getConnectTime());

        temp = temp->collision;
        while (temp)
	{
          if (server.same(temp->info->getHost(), userptr->info->getHost()))
	  {
	    ++numfound;
            connfromhost.push_back(temp->info->getConnectTime());
	  }
          temp = temp->collision;
        }

        if (numfound > 2)
	{
	  // sort connect times in decreasing order
	  std::sort(connfromhost.begin(), connfromhost.end(),
	    std::greater<time_t>());

	  int j, k;

          for (k = numfound - 1; k > 1; --k)
	  {
            for (j = 0; j < numfound - k; ++j)
	    {
              if ((connfromhost[j] > 0) && (connfromhost[j + k] > 0) &&
		((connfromhost[j] - connfromhost[j + k]) <=
		((k + 1) * CLONE_DETECT_INC)))
	      {
                goto getout;  /* goto rules! */
	      }
            }
	  }
          getout:
          if (k > 1)
	  {
            if (!foundany)
	    {
	      foundany = true;
              client->send("Possible clonebots from the following hosts:");
	    }

            char outmsg[MAX_BUFF];
            sprintf(outmsg,
	      "  %2d connections in %3ld seconds (%2d total) from %s", k + 1,
	      connfromhost[j] - connfromhost[j + k], numfound,
	      userptr->info->getHost().c_str());

            client->send(outmsg);
          }   
        } 
      }     
      userptr = userptr->collision;
    }
  }
  if (!foundany)
  {
    client->send("No potential clonebots found.");
  }
}


void
UserHash::reportMulti(BotClient * client, const int minimum)
{
  HashRec *userptr,*top,*temp;
  int numfound,i;
  char notip, foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (i=0;i<HASHTABLESIZE;++i) {
    top = userptr = this->domaintable[i];
    while (userptr)
    {
      numfound = 0;
      /* Ensure we haven't already checked this user & domain */
      temp = top;
      while (temp != userptr)
      {
        if (server.same(temp->info->getUser(), userptr->info->getUser()) &&
          server.same(temp->info->getDomain(), userptr->info->getDomain()))
	{
          break;
	}
        else
	{
          temp = temp->collision;
	}
      }
      if (temp == userptr)
      {
        temp = temp->collision;
        while (temp)
	{
          if (server.same(temp->info->getUser(), userptr->info->getUser()) &&
            server.same(temp->info->getDomain(), userptr->info->getDomain()))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!temp->info->getOper() &&
	      !Config::IsOper(temp->info->getUserHost(), temp->info->getIp())))
            {
              numfound++;       /* - zaph & Dianora :-) */
	    }
	  }
          temp = temp->collision;
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following userhosts:");
	  }

          notip = strncmp(userptr->info->getDomain().c_str(),
	    userptr->info->getHost().c_str(),
            strlen(userptr->info->getDomain().c_str())) ||
            (strlen(userptr->info->getDomain().c_str()) ==
	    strlen(userptr->info->getHost().c_str()));
          numfound++; /* - zaph and next line*/
          sprintf(outmsg, " %s %2d -- %s@%s%s",
            (numfound > minclones) ? "==>" : "   ", numfound,
	    userptr->info->getUser().c_str(),
	    notip ? "*" : userptr->info->getDomain().c_str(),
	    notip ? userptr->info->getDomain().c_str() : "*");
          client->send(outmsg);
        }
      }  
      userptr = userptr->collision;
    }
  }         
  if (!foundany)
  {
    client->send("No multiple logins found.");
  }
}

void
UserHash::reportHMulti(BotClient * client, const int minimum)
{
  HashRec *userptr,*top,*temp;
  int numfound,i;
  char foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (i=0;i<HASHTABLESIZE;++i) {
    top = userptr = this->domaintable[i];
    while (userptr)
    {
      numfound = 0;
      // Ensure we haven't already checked this hostname
      temp = top;
      while (temp != userptr)
        if (server.same(temp->info->getHost(), userptr->info->getHost()))
          break;
        else
          temp = temp->collision;
      if (temp == userptr) {
        temp = temp->collision;
        while (temp) {
          if (server.same(temp->info->getHost(), userptr->info->getHost()))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!temp->info->getOper() &&
	      !Config::IsOper(temp->info->getUserHost(), temp->info->getIp())))
            {
              numfound++;       /* - zaph & Dianora :-) */
	    }
	  }
          temp = temp->collision;
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following hosts:");
	  }

          numfound++; /* - zaph and next line*/
          sprintf(outmsg, " %s %2d -- *@%s",
	    (numfound > minclones) ? "==>" : "   ", numfound,
	    userptr->info->getHost().c_str());
          client->send(outmsg);
        }
      }  
      userptr = userptr->collision;
    }
  }         
  if (!foundany)
  {
    client->send("No multiple logins found.");
  }
}

void
UserHash::reportUMulti(BotClient * client, const int minimum)
{
  HashRec *userptr,*top,*temp;
  int numfound,i;
  char foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (i=0;i<HASHTABLESIZE;++i) {
    top = userptr = this->usertable[i];
    while (userptr)
    {
      numfound = 0;
      // Ensure we haven't already checked this username
      temp = top;
      while (temp != userptr)
        if (server.same(temp->info->getUser(), userptr->info->getUser()))
          break;
        else
          temp = temp->collision;
      if (temp == userptr) {
        temp = temp->collision;
        while (temp) {
          if (server.same(temp->info->getUser(), userptr->info->getUser()))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!temp->info->getOper() &&
	      !Config::IsOper(temp->info->getUserHost(), temp->info->getIp())))
            {
              numfound++;       /* - zaph & Dianora :-) */
	    }
	  }
          temp = temp->collision;
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following usernames:");
	  }

          numfound++; /* - zaph and next line*/
          sprintf(outmsg, " %s %2d -- %s@*",
	    (numfound > minclones) ? "==>" : "   ", numfound,
	    userptr->info->getUser().c_str());
          client->send(outmsg);
        }
      }  
      userptr = userptr->collision;
    }
  }         
  if (!foundany)
  {
    client->send("No multiple logins found.");
  }
}

void
UserHash::reportVMulti(BotClient * client, const int minimum)
{
  HashRec *userptr,*top,*temp;
  int numfound,i;
  char foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (i=0;i<HASHTABLESIZE;++i) {
    top = userptr = this->iptable[i];
    while (userptr)
    {
      numfound = 0;
      /* Ensure we haven't already checked this user & domain */
      temp = top;
      while (temp != userptr)
        if (server.same(temp->info->getUser(), userptr->info->getUser()) &&
          ((userptr->info->getIp() & BotSock::ClassCNetMask) ==
	  (userptr->info->getIp() & BotSock::ClassCNetMask)))
	{
          break;
	}
        else
	{
          temp = temp->collision;
	}
      if (temp == userptr) {
        temp = temp->collision;
        while (temp) {
          if (server.same(temp->info->getUser(), userptr->info->getUser()) &&
            ((userptr->info->getIp() & BotSock::ClassCNetMask) ==
	    (userptr->info->getIp() & BotSock::ClassCNetMask)))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!temp->info->getOper() &&
	      !Config::IsOper(temp->info->getUserHost(), temp->info->getIp())))
            {
              numfound++;       /* - zaph & Dianora :-) */
	    }
	  }
          temp = temp->collision;
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following vhosts:");
	  }

          numfound++; /* - zaph and next line*/
	  std::string IP = BotSock::inet_ntoa(userptr->info->getIp());
	  std::string::size_type lastDot = IP.rfind('.');
	  if (std::string::npos != lastDot)
	  {
	    IP = IP.substr(0, lastDot);
	  }
          sprintf(outmsg, " %s %2d -- %s@%s.*",
	    (numfound > minclones) ? "==>" : "   ", numfound,
	    userptr->info->getUser().c_str(), IP.c_str());
          client->send(outmsg);
        }
      }  
      userptr = userptr->collision;
    }
  }         
  if (!foundany)
  {
    client->send("No multiple logins found.");
  }
}


void
UserHash::checkHostClones(const std::string & host)
{
  const int index = UserHash::hashFunc(host);
  UserHash::HashRec *find = this->hosttable[index];
  
  time_t now = time(NULL);
  time_t oldest = now;
  time_t lastReport = 0;
  
  int cloneCount = 0; 
  int reportedClones = 0;

  while (NULL != find)
  {
    if (server.same(find->info->getHost(), host) &&
      ((now - find->info->getConnectTime()) <=
      vars[VAR_CLONE_MAX_TIME]->getInt()))
    {
      if (find->info->getReportTime() > 0)
      {
        ++reportedClones;

        if (lastReport < find->info->getReportTime())
	{
          lastReport = find->info->getReportTime();
	}
      }
      else
      {
        ++cloneCount;

        if (find->info->getConnectTime() < oldest)
        {
          oldest = find->info->getConnectTime();
        }
      }
    }
    find = find->collision;
  }
  
  if (((reportedClones == 0) &&
    (cloneCount < vars[VAR_CLONE_MIN_COUNT]->getInt())) ||
    ((now - lastReport) < vars[VAR_CLONE_REPORT_INTERVAL]->getInt()))
  {
    return;
  }

  std::string notice;
  if (reportedClones)
  {
    notice = IntToStr(cloneCount) + " more possible clones (" +
      IntToStr(cloneCount + reportedClones) + " total) from " + host + ":";
  }
  else
  {
    notice = "Possible clones from " + host + " detected: " +
      IntToStr(cloneCount) + " connects in " + IntToStr(now - oldest) +
      " seconds";
  }
  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);
            
  cloneCount = 0;
  find = this->hosttable[index];
    
  std::string lastUser;
  bool lastIdentd, currentIdentd;
  bool differentUser;

  std::string notice1;

  while (find)
  {
    if (server.same(find->info->getHost(), host) &&
      ((now - find->info->getConnectTime()) <=
      vars[VAR_CLONE_MAX_TIME]->getInt()) &&
      (find->info->getReportTime() == 0))
    {
      ++cloneCount;

#ifdef USERHASH_DEBUG
      std::cout << "clone(" << cloneCount << "): " <<
	find->info->output("%n %@ %i") << std::endl;
#endif

      std::string notice;
      if (cloneCount == 1)  
      {
	notice1 = find->info->output(
	  vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
      else
      {
	notice = find->info->output(vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
  
      lastIdentd = currentIdentd = true;
      differentUser = false;
        
      if (1 == cloneCount)
      {
        lastUser = find->info->getUser();
      }
      else if (2 == cloneCount)
      {
        std::string currentUser;

        if (lastUser[0] == '~')
        {
          lastUser = lastUser.substr(1);
          lastIdentd = false;
        }

        currentUser = find->info->getUser();
        if (currentUser[0] == '~')
        {
          currentUser = currentUser.substr(1);
          currentIdentd = false;
        }

        if (!server.same(lastUser, currentUser))
        {
          differentUser = true;
        }

        klineClones(true, currentUser, find->info->getHost(),
	  find->info->getIp(), differentUser, false,
	  lastIdentd | currentIdentd);
      }

      find->info->setReportTime(now);
      if (cloneCount == 1)
      {
	// do nothing
      }
      else if (cloneCount == 2) 
      {
        ::SendAll(notice1, UserFlags::OPER);
        ::SendAll(notice, UserFlags::OPER);
        Log::Write(notice1);
        Log::Write(notice);
      }
      else if (cloneCount < 5)
      {
        ::SendAll(notice, UserFlags::OPER);
        Log::Write(notice);
      }
      else if (cloneCount == 5)
      {
        ::SendAll(notice, UserFlags::OPER);
        Log::Write(notice);
      }
    }
    find = find->collision;
  }
}


void
UserHash::checkIpClones(const BotSock::Address & ip)
{
  const int index = UserHash::hashFunc(ip);
  UserHash::HashRec *find = this->iptable[index];
  
  time_t now = time(NULL);
  time_t oldest = now;
  time_t lastReport = 0;
  
  int cloneCount = 0; 
  int reportedClones = 0;

  while (NULL != find)
  {
    if (BotSock::sameClassC(find->info->getIp(), ip) &&
      ((now - find->info->getConnectTime()) <=
      vars[VAR_CLONE_MAX_TIME]->getInt()))
    {
      if (find->info->getReportTime() > 0)
      {
        ++reportedClones;

        if (lastReport < find->info->getReportTime())
	{
          lastReport = find->info->getReportTime();
	}
      }
      else
      {
        ++cloneCount;

        if (find->info->getConnectTime() < oldest)
        {
          oldest = find->info->getConnectTime();
        }
      }
    }
    find = find->collision;
  }
  
  if (((reportedClones == 0) &&
    (cloneCount < vars[VAR_CLONE_MIN_COUNT]->getInt())) ||
    ((now - lastReport) < vars[VAR_CLONE_REPORT_INTERVAL]->getInt()))
  {
    return;
  }

  std::string notice;
  if (reportedClones)
  {
    notice = IntToStr(cloneCount) + " more possible clones (" +
      IntToStr(cloneCount + reportedClones) + " total) from " +
      BotSock::inet_ntoa(ip) + ":";
  }
  else
  {
    notice = "Possible clones from " + BotSock::inet_ntoa(ip) + " detected: " +
      IntToStr(cloneCount) + " connects in " + IntToStr(now - oldest) +
      " seconds";
  }
  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);
            
  cloneCount = 0;
  find = this->iptable[index];

  BotSock::Address lastIp;
  std::string lastUser;
  bool lastIdentd, currentIdentd;
  bool differentIp, differentUser;

  std::string notice1;

  while (find)
  {
    if (BotSock::sameClassC(find->info->getIp(), ip) &&
      ((now - find->info->getConnectTime()) <=
      vars[VAR_CLONE_MAX_TIME]->getInt()) && (find->info->getReportTime() == 0))
    {
      ++cloneCount;

#ifdef USERHASH_DEBUG
      std::cout << "clone(" << cloneCount << "): " <<
	find->info->output("%n %@ %i") << std::endl;
#endif

      std::string notice;
      if (cloneCount == 1)  
      {
	notice1 = find->info->output(
	  vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
      else
      {
	notice = find->info->output(vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
  
      lastIdentd = currentIdentd = true;
      differentIp = false;
      differentUser = false;
        
      if (1 == cloneCount)
      {
        lastUser = find->info->getUser();
        lastIp = find->info->getIp();
      }
      else if (2 == cloneCount)
      {
        std::string currentUser;
        BotSock::Address currentIp;

        if (lastUser[0] == '~')
        {
          lastUser = lastUser.substr(1);
          lastIdentd = false;
        }

        currentUser = find->info->getUser();
        if (currentUser[0] == '~')
        {
          currentUser = currentUser.substr(1);
          currentIdentd = false;
        }

	currentIp = find->info->getIp();

        if (!server.same(lastUser, currentUser))
        {
          differentUser = true;
        }

	if (lastIp != currentIp)
	{
	  differentIp = true;
	}

        klineClones(true, currentUser, find->info->getHost(),
	  find->info->getIp(), differentUser, differentIp,
	  lastIdentd | currentIdentd);
      }

      find->info->setReportTime(now);
      if (cloneCount == 1)
      {
	// do nothing
      }
      else if (cloneCount == 2) 
      {
        ::SendAll(notice1, UserFlags::OPER);
        ::SendAll(notice, UserFlags::OPER);
        Log::Write(notice1);
        Log::Write(notice);
      }
      else if (cloneCount < 5)
      {
        ::SendAll(notice, UserFlags::OPER);
        Log::Write(notice);
      }
      else if (cloneCount == 5)
      {
        ::SendAll(notice, UserFlags::OPER);
        Log::Write(notice);
      }
    }
    find = find->collision;
  }
}


void
UserHash::status(BotClient * client)
{
  client->send("Users: " + IntToStr(this->userCount));

  int userHashCount = 0;
  long scoreSum = 0;
  for (int index = 0; index < HASHTABLESIZE; ++index)
  {
    for (UserHash::HashRec *pos = this->usertable[index]; NULL != pos;
      pos = pos->collision)
    {
      scoreSum += pos->info->getScore();
      userHashCount++;
    }
  }
  if (userHashCount != this->userCount)
  {
    client->send("Users in usertable: " + IntToStr(userHashCount));
  }

  int hostHashCount = 0;
  for (int index = 0; index < HASHTABLESIZE; ++index)
  {
    for (UserHash::HashRec *pos = this->hosttable[index]; NULL != pos;
      pos = pos->collision)
    {
      hostHashCount++;
    }
  }
  if (hostHashCount != this->userCount)
  {
    client->send("Users in hosttable: " + IntToStr(hostHashCount));
  }

  int domainHashCount = 0;
  for (int index = 0; index < HASHTABLESIZE; ++index)
  {
    for (UserHash::HashRec *pos = this->domaintable[index]; NULL != pos;
      pos = pos->collision)
    {
      domainHashCount++;
    }
  }
  if (domainHashCount != this->userCount)
  {
    client->send("Users in domaintable: " + IntToStr(domainHashCount));
  }

  int ipHashCount = 0;
  for (int index = 0; index < HASHTABLESIZE; ++index)
  {
    for (UserHash::HashRec *pos = this->iptable[index]; NULL != pos;
      pos = pos->collision)
    {
      ipHashCount++;
    }
  }
  if (ipHashCount != this->userCount)
  {
    client->send("Users in iptable: " + IntToStr(ipHashCount));
  }

  client->send("Average seedrand score: " +
    ((this->userCount > 0) ? IntToStr(scoreSum / this->userCount) : "N/A"));
}


BotSock::Address
UserHash::getIP(std::string nick, const std::string & userhost) const
{
  std::string::size_type at = userhost.find('@');

  if (std::string::npos != at)
  {
    std::string user = server.downCase(userhost.substr(0, at));
    std::string host = server.downCase(userhost.substr(at + 1));

    nick = server.downCase(nick);

    int index = UserHash::hashFunc(host);

    UserHash::HashRec *find = this->hosttable[index];

    while (NULL != find)
    {
      if ((nick.empty() ||
	(0 == nick.compare(server.downCase(find->info->getNick())))) &&
	(0 == user.compare(server.downCase(find->info->getUser()))) &&
	(0 == host.compare(server.downCase(find->info->getHost()))))
      {
        // Found the user -- return its IP address
        return find->info->getIp();
      }
      find = find->collision;
    }
  }
  return INADDR_NONE;
}


bool
UserHash::isOper(std::string nick, const std::string & userhost) const
{
  std::string::size_type at = userhost.find('@');

  if (std::string::npos != at)
  {
    std::string user = server.downCase(userhost.substr(0, at));
    std::string host = server.downCase(userhost.substr(at + 1));

    nick = server.downCase(nick);

    int index = UserHash::hashFunc(host);

    UserHash::HashRec *find = this->hosttable[index];

    while (NULL != find)
    {
      if ((0 == nick.compare(server.downCase(find->info->getNick()))) &&
	(0 == user.compare(server.downCase(find->info->getUser()))) &&
	(0 == host.compare(server.downCase(find->info->getHost()))))
      {
        // Found the user -- is it an oper?
        return find->info->getOper();
      }
      find = find->collision;
    }
  }
  return false;
}


int
UserHash::getUserCountDelta(void)
{
  int delta = this->userCount - this->previousCount;

  this->previousCount = this->userCount;

  return delta;
}


void
UserHash::resetUserCountDelta(void)
{
  this->previousCount = this->userCount;
}


UserHash::UserEntry::UserEntry(const std::string & aNick,
  const std::string & aUser, const std::string & aHost,
  const std::string & aUserClass, const std::string & aGecos,
  const BotSock::Address anIp, const time_t aConnectTime, const bool oper)
  : user(aUser), host(aHost), domain(::getDomain(aHost, false)),
  userClass(::server.downCase(aUserClass)), gecos(aGecos), ip(anIp),
  connectTime(aConnectTime), reportTime(0), isOper(oper), linkCount(0)
{
  this->setNick(aNick);
}


void
UserHash::UserEntry::setNick(const std::string & aNick)
{
  this->nick = aNick;
  this->randScore = ::seedrandScore(aNick);
}


std::string
UserHash::UserEntry::output(const std::string & format) const
{
  std::string::size_type len(format.length());
  std::string::size_type idx(0), next; 
  std::string result;

  while (idx < len)
  {
    next = idx + 1;

    if ((format[idx] == '%') && (next < len))
    {
      switch (UpCase(format[next]))
      {
        case '%':
	  result += '%';
	  ++next;
	  break;
        case '_':
	  result += ' ';
	  ++next;
	  break;
        case '-':
	  if ((result.length() > 0) && (result[result.length() - 1] != ' '))
	  {
	    result += ' ';
	  }
	  ++next;
	  break;
	case '@':
	  result += this->getUserHost();
	  ++next;
	  break;
	case 'C':
	  result += '{' + this->getClass() + '}';
	  ++next;
	  break;
	case 'G':
	  if (this->getGecos().length() > 0)
	  {
	    result += '<' + this->getGecos() + '>';
	  }
	  ++next;
	  break;
	case 'H':
	  result += this->getHost();
	  ++next;
	  break;
	case 'I':
	  if (this->getIp() != INADDR_NONE)
	  {
            result += '[' + BotSock::inet_ntoa(this->getIp()) + ']';
          }
	  ++next;
	  break;
	case 'N':
	  result += this->getNick();
	  ++next;
	  break;
	case 'S':
	  result += ::IntToStr(this->getScore(), 4);
	  ++next;
	  break;
	case 'T':
	  if (this->getConnectTime() != 0)
	  {
	    result += '(' + timeStamp(TIMESTAMP_CLIENT,
	      this->getConnectTime()) + ')';
	  }
	  ++next;
	  break;
	case 'U':
	  result += this->getUser();
	  ++next;
	  break;
        default:
	  result += '%';
	  break;
      }
    }
    else
    {
      result += format[idx];
    }
    idx = next;
  }
  return result;
}

