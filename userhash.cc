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
#include <ctime>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// Std C Headers
#include <stdio.h>

// OOMon Headers
#include "strtype"
#include "userhash.h"
#include "userentry.h"
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
#include "filter.h"
#include "botclient.h"
#include "format.h"


const static int CLONE_DETECT_INC = 15;


UserHash users;


#ifdef DEBUG_USERHASH
unsigned long userEntryCount = 0;
#endif


UserHash::UserHash(void)
{
  this->hosttable.resize(HASHTABLESIZE);
  this->domaintable.resize(HASHTABLESIZE);
  this->usertable.resize(HASHTABLESIZE);
  this->iptable.resize(HASHTABLESIZE);
  this->userCount = this->previousCount = 0;
}


UserHash::~UserHash(void)
{
  this->clear();
}

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

      std::string fakeHost;
      if ((0 == server.downCase(nick).compare(this->maskNick)) &&
	(0 == server.downCase(host).compare(this->maskRealHost)))
      {
	fakeHost = this->maskFakeHost;
      }

      UserEntryPtr newuser(new UserEntry(nick, user, host, fakeHost, userClass,
        gecos, ip.empty() ? INADDR_NONE : BotSock::inet_addr(ip),
        (fromTrace ? 0 : std::time(NULL)), isOper));

      // Add it to the hash tables
      UserHash::addToHash(this->hosttable, newuser->getHost(), newuser);
      UserHash::addToHash(this->domaintable, newuser->getDomain(), newuser);
      UserHash::addToHash(this->usertable, newuser->getUser(), newuser);
      UserHash::addToHash(this->iptable, newuser->getIP(), newuser);
      ++this->userCount;

      // Don't check for wingate or clones when doing a TRACE
      if (!fromTrace)
      {
        if (!Config::IsOKHost(userhost, ip) && !Config::IsOper(userhost, ip))
        {
          // Check if this client matches any traps
          if (vars[VAR_TRAP_CONNECTS]->getBool())
          {
            TrapList::match(newuser);
          }

          if (newuser->getScore() >= vars[VAR_SEEDRAND_REPORT_MIN]->getInt())
          {
	    std::string scoreStr(
	      boost::lexical_cast<std::string>(newuser->getScore()));

	    std::string notice("Random (score: ");
	    notice += scoreStr;
	    notice += ") nick connect: ";
	    notice += nick;
	    notice += " (";
	    notice += userhost;
	    notice += ")";
	    if ("" != ip)
	    {
	      notice += " [";
	      notice += ip;
	      notice += "]";
	    }

	    ::SendAll(notice, UserFlags::OPER, WATCH_SEEDRAND);
	    Log::Write(notice);

	    Format reason;
	    reason.setStringToken('n', nick);
	    reason.setStringToken('u', userhost);
	    reason.setStringToken('i', ip);
	    reason.setStringToken('s', scoreStr);

	    doAction(nick, userhost, BotSock::inet_addr(ip),
	      vars[VAR_SEEDRAND_ACTION]->getAction(),
	      vars[VAR_SEEDRAND_ACTION]->getInt(),
	      reason.format(vars[VAR_SEEDRAND_REASON]->getString()), false);
          }

          if (ip != "")
          {
            CheckProxy(ip, host, nick, userhost);
          }

	  if (vars[VAR_CTCPVERSION_ENABLE]->getBool())
	  {
	    newuser->version();
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
UserHash::setMaskHost(const std::string & nick, const std::string & realHost,
  const std::string & fakeHost)
{
  this->maskNick = server.downCase(nick);
  this->maskRealHost = server.downCase(realHost);
  this->maskFakeHost = fakeHost;
}


void
UserHash::updateOper(const std::string & nick, const std::string & userhost,
  bool isOper)
{
  UserEntryPtr find(this->findUser(nick, userhost));

  if (find)
  {
    find->setOper(true);
  }
}


void
UserHash::onVersionReply(const std::string & nick, const std::string & userhost,
  const std::string & version)
{
  UserEntryPtr find(this->findUser(nick, userhost));

  if (find)
  {
    find->hasVersion(version);
    if (vars[VAR_TRAP_CTCP_VERSIONS]->getBool())
    {
      TrapList::matchCtcpVersion(find, version);
    }
  }
}


void
UserHash::onPrivmsg(const std::string & nick, const std::string & userhost,
  const std::string & privmsg)
{
  if (vars[VAR_TRAP_PRIVMSGS]->getBool())
  {
    UserEntryPtr find(this->findUser(nick, userhost));

    if (find)
    {
	TrapList::matchPrivmsg(find, privmsg);
    }
  }
}


void
UserHash::onNotice(const std::string & nick, const std::string & userhost,
  const std::string & notice)
{
  if (vars[VAR_TRAP_NOTICES]->getBool())
  {
    UserEntryPtr find(this->findUser(nick, userhost));

    if (find)
    {
	TrapList::matchNotice(find, notice);
    }
  }
}


void
UserHash::checkVersionTimeout(void)
{
  std::time_t timeout = vars[VAR_CTCPVERSION_TIMEOUT]->getInt();

  if (timeout > 0)
  {
    std::time_t now = std::time(0);

    int n = 0;
    for (UserEntryTable::iterator i = usertable.begin(); i != usertable.end();
      ++i)
    {
      for (UserEntryList::iterator hp = i->begin(); hp != i->end(); ++hp)
      {
        (*hp)->checkVersionTimeout(now, timeout);
      }
      ++n;
    }
  }
}


void
UserHash::updateNick(const std::string & oldNick, const std::string & userhost,
  const std::string & newNick)
{
  UserEntryPtr find(this->findUser(oldNick, userhost));

  if (find)
  {
    find->setNick(newNick);

    if (vars[VAR_TRAP_NICK_CHANGES]->getBool())
    {
      TrapList::match(find);
    }

    if ((find->getScore() >= vars[VAR_SEEDRAND_REPORT_MIN]->getInt()) &&
      !Config::IsOper(userhost, find->getIP()) &&
      !Config::IsOKHost(userhost, find->getIP()) && !find->getOper())
    {
      std::string scoreStr(
	boost::lexical_cast<std::string>(find->getScore()));

      std::string notice("Random (score: ");
      notice += scoreStr;
      notice += ") nick change: ";
      notice += find->getNick();
      notice += " (";
      notice += userhost;
      notice += ")";
      if (INADDR_NONE != find->getIP())
      {
	notice += " [";
	notice += BotSock::inet_ntoa(find->getIP());
	notice += "]";
      }
      ::SendAll(notice, UserFlags::OPER, WATCH_SEEDRAND);
      Log::Write(notice);

      Format reason;
      reason.setStringToken('n', find->getNick());
      reason.setStringToken('u', userhost);
      reason.setStringToken('i', BotSock::inet_ntoa(find->getIP()));
      reason.setStringToken('s', scoreStr);

      doAction(find->getNick(), userhost, find->getIP(),
	vars[VAR_SEEDRAND_ACTION]->getAction(),
	vars[VAR_SEEDRAND_ACTION]->getInt(),
	reason.format(vars[VAR_SEEDRAND_REASON]->getString()), false);
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
UserHash::addToHash(UserEntryTable & table, const std::string & key,
  const UserEntryPtr & item)
{
  int index = UserHash::hashFunc(key);

  table[index].push_back(item);
}


void
UserHash::addToHash(UserEntryTable & table, const BotSock::Address & key,
  const UserEntryPtr & item)
{
  int index = UserHash::hashFunc(key);

  table[index].push_back(item);
}


bool
UserHash::removeFromHashEntry(UserEntryList & list, const std::string & host,
  const std::string & user, const std::string & nick)
{
  bool result = false;

  UserEntryList::iterator find = std::find_if(list.begin(), list.end(),
    boost::bind(&UserEntry::same, _1, nick, user, host));

  if (find != list.end())
  {
    list.erase(find);
    result = true;
  }

  return result;
}


bool
UserHash::removeFromHash(UserEntryTable & table, const std::string & key,
  const std::string & host, const std::string & user, const std::string & nick)
{
  if (key.length() > 0)
  {
    int index = UserHash::hashFunc(key);

    return removeFromHashEntry(table[index], host, user, nick);
  }
  else
  {
    for (UserEntryTable::iterator pos = table.begin(); pos != table.end();
      ++pos)
    {
      if (removeFromHashEntry(*pos, host, user, nick))
      {
	return true;
      }
    }
    return false;
  }
}


bool
UserHash::removeFromHash(UserEntryTable & table, const BotSock::Address & key,
  const std::string & host, const std::string & user, const std::string & nick)
{
  int index = UserHash::hashFunc(key);

  if (removeFromHashEntry(table[index], host, user, nick))
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
UserHash::clearHash(UserEntryTable & table)
{
  table.clear();
  table.resize(HASHTABLESIZE);
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
UserHash::findUsers(BotClient * client, const Filter & filter) const
{
  int numfound = 0;

  for (UserEntryTable::const_iterator index = this->usertable.begin();
    index != this->usertable.end(); ++index)
  {
    for (UserEntryList::const_iterator pos = index->begin();
      pos != index->end(); ++pos)
    {
      if (filter.matches(*pos))
      {
	++numfound;
	std::string outmsg((*pos)->output(vars[VAR_LIST_FORMAT]->getString()));
        client->send(outmsg);
      }
    }
  }

  std::string outmsg;
  outmsg += (numfound == 0) ? "No" : boost::lexical_cast<std::string>(numfound);
  outmsg += " match";
  outmsg += (numfound == 1) ? "" : "es";
  outmsg += " for ";
  outmsg += filter.get();
  outmsg += " found.";
  client->send(outmsg);

  return numfound;
}


int
UserHash::listUsers(BotClient * client, const PatternPtr userhost,
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

    for (UserEntryTable::const_iterator index = this->domaintable.begin();
      index != this->domaintable.end(); ++index)
    {
      for (UserEntryList::const_iterator userptr = index->begin();
        userptr != index->end(); ++userptr)
      {
	bool matchesUH = userhost->match((*userptr)->getUserHost());
	bool matchesUIP = false;
	if (INADDR_NONE != (*userptr)->getIP())
	{
	  matchesUIP = userhost->match((*userptr)->getUserIP());
	}
	if ((matchesUH || matchesUIP) && ((0 == className.length()) ||
	  (0 == className.compare((*userptr)->getClass()))))
	{
	  ++numfound;
	  std::string outmsg((*userptr)->output(vars[VAR_LIST_FORMAT]->getString()));
	  if (action == UserHash::LIST_KILL)
	  {
	    server.kill(from, (*userptr)->getNick(), reason);
	  }
	  if (action == UserHash::LIST_VIEW)
	  {
	    client->send(outmsg);
	  }
	}
      }
    }

    std::string outmsg;
    if (numfound > 0)
    {
      outmsg = boost::lexical_cast<std::string>(numfound) + " matches for " +
	userhost->get() + " found.";
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
UserHash::listNicks(BotClient * client, const PatternPtr nick,
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

  for (UserEntryTable::const_iterator index = this->domaintable.begin();
    index != this->domaintable.end(); ++index)
  {
    for (UserEntryList::const_iterator userptr = index->begin();
      userptr != index->end(); ++userptr)
    {
      if (nick->match((*userptr)->getNick()) && (className.empty() ||
	(0 == className.compare((*userptr)->getClass()))))
      {
        ++numfound;
	if (action == UserHash::LIST_KILL)
	{
	  server.kill(from, (*userptr)->getNick(), reason);
	}
	if (action == UserHash::LIST_VIEW)
	{
	  client->send((*userptr)->output(vars[VAR_NFIND_FORMAT]->getString()));
	}
      }
    }
  }

  std::string outmsg;
  if (numfound > 0)
  {
    outmsg = boost::lexical_cast<std::string>(numfound) + " matches for " +
      nick->get() + " found.";
  }
  else
  {
    outmsg = "No matches for " + nick->get() + " found.";
  }
  client->send(outmsg);

  return numfound;
}


int
UserHash::listGecos(BotClient * client, const PatternPtr gecos,
  std::string className, const bool count) const
{
  int numfound = 0;

  // Convert class name to uppercase now
  className = server.downCase(className);

  for (UserEntryTable::const_iterator index = this->domaintable.begin();
    index != this->domaintable.end(); ++index)
  {
    for (UserEntryList::const_iterator userptr = index->begin();
      userptr != index->end(); ++userptr)
    {
      if (gecos->match((*userptr)->getGecos()) && (className.empty() ||
	(0 == className.compare((*userptr)->getClass()))))
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
          client->send((*userptr)->output(vars[VAR_GLIST_FORMAT]->getString()));
	}
      }
    }
  }

  std::string outmsg;
  if (numfound > 0)
  {
    outmsg = boost::lexical_cast<std::string>(numfound) + " matches for " +
      gecos->get() + " found.";
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
  return (this->findUser(nick));
}


UserEntryPtr
UserHash::findUser(const std::string & nick) const
{
  std::string lcNick(server.downCase(nick));
  UserEntryPtr result;

  for (UserEntryTable::const_iterator i = this->hosttable.begin();
    i != this->hosttable.end(); ++i)
  {
    UserEntryList::const_iterator find =
      std::find_if(i->begin(), i->end(),
          boost::bind(&UserEntry::matches, _1, lcNick));

    if (find != i->end())
    {
      result = *find;
      break;
    }
  }

  return result;
}


UserEntryPtr
UserHash::findUser(const std::string & nick, const std::string & userhost) const
{
  std::string lcNick(server.downCase(nick));
  UserEntryPtr result;

  std::string::size_type at = userhost.find('@');
  if (std::string::npos != at)
  {
    std::string lcUser(server.downCase(userhost.substr(0, at)));
    std::string lcHost(server.downCase(userhost.substr(at + 1)));

    const int hashIndex = UserHash::hashFunc(lcUser);

    const UserEntryList & bucket = this->usertable[hashIndex];

    UserEntryList::const_iterator find =
      std::find_if(bucket.begin(), bucket.end(),
          boost::bind(&UserEntry::matches, _1, lcNick, lcUser, lcHost));

    if (find != bucket.end())
    {
      result = *find;
    }
  }

  return result;
}


void
UserHash::reportClasses(BotClient * client, const std::string & className)
{
  typedef std::map<std::string,int> ClassType;
  ClassType Classes;

  for (UserEntryTable::iterator i = this->domaintable.begin();
    i != this->domaintable.end(); ++i)
  {
    for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
      ++userptr)
    {
      Classes[server.downCase((*userptr)->getClass())]++;
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
UserHash::reportSeedrand(BotClient * client, const PatternPtr mask,
  const int threshold, const bool count) const
{
  if (!count)
  {
    client->send("Searching " + mask->get() + ".  Threshold is " +
      boost::lexical_cast<std::string>(threshold));
  }

  std::list<UserHash::ScoreNode> scores;

  for (UserEntryTable::const_iterator i = this->usertable.begin();
    i != this->usertable.end(); ++i)
  {
    for (UserEntryList::const_iterator find = i->begin(); find != i->end();
      ++find)
    {
      if (mask->match((*find)->getNick()))
      {
	if ((*find)->getScore() >= threshold)
        {
	  UserHash::ScoreNode tmp(*find, (*find)->getScore());
	  scores.push_back(tmp);
        }
      }
    }
  }

  if (!count)
  {
    scores.sort();

    for (std::list<UserHash::ScoreNode>::const_iterator pos = scores.begin();
      pos != scores.end(); ++pos)
    {
      UserEntryPtr info(pos->getInfo());

      client->send(info->output(vars[VAR_SEEDRAND_FORMAT]->getString()));
    }
  }

  if (0 == scores.size())
  {
    client->send("No matches (score >= " +
      boost::lexical_cast<std::string>(threshold) + ") for " + mask->get() +
      " found.");
  }
  else if (1 == scores.size())
  {
    client->send("1 match (score >= " +
      boost::lexical_cast<std::string>(threshold) + ") for " + mask->get() +
      " found.");
  }
  else
  {
    client->send(boost::lexical_cast<std::string>(scores.size()) +
      " matches (score >= " + boost::lexical_cast<std::string>(threshold) +
      ") for " + mask->get() + " found.");
  }
}


void
UserHash::reportDomains(BotClient * client, const int minimum)
{
  typedef std::map<std::string, int> UnsortedMap;
  typedef std::multimap<int, std::string> SortedMap;
  UnsortedMap unsorted;
  SortedMap sorted;

  if (minimum < 1)
  {
    client->send("*** Parameter must be greater than 0!");
  }
  else
  {
    for (UserEntryTable::iterator i = this->hosttable.begin();
        i != this->hosttable.end(); ++i)
    {
      for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
          ++userptr)
      {
        std::string domain(server.downCase((*userptr)->getDomain()));
        ++unsorted[domain];
      }
    }

    for (UnsortedMap::iterator pos = unsorted.begin(); pos != unsorted.end();
        ++pos)
    {
      if (pos->second >= minimum)
      {
        sorted.insert(SortedMap::value_type(pos->second, pos->first));
      }
    }

    if (sorted.empty())
    {
      std::string outmsg("*** No domains have ");
      outmsg += boost::lexical_cast<std::string>(minimum);
      outmsg += " or more users.";
      client->send(outmsg);
    }
    else
    {
      client->send("Domains with most users on the server:");

      for (SortedMap::reverse_iterator pos = sorted.rbegin();
          pos != sorted.rend(); ++pos)
      {
        char outmsg[MAX_BUFF];            
        sprintf(outmsg, "  %-40s %3d user%s", pos->second.c_str(),
          pos->first, (pos->first == 1) ? "" : "s");
        client->send(outmsg);
      }
    }
  }
}


void
UserHash::reportNets(BotClient * client, const int minimum)
{
  typedef std::map<BotSock::Address, int> UnsortedMap;
  typedef std::multimap<int, BotSock::Address> SortedMap;
  UnsortedMap unsorted;
  SortedMap sorted;

  if (minimum < 1)
  {
    client->send("*** Parameter must be greater than 0!");
  }
  else
  {
    for (UserEntryTable::iterator i = this->hosttable.begin();
        i != this->hosttable.end(); ++i)
    {
      for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
          ++userptr)
      {
        BotSock::Address ip((*userptr)->getIP());

        if (INADDR_NONE != ip)
        {
          ++unsorted[ip & BotSock::ClassCNetMask];
        }
      }
    }

    for (UnsortedMap::iterator pos = unsorted.begin(); pos != unsorted.end();
        ++pos)
    {
      if (pos->second >= minimum)
      {
        sorted.insert(SortedMap::value_type(pos->second, pos->first));
      }
    }

    if (sorted.empty())
    {
      std::string outmsg("*** No nets have ");
      outmsg += boost::lexical_cast<std::string>(minimum);
      outmsg += " or more users.";
      client->send(outmsg);
    }
    else
    {
      client->send("Nets with most users on the server:");

      for (SortedMap::reverse_iterator pos = sorted.rbegin();
          pos != sorted.rend(); ++pos)
      {
        char outmsg[MAX_BUFF];            
        sprintf(outmsg, "  %-40s %3d user%s",
	  classCMask(BotSock::inet_ntoa(pos->second)).c_str(), pos->first,
          (pos->first == 1) ? "" : "s");
        client->send(outmsg);
      }
    }
  }
}


void
UserHash::reportClones(BotClient * client)
{
  bool foundany = false;

  for (UserEntryTable::iterator i = this->hosttable.begin();
    i != this->hosttable.end(); ++i)
  {
    for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
      ++userptr)
    {
      UserEntryList::iterator temp = i->begin();
      for (; temp != userptr; ++temp)
      {
        if (server.same((*temp)->getHost(), (*userptr)->getHost()))
	{
	  break;
	}
      }

      if (temp == userptr)
      {
        std::vector<std::time_t> connfromhost;
	connfromhost.reserve(100);

        int numfound = 1;
        connfromhost.push_back((*temp)->getConnectTime());

        ++temp;
        while (temp != i->end())
	{
          if (server.same((*temp)->getHost(), (*userptr)->getHost()))
	  {
	    ++numfound;
            connfromhost.push_back((*temp)->getConnectTime());
	  }
          ++temp;
        }

        if (numfound > 2)
	{
	  // sort connect times in decreasing order
	  std::sort(connfromhost.begin(), connfromhost.end(),
	    std::greater<std::time_t>());

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
	      (*userptr)->getHost().c_str());

            client->send(outmsg);
          }   
        } 
      }     
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
  int numfound;
  char notip, foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (UserEntryTable::iterator i = this->domaintable.begin();
    i != this->domaintable.end(); ++i)
  {
    for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
      ++userptr)
    {
      numfound = 0;
      /* Ensure we haven't already checked this user & domain */
      UserEntryList::iterator temp = i->begin();
      for (; temp != userptr; ++temp)
      {
        if (server.same((*temp)->getUser(), (*userptr)->getUser()) &&
          server.same((*temp)->getDomain(), (*userptr)->getDomain()))
	{
          break;
	}
      }
      if (temp == userptr)
      {
        for (++temp; temp != i->end(); ++temp)
	{
          if (server.same((*temp)->getUser(), (*userptr)->getUser()) &&
            server.same((*temp)->getDomain(), (*userptr)->getDomain()))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!(*temp)->getOper() &&
	      !Config::IsOper((*temp)->getUserHost(), (*temp)->getIP())))
            {
              numfound++;       /* - zaph & Dianora :-) */
	    }
	  }
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following userhosts:");
	  }

          notip = strncmp((*userptr)->getDomain().c_str(),
	    (*userptr)->getHost().c_str(),
            strlen((*userptr)->getDomain().c_str())) ||
            (strlen((*userptr)->getDomain().c_str()) ==
	    strlen((*userptr)->getHost().c_str()));
          numfound++; /* - zaph and next line*/
          sprintf(outmsg, " %s %2d -- %s@%s%s",
            (numfound > minclones) ? "==>" : "   ", numfound,
	    (*userptr)->getUser().c_str(),
	    notip ? "*" : (*userptr)->getDomain().c_str(),
	    notip ? (*userptr)->getDomain().c_str() : "*");
          client->send(outmsg);
        }
      }  
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
  int numfound;
  char foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (UserEntryTable::iterator i = this->domaintable.begin();
    i != this->domaintable.end(); ++i)
  {
    for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
      ++userptr)
    {
      numfound = 0;
      // Ensure we haven't already checked this hostname
      UserEntryList::iterator temp = i->begin();
      for (; temp != userptr; ++temp)
      {
        if (server.same((*temp)->getHost(), (*userptr)->getHost()))
        {
          break;
        }
      }
      if (temp == userptr)
      {
        for (++temp; temp != i->end(); ++temp)
        {
          if (server.same((*temp)->getHost(), (*userptr)->getHost()))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!(*temp)->getOper() &&
	      !Config::IsOper((*temp)->getUserHost(), (*temp)->getIP())))
            {
              numfound++;       /* - zaph & Dianora :-) */
	    }
	  }
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
	    (*userptr)->getHost().c_str());
          client->send(outmsg);
        }
      }  
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
  int numfound;
  char foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (UserEntryTable::iterator i = this->usertable.begin();
    i != this->usertable.end(); ++i)
  {
    for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
      ++userptr)
    {
      numfound = 0;
      // Ensure we haven't already checked this username
      UserEntryList::iterator temp = i->begin();
      for (; temp != userptr; ++temp)
      {
        if (server.same((*temp)->getUser(), (*userptr)->getUser()))
        {
          break;
        }
      }
      if (temp == userptr)
      {
        for (++temp; temp != i->end(); ++temp)
        {
          if (server.same((*temp)->getUser(), (*userptr)->getUser()))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!(*temp)->getOper() &&
	      !Config::IsOper((*temp)->getUserHost(), (*temp)->getIP())))
            {
              ++numfound;       /* - zaph & Dianora :-) */
	    }
	  }
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following usernames:");
	  }

          ++numfound; /* - zaph and next line*/
          sprintf(outmsg, " %s %2d -- %s@*",
	    (numfound > minclones) ? "==>" : "   ", numfound,
	    (*userptr)->getUser().c_str());
          client->send(outmsg);
        }
      }  
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
  int numfound;
  char foundany = 0;
  char outmsg[MAX_BUFF];
  int minclones = vars[VAR_MULTI_MIN]->getInt();

  if (minimum > 0)
    minclones = minimum;

  for (UserEntryTable::iterator i = this->iptable.begin();
    i != this->iptable.end(); ++i)
  {
    for (UserEntryList::iterator userptr = i->begin(); userptr != i->end();
      ++userptr)
    {
      numfound = 0;
      /* Ensure we haven't already checked this user & domain */
      UserEntryList::iterator temp = i->begin();
      for (; temp != userptr; ++temp)
      {
        if (server.same((*temp)->getUser(), (*userptr)->getUser()) &&
          (((*userptr)->getIP() & BotSock::ClassCNetMask) ==
	  ((*userptr)->getIP() & BotSock::ClassCNetMask)))
	{
          break;
	}
      }
      if (temp == userptr)
      {
        for (++temp; temp != i->end(); ++temp)
        {
          if (server.same((*temp)->getUser(), (*userptr)->getUser()) &&
            (((*userptr)->getIP() & BotSock::ClassCNetMask) ==
	    ((*userptr)->getIP() & BotSock::ClassCNetMask)))
	  {
            if (vars[VAR_OPER_IN_MULTI]->getBool() || (!(*temp)->getOper() &&
	      !Config::IsOper((*temp)->getUserHost(), (*temp)->getIP())))
            {
              ++numfound;       /* - zaph & Dianora :-) */
	    }
	  }
        }
        if (numfound >= (minclones - 1))
	{
          if (!foundany++)
	  {
            client->send("Multiple clients from the following vhosts:");
	  }

          ++numfound; /* - zaph and next line*/
	  std::string IP = BotSock::inet_ntoa((*userptr)->getIP());
	  std::string::size_type lastDot = IP.rfind('.');
	  if (std::string::npos != lastDot)
	  {
	    IP = IP.substr(0, lastDot);
	  }
          sprintf(outmsg, " %s %2d -- %s@%s.*",
	    (numfound > minclones) ? "==>" : "   ", numfound,
	    (*userptr)->getUser().c_str(), IP.c_str());
          client->send(outmsg);
        }
      }  
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
  
  std::time_t now = std::time(NULL);
  std::time_t oldest = now;
  std::time_t lastReport = 0;
  
  int cloneCount = 0; 
  int reportedClones = 0;

  for (UserEntryList::iterator find = this->hosttable[index].begin();
    find != this->hosttable[index].end(); ++find)
  {
    if (server.same((*find)->getHost(), host) &&
      ((now - (*find)->getConnectTime()) <= vars[VAR_CLONE_MAX_TIME]->getInt()))
    {
      if ((*find)->getReportTime() > 0)
      {
        ++reportedClones;

        if (lastReport < (*find)->getReportTime())
	{
          lastReport = (*find)->getReportTime();
	}
      }
      else
      {
        ++cloneCount;

        if ((*find)->getConnectTime() < oldest)
        {
          oldest = (*find)->getConnectTime();
        }
      }
    }
  }
  
  if (((reportedClones == 0) &&
    (cloneCount < vars[VAR_CLONE_MIN_COUNT]->getInt())) ||
    ((now - lastReport) < vars[VAR_CLONE_REPORT_INTERVAL]->getInt()))
  {
    return;
  }

  std::string rate(boost::lexical_cast<std::string>(cloneCount +
    reportedClones));
  rate += " connect";
  if (1 != (cloneCount + reportedClones))
  {
    rate += 's';
  }
  rate += " in ";
  rate += boost::lexical_cast<std::string>(now - oldest);
  rate += " second";
  if (1 != (now - oldest))
  {
    rate += 's';
  }

  std::string notice;
  if (reportedClones)
  {
    notice = boost::lexical_cast<std::string>(cloneCount);
    notice += " more possible clones (";
    notice += boost::lexical_cast<std::string>(cloneCount + reportedClones);
    notice += " total) from ";
    notice += host;
    notice += ':';
  }
  else
  {
    notice = "Possible clones from ";
    notice += host;
    notice += " detected: ";
    notice += rate;
  }
  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);
            
  cloneCount = 0;
    
  std::string lastUser;
  bool lastIdentd, currentIdentd;
  bool differentUser;

  std::string notice1;

  for (UserEntryList::iterator find = this->hosttable[index].begin();
    find != this->hosttable[index].end(); ++find)
  {
    if (server.same((*find)->getHost(), host) &&
      ((now - (*find)->getConnectTime()) <=
       vars[VAR_CLONE_MAX_TIME]->getInt()) &&
      ((*find)->getReportTime() == 0))
    {
      ++cloneCount;

#ifdef USERHASH_DEBUG
      std::cout << "clone(" << cloneCount << "): " <<
	(*find)->output("%n %@ %i") << std::endl;
#endif

      std::string notice;
      if (cloneCount == 1)  
      {
	notice1 = (*find)->output(
	  vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
      else
      {
	notice = (*find)->output(vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
  
      lastIdentd = currentIdentd = true;
      differentUser = false;
        
      if (1 == cloneCount)
      {
        lastUser = (*find)->getUser();
      }
      else if (2 == cloneCount)
      {
        std::string currentUser;

        if (lastUser[0] == '~')
        {
          lastUser = lastUser.substr(1);
          lastIdentd = false;
        }

        currentUser = (*find)->getUser();
        if (currentUser[0] == '~')
        {
          currentUser = currentUser.substr(1);
          currentIdentd = false;
        }

        if (!server.same(lastUser, currentUser))
        {
          differentUser = true;
        }

        klineClones(true, rate, currentUser, (*find)->getHost(),
          (*find)->getIP(), differentUser, false, lastIdentd | currentIdentd);
      }

      (*find)->setReportTime(now);
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
  }
}


void
UserHash::checkIpClones(const BotSock::Address & ip)
{
  const int index = UserHash::hashFunc(ip);
  
  std::time_t now = std::time(NULL);
  std::time_t oldest = now;
  std::time_t lastReport = 0;
  
  int cloneCount = 0; 
  int reportedClones = 0;

  for (UserEntryList::iterator find = this->iptable[index].begin();
    find != this->iptable[index].end(); ++find)
  {
    if (BotSock::sameClassC((*find)->getIP(), ip) &&
      ((now - (*find)->getConnectTime()) <= vars[VAR_CLONE_MAX_TIME]->getInt()))
    {
      if ((*find)->getReportTime() > 0)
      {
        ++reportedClones;

        if (lastReport < (*find)->getReportTime())
	{
          lastReport = (*find)->getReportTime();
	}
      }
      else
      {
        ++cloneCount;

        if ((*find)->getConnectTime() < oldest)
        {
          oldest = (*find)->getConnectTime();
        }
      }
    }
  }
  
  if (((reportedClones == 0) &&
    (cloneCount < vars[VAR_CLONE_MIN_COUNT]->getInt())) ||
    ((now - lastReport) < vars[VAR_CLONE_REPORT_INTERVAL]->getInt()))
  {
    return;
  }

  std::string rate(boost::lexical_cast<std::string>(cloneCount +
    reportedClones));
  rate += " connect";
  if (1 != (cloneCount + reportedClones))
  {
    rate += 's';
  }
  rate += " in ";
  rate += boost::lexical_cast<std::string>(now - oldest);
  rate += " second";
  if (1 != (now - oldest))
  {
    rate += 's';
  }

  std::string notice;
  if (reportedClones)
  {
    notice = boost::lexical_cast<std::string>(cloneCount);
    notice += " more possible clones (";
    notice += boost::lexical_cast<std::string>(cloneCount + reportedClones);
    notice += " total) from ";
    notice += BotSock::inet_ntoa(ip);
    notice += ':';
  }
  else
  {
    notice = "Possible clones from ";
    notice += BotSock::inet_ntoa(ip);
    notice += " detected: ";
    notice += rate;
  }
  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);
            
  cloneCount = 0;

  BotSock::Address lastIp = 0;
  std::string lastUser;
  bool lastIdentd, currentIdentd;
  bool differentIp, differentUser;

  std::string notice1;

  for (UserEntryList::iterator find = this->iptable[index].begin();
    find != this->iptable[index].end(); ++find)
  {
    if (BotSock::sameClassC((*find)->getIP(), ip) &&
      ((now - (*find)->getConnectTime()) <=
       vars[VAR_CLONE_MAX_TIME]->getInt()) &&
      ((*find)->getReportTime() == 0))
    {
      ++cloneCount;

#ifdef USERHASH_DEBUG
      std::cout << "clone(" << cloneCount << "): " <<
	(*find)->output("%n %@ %i") << std::endl;
#endif

      std::string notice;
      if (cloneCount == 1)  
      {
	notice1 = (*find)->output(
	  vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
      else
      {
	notice = (*find)->output(vars[VAR_CLONE_REPORT_FORMAT]->getString());
      }
  
      lastIdentd = currentIdentd = true;
      differentIp = false;
      differentUser = false;
        
      if (1 == cloneCount)
      {
        lastUser = (*find)->getUser();
        lastIp = (*find)->getIP();
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

        currentUser = (*find)->getUser();
        if (currentUser[0] == '~')
        {
          currentUser = currentUser.substr(1);
          currentIdentd = false;
        }

	currentIp = (*find)->getIP();

        if (!server.same(lastUser, currentUser))
        {
          differentUser = true;
        }

	if (lastIp != currentIp)
	{
	  differentIp = true;
	}

        klineClones(true, rate, currentUser, (*find)->getHost(),
          (*find)->getIP(), differentUser, differentIp,
          lastIdentd | currentIdentd);
      }

      (*find)->setReportTime(now);
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
  }
}


#ifdef DEBUG_USERHASH
void
UserHash::debugStatus(BotClient * client, const UserEntryTable & table,
    const std::string & label, const int userCount)
{
  int tableSize = table.size();
  if (tableSize > 0)
  {
    double average = userCount / tableSize;
    int max = 0;
    double square = 0.0;
    for (UserEntryTable::const_iterator i = table.begin(); i != table.end();
        ++i)
    {
      int size = i->size();
      double diff = size - average;
      square += (diff * diff) / tableSize;
      if (size > max)
      {
        max = size;
      }
    }

    std::string notice(label);
    notice += " stddev: ";
    notice += boost::lexical_cast<std::string>(sqrt(square));
    notice += " (max = ";
    notice += boost::lexical_cast<std::string>(max);
    notice += ")";
    client->send(notice);
  }
}
#endif


void
UserHash::status(BotClient * client)
{
  client->send("Users: " + boost::lexical_cast<std::string>(this->userCount));
#ifdef DEBUG_USERHASH
  client->send("userEntryCount: " +
    boost::lexical_cast<std::string>(userEntryCount));
#endif

  int userHashCount = 0;
  long scoreSum = 0;
  for (UserEntryTable::iterator index = this->usertable.begin();
    index != this->usertable.end(); ++index)
  {
    for (UserEntryList::iterator pos = index->begin(); pos != index->end();
      ++pos)
    {
      scoreSum += (*pos)->getScore();
      ++userHashCount;
    }
  }
  if (userHashCount != this->userCount)
  {
    client->send("Users in usertable: " +
      boost::lexical_cast<std::string>(userHashCount));
  }

  int hostHashCount = 0;
  for (UserEntryTable::iterator index = this->hosttable.begin();
    index != this->hosttable.end(); ++index)
  {
    hostHashCount += index->size();
  }
  if (hostHashCount != this->userCount)
  {
    client->send("Users in hosttable: " +
      boost::lexical_cast<std::string>(hostHashCount));
  }

  int domainHashCount = 0;
  for (UserEntryTable::iterator index = this->domaintable.begin();
    index != this->domaintable.end(); ++index)
  {
    domainHashCount += index->size();
  }
  if (domainHashCount != this->userCount)
  {
    client->send("Users in domaintable: " +
      boost::lexical_cast<std::string>(domainHashCount));
  }

  int ipHashCount = 0;
  for (UserEntryTable::iterator index = this->iptable.begin();
    index != this->iptable.end(); ++index)
  {
    ipHashCount += index->size();
  }
  if (ipHashCount != this->userCount)
  {
    client->send("Users in iptable: " +
      boost::lexical_cast<std::string>(ipHashCount));
  }

  client->send("Average seedrand score: " +
    ((this->userCount > 0) ?
    boost::lexical_cast<std::string>(scoreSum / this->userCount) : "N/A"));

#ifdef DEBUG_USERHASH
  this->debugStatus(client, this->usertable, "usertable", this->userCount);
  this->debugStatus(client, this->hosttable, "hosttable", this->userCount);
  this->debugStatus(client, this->domaintable, "domaintable", this->userCount);
  this->debugStatus(client, this->iptable, "iptable", this->userCount);
#endif
}


BotSock::Address
UserHash::getIP(std::string nick, const std::string & userhost) const
{
  UserEntryPtr entry(this->findUser(nick, userhost));
  BotSock::Address result = INADDR_NONE;

  if (entry)
  {
    result = entry->getIP();
  }

  return result;
}


bool
UserHash::isOper(std::string nick, const std::string & userhost) const
{
  UserEntryPtr find(this->findUser(nick, userhost));
  bool result = false;

  if (find)
  {
    // Found the user -- is it an oper?
    result = find->getOper();
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

