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
#include <boost/format.hpp>

// OOMon Headers
#include "strtype"
#include "userhash.h"
#include "userentry.h"
#include "botsock.h"
#include "util.h"
#include "config.h"
#include "irc.h"
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
#include "action.h"
#include "defaults.h"


// Used by reportSeedrand
class ScoreNode
{
public:
  ScoreNode(UserEntryPtr info, int score) : info_(info),
    score_(score) { }
  UserEntryPtr getInfo() const { return this->info_; };
  int getScore() const { return this->score_; };
  bool operator < (const ScoreNode & compare) const
    { return (this->score_ < compare.score_); };
private:
  UserEntryPtr info_;
  int score_;
};


const static int CLONE_DETECT_INC = 15;


int UserHash::cloneMaxTime(DEFAULT_CLONE_MAX_TIME);
int UserHash::cloneMinCount(DEFAULT_CLONE_MIN_COUNT);
std::string UserHash::cloneReportFormat(DEFAULT_CLONE_REPORT_FORMAT);
int UserHash::cloneReportInterval(DEFAULT_CLONE_REPORT_INTERVAL);
bool UserHash::ctcpversionEnable(DEFAULT_CTCPVERSION_ENABLE);
int UserHash::ctcpversionTimeout(DEFAULT_CTCPVERSION_TIMEOUT);
AutoAction UserHash::ctcpversionTimeoutAction(DEFAULT_CTCPVERSION_TIMEOUT_ACTION,
    DEFAULT_CTCPVERSION_TIMEOUT_ACTION_TIME);
std::string UserHash::ctcpversionTimeoutReason(DEFAULT_CTCPVERSION_TIMEOUT_REASON);
int UserHash::multiMin(DEFAULT_MULTI_MIN);
bool UserHash::operInMulti(DEFAULT_OPER_IN_MULTI);
AutoAction UserHash::seedrandAction(DEFAULT_SEEDRAND_ACTION,
    DEFAULT_SEEDRAND_ACTION_TIME);
std::string UserHash::seedrandFormat(DEFAULT_SEEDRAND_FORMAT);
std::string UserHash::seedrandReason(DEFAULT_SEEDRAND_REASON);
int UserHash::seedrandReportMin(DEFAULT_SEEDRAND_REPORT_MIN);
bool UserHash::trapConnects(DEFAULT_TRAP_CONNECTS);
bool UserHash::trapCtcpVersions(DEFAULT_TRAP_CTCP_VERSIONS);
bool UserHash::trapNickChanges(DEFAULT_TRAP_NICK_CHANGES);
bool UserHash::trapNotices(DEFAULT_TRAP_NOTICES);
bool UserHash::trapPrivmsgs(DEFAULT_TRAP_PRIVMSGS);


UserHash users;


#ifdef USERHASH_DEBUG
unsigned long userEntryCount = 0;
#endif /* USERHASH_DEBUG */


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

unsigned int
UserHash::hashFunc(const std::string & key)
{
  std::string::size_type len = key.length();
  unsigned int i = 0;

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


unsigned int
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

    if (!checkForSpoof(nick, user, host, ip, userClass))
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
        (fromTrace ? 0 : std::time(0)), isOper));

#ifdef USERHASH_DEBUG
      std::cout << "New UserEntryPtr: " << newuser.get() << std::endl;
#endif

      // Add it to the hash tables
      UserHash::addToHash(this->hosttable, newuser->getHost(), newuser);
      UserHash::addToHash(this->domaintable, newuser->getDomain(), newuser);
      UserHash::addToHash(this->usertable, newuser->getUser(), newuser);
      UserHash::addToHash(this->iptable, newuser->getIP(), newuser);
      ++this->userCount;

      // Don't check for wingate or clones when doing a TRACE
      if (!fromTrace && !config.isOper(newuser))
      {
        // Check if this client matches any traps
        if (UserHash::trapConnects)
        {
          TrapList::match(newuser);
        }

        if ((newuser->getScore() >= UserHash::seedrandReportMin) &&
            !config.isExempt(newuser, Config::EXEMPT_SEEDRAND))
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
              UserHash::seedrandAction, reason.format(UserHash::seedrandReason),
              false);
        }

        if ((newuser->getIP() != INADDR_NONE) &&
            !config.isExempt(newuser, Config::EXEMPT_PROXY))
        {
          checkProxy(newuser);
        }

        if (UserHash::ctcpversionEnable &&
            !config.isExempt(newuser, Config::EXEMPT_VERSION))
        {
          newuser->version();
        }

        if (!config.isExempt(newuser, Config::EXEMPT_CLONE))
        {
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
    if (UserHash::trapCtcpVersions &&
        !config.isExempt(find, Config::EXEMPT_VERSION))
    {
      TrapList::matchCtcpVersion(find, version);
    }
  }
}


void
UserHash::onPrivmsg(const std::string & nick, const std::string & userhost,
  const std::string & privmsg)
{
  if (UserHash::trapPrivmsgs)
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
  if (UserHash::trapNotices)
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
  std::time_t timeout = UserHash::ctcpversionTimeout;

  if (timeout > 0)
  {
    std::time_t now = std::time(0);

    for (UserEntryTable::iterator i = usertable.begin(); i != usertable.end();
      ++i)
    {
      for (UserEntryList::iterator hp = i->begin(); hp != i->end(); ++hp)
      {
        UserEntryPtr user(*hp);

        std::time_t timedOut = user->checkVersionTimeout(now, timeout);
        if (timedOut > 0)
        {
          std::string nick(user->getNick());
          std::string userhost(user->getUserHost());

          std::string notice("*** No CTCP VERSION reply from ");
          notice += nick;
          notice += " (";
          notice += userhost;
          notice += ") in ";
          notice += boost::lexical_cast<std::string>(timedOut);
          notice += " seconds.";

          ::SendAll(notice, UserFlags::OPER, WATCH_CTCPVERSIONS);
          Log::Write(notice);

          doAction(nick, userhost, user->getIP(),
              UserHash::ctcpversionTimeoutAction,
              UserHash::ctcpversionTimeoutReason, false);
        }
      }
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

    if (UserHash::trapNickChanges)
    {
      TrapList::match(find);
    }

    if ((find->getScore() >= UserHash::seedrandReportMin) &&
        !find->getOper() && !config.isOper(find) &&
        !config.isExempt(find, Config::EXEMPT_SEEDRAND))
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
          UserHash::seedrandAction, reason.format(UserHash::seedrandReason),
          false);
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

    if (UserEntry::brokenHostnameMunging())
    {
      host = "";
      domain = "";
    }

    --this->userCount;

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
  UserEntryPtr item)
{
  const unsigned int index = UserHash::hashFunc(key);

  table[index].push_back(item);
}


void
UserHash::addToHash(UserEntryTable & table, const BotSock::Address & key,
  UserEntryPtr item)
{
  const unsigned int index = UserHash::hashFunc(key);

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
    (*find)->disconnect();

    list.erase(find);
    result = true;
  }

  return result;
}


bool
UserHash::removeFromHash(UserEntryTable & table, const std::string & key,
  const std::string & host, const std::string & user, const std::string & nick)
{
  if (!key.empty())
  {
    const unsigned int index = UserHash::hashFunc(key);

    return UserHash::removeFromHashEntry(table[index], host, user, nick);
  }
  else
  {
    for (UserEntryTable::iterator pos = table.begin(); pos != table.end();
        ++pos)
    {
      if (UserHash::removeFromHashEntry(*pos, host, user, nick))
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
  const unsigned int index = UserHash::hashFunc(key);

  if (UserHash::removeFromHashEntry(table[index], host, user, nick))
  {
    return true;
  }
  else
  {
    // Didn't find it, try it without an IP
    if (INADDR_NONE != key)
    {
      return UserHash::removeFromHash(table, INADDR_NONE, host, user, nick);
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
UserHash::findUsers(BotClient * client, const Filter & filter, ActionPtr action)
  const
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
        (*action)(*pos);
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

    const unsigned int hashIndex = UserHash::hashFunc(lcUser);

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
UserHash::reportClasses(BotClient * client, const std::string & className) const
{
  typedef std::map<std::string, int> UnsortedMap;
  UnsortedMap unsorted;
  std::string::size_type maxLength = 5;

  for (UserEntryTable::const_iterator index = this->domaintable.begin();
      index != this->domaintable.end(); ++index)
  {
    for (UserEntryList::const_iterator userptr = index->begin();
        userptr != index->end(); ++userptr)
    {
      std::string name(server.downCase((*userptr)->getClass()));
      ++unsorted[name];
    }
  }

  if (className.empty())
  {
    if (unsorted.empty())
    {
      client->send("*** No users!");
    }
    else
    {
      std::string header(padRight("Class", maxLength));
      header += "  Count  Description";
      client->send(header);

      typedef std::multimap<int, std::string> SortedMap;
      SortedMap sorted;

      // Sort the list
      for (UnsortedMap::const_iterator pos = unsorted.begin();
          pos != unsorted.end(); ++pos)
      {
        sorted.insert(SortedMap::value_type(pos->second, pos->first));

        std::string::size_type length(pos->first.length());
        if (length > maxLength)
        {
          maxLength = length;
        }
      }

      // Now display class user counts in decreasing order
      for (SortedMap::reverse_iterator pos = sorted.rbegin();
          pos != sorted.rend(); ++pos)
      {
        std::string buffer(padRight(pos->second, maxLength));
        buffer += "  ";
        buffer += padRight(boost::lexical_cast<std::string>(pos->first), 5);
        buffer += "  ";
        buffer += config.classDescription(pos->second);
        client->send(buffer);
      }
    }
  }
  else
  {
    UnsortedMap::const_iterator pos = unsorted.find(server.downCase(className));
    if (pos == unsorted.end())
    {
      client->send("*** No users found with class \"" + className + "\"");
    }
    else
    {
      if (pos->first.length() > maxLength)
      {
        maxLength = pos->first.length();
      }

      std::string header(padRight("Class", maxLength));
      header += "  Count  Description";
      client->send(header);

      std::string buffer(padRight(pos->first, maxLength));
      buffer += "  ";
      buffer += padRight(boost::lexical_cast<std::string>(pos->second), 5);
      buffer += "  ";
      buffer += config.classDescription(pos->first);
      client->send(buffer);
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

  std::list<ScoreNode> scores;

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
	  ScoreNode tmp(*find, (*find)->getScore());
	  scores.push_back(tmp);
        }
      }
    }
  }

  if (!count)
  {
    scores.sort();

    for (std::list<ScoreNode>::const_iterator pos = scores.begin();
      pos != scores.end(); ++pos)
    {
      UserEntryPtr info(pos->getInfo());

      client->send(info->output(UserHash::seedrandFormat));
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
UserHash::reportDomains(BotClient * client, const int minimum) const
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
    for (UserEntryTable::const_iterator i = this->hosttable.begin();
        i != this->hosttable.end(); ++i)
    {
      for (UserEntryList::const_iterator userptr = i->begin();
          userptr != i->end(); ++userptr)
      {
        std::string domain(server.downCase((*userptr)->getDomain()));
        ++unsorted[domain];
      }
    }

    std::string::size_type maxLength(0);

    // Sort the list
    for (UnsortedMap::const_iterator pos = unsorted.begin();
        pos != unsorted.end(); ++pos)
    {
      if (pos->second >= minimum)
      {
        sorted.insert(SortedMap::value_type(pos->second, pos->first));

        if (pos->first.length() > maxLength)
        {
          maxLength = pos->first.length();
        }
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
        std::string buffer("  ");
        buffer += padRight(pos->second, maxLength);
        buffer += "  ";
        buffer += padLeft(boost::lexical_cast<std::string>(pos->first), 3);
        buffer += " user";
        if (pos->first != 1)
        {
          buffer += 's';
        }
        client->send(buffer);
      }
    }
  }
}


void
UserHash::reportNets(BotClient * client, const int minimum) const
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
    for (UserEntryTable::const_iterator i = this->hosttable.begin();
        i != this->hosttable.end(); ++i)
    {
      for (UserEntryList::const_iterator userptr = i->begin();
          userptr != i->end(); ++userptr)
      {
        BotSock::Address ip((*userptr)->getIP());

        if (INADDR_NONE != ip)
        {
          ++unsorted[ip & BotSock::ClassCNetMask];
        }
      }
    }

    // Sort the list
    for (UnsortedMap::const_iterator pos = unsorted.begin();
        pos != unsorted.end(); ++pos)
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
        std::string buffer("  ");
        buffer += padRight(classCMask(BotSock::inet_ntoa(pos->second)), 15);
        buffer += "  ";
        buffer += padLeft(boost::lexical_cast<std::string>(pos->first), 3);
        buffer += " user";
        if (pos->first != 1)
        {
          buffer += 's';
        }
        client->send(buffer);
      }
    }
  }
}


void
UserHash::reportClones(BotClient * client) const
{
  bool foundany = false;

  for (UserEntryTable::const_iterator i = this->hosttable.begin();
      i != this->hosttable.end(); ++i)
  {
    for (UserEntryList::const_iterator userptr = i->begin();
        userptr != i->end(); ++userptr)
    {
      UserEntryList::const_iterator temp = i->begin();
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
              client->send("Possible clones from the following hosts:");
	    }

            boost::format outfmt(
                "  %2d connections in %3ld seconds (%2d total) from %s");
            client->send(str(outfmt % (k + 1) %
                  (connfromhost[j] - connfromhost[j + k]) % numfound %
                  (*userptr)->getHost()));
          }
        }
      }
    }
  }

  if (!foundany)
  {
    client->send("No potential clones found.");
  }
}


void
UserHash::reportVClones(BotClient * client) const
{
  bool foundany = false;

  for (UserEntryTable::const_iterator i = this->iptable.begin();
      i != this->iptable.end(); ++i)
  {
    for (UserEntryList::const_iterator userptr = i->begin();
        userptr != i->end(); ++userptr)
    {
      UserEntryList::const_iterator temp = i->begin();
      for (; temp != userptr; ++temp)
      {
        if (BotSock::sameClassC((*temp)->getIP(), (*userptr)->getIP()) &&
            server.same((*temp)->getUser(), (*userptr)->getUser()))
	{
	  break;
	}
      }

      if (temp == userptr)
      {
        std::vector<std::time_t> connectTime;
	connectTime.reserve(100);

        int numfound = 1;
        connectTime.push_back((*temp)->getConnectTime());

        ++temp;
        while (temp != i->end())
	{
          if (BotSock::sameClassC((*temp)->getIP(), (*userptr)->getIP()) &&
              server.same((*temp)->getUser(), (*userptr)->getUser()))
	  {
	    ++numfound;
            connectTime.push_back((*temp)->getConnectTime());
	  }
          ++temp;
        }

        if (numfound > 2)
	{
	  // sort connect times in decreasing order
	  std::sort(connectTime.begin(), connectTime.end(),
              std::greater<std::time_t>());

	  int j, k;

          for (k = numfound - 1; k > 1; --k)
	  {
            for (j = 0; j < numfound - k; ++j)
	    {
              if ((connectTime[j] > 0) && (connectTime[j + k] > 0) &&
                  ((connectTime[j] - connectTime[j + k]) <=
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
              client->send("Possible vhosted clones from the following users:");
	    }

            boost::format outfmt(
                "  %2d connections in %3ld seconds (%2d total) from %s@%s");
            client->send(str(outfmt % (k + 1) %
                  (connectTime[j] - connectTime[j + k]) % numfound %
                  (*userptr)->getUser() %
                  classCMask(BotSock::inet_ntoa((*userptr)->getIP()))));
          }
        }
      }
    }
  }

  if (!foundany)
  {
    client->send("No potential vhosted clones found.");
  }
}


void
UserHash::reportMulti(BotClient * client, const unsigned int minimum) const
{
  unsigned int minclones((minimum > 0) ? minimum : UserHash::multiMin);
  bool foundAny(false);

  for (UserEntryTable::const_iterator i = this->domaintable.begin();
      i != this->domaintable.end(); ++i)
  {
    typedef std::map<std::string, unsigned int> UnsortedMap;
    UnsortedMap unsorted;

    for (UserEntryList::const_iterator userptr = i->begin();
        userptr != i->end(); ++userptr)
    {
      if (UserHash::operInMulti ||
          (!(*userptr)->getOper() && !config.isOper(*userptr)))
      {
        std::string user((*userptr)->getUser());
        std::string domain((*userptr)->getDomain());
        bool isIP(isNumericIP(domain));

        if (isIP)
        {
          ++unsorted[server.downCase(user + '@' + domain + '*')];
        }
        else
        {
          ++unsorted[server.downCase(user + "@*" + domain)];
        }
      }
    }

    for (UnsortedMap::const_iterator pos = unsorted.begin();
        pos != unsorted.end(); ++pos)
    {
      if (pos->second >= minclones)
      {
        if (!foundAny)
        {
          foundAny = true;
          client->send("Multiple clients from the following userhosts:");
        }
        boost::format outfmt(" %s %2u -- %s");
        client->send(str(outfmt % ((pos->second > minclones) ? "==>" : "   ") %
              pos->second % pos->first));
      }
    }
  }

  if (!foundAny)
  {
    client->send("No multiple logins found.");
  }
}


void
UserHash::reportHMulti(BotClient * client, const unsigned int minimum) const
{
  unsigned int minclones((minimum > 0) ? minimum : UserHash::multiMin);
  bool foundAny(false);

  for (UserEntryTable::const_iterator i = this->hosttable.begin();
      i != this->hosttable.end(); ++i)
  {
    typedef std::map<std::string, unsigned int> UnsortedMap;
    UnsortedMap unsorted;

    for (UserEntryList::const_iterator userptr = i->begin();
        userptr != i->end(); ++userptr)
    {
      if (UserHash::operInMulti ||
          (!(*userptr)->getOper() && !config.isOper(*userptr)))
      {
        ++unsorted[server.downCase((*userptr)->getHost())];
      }
    }

    for (UnsortedMap::const_iterator pos = unsorted.begin();
        pos != unsorted.end(); ++pos)
    {
      if (pos->second >= minclones)
      {
        if (!foundAny)
        {
          foundAny = true;
          client->send("Multiple clients from the following hosts:");
        }
        boost::format outfmt(" %s %2u -- *@%s");
        client->send(str(outfmt % ((pos->second > minclones) ? "==>" : "   ") %
              pos->second % pos->first));
      }
    }
  }

  if (!foundAny)
  {
    client->send("No multiple logins found.");
  }
}


void
UserHash::reportUMulti(BotClient * client, const unsigned int minimum) const
{
  unsigned int minclones((minimum > 0) ? minimum : UserHash::multiMin);
  bool foundAny(false);

  for (UserEntryTable::const_iterator i = this->usertable.begin();
      i != this->usertable.end(); ++i)
  {
    typedef std::map<std::string, unsigned int> UnsortedMap;
    UnsortedMap unsorted;

    for (UserEntryList::const_iterator userptr = i->begin();
        userptr != i->end(); ++userptr)
    {
      if (UserHash::operInMulti ||
          (!(*userptr)->getOper() && !config.isOper(*userptr)))
      {
        ++unsorted[server.downCase((*userptr)->getUser())];
      }
    }

    for (UnsortedMap::const_iterator pos = unsorted.begin();
        pos != unsorted.end(); ++pos)
    {
      if (pos->second >= minclones)
      {
        if (!foundAny)
        {
          foundAny = true;
          client->send("Multiple clients from the following usernames:");
        }
        boost::format outfmt(" %s %2u -- %s@*");
        client->send(str(outfmt % ((pos->second > minclones) ? "==>" : "   ") %
              pos->second % pos->first));
      }
    }
  }

  if (!foundAny)
  {
    client->send("No multiple logins found.");
  }
}


void
UserHash::reportVMulti(BotClient * client, const unsigned int minimum) const
{
  unsigned int minclones((minimum > 0) ? minimum : UserHash::multiMin);
  bool foundAny(false);

  for (UserEntryTable::const_iterator i = this->iptable.begin();
      i != this->iptable.end(); ++i)
  {
    typedef std::map<std::string, unsigned int> UnsortedMap;
    UnsortedMap unsorted;

    for (UserEntryList::const_iterator userptr = i->begin();
        userptr != i->end(); ++userptr)
    {
      if (UserHash::operInMulti ||
          (!(*userptr)->getOper() && !config.isOper(*userptr)))
      {
        std::string user((*userptr)->getUser());
        std::string IP((*userptr)->getTextIP());
        std::string::size_type lastDot = IP.rfind('.');
        if (std::string::npos != lastDot)
        {
          IP = IP.substr(0, lastDot);
        }

        ++unsorted[server.downCase(user + '@' + IP + ".*")];
      }
    }

    for (UnsortedMap::const_iterator pos = unsorted.begin();
        pos != unsorted.end(); ++pos)
    {
      if (pos->second >= minclones)
      {
        if (!foundAny)
        {
          foundAny = true;
          client->send("Multiple clients from the following vhosts:");
        }
        boost::format outfmt(" %s %2u -- %s");
        client->send(str(outfmt % ((pos->second > minclones) ? "==>" : "   ") %
              pos->second % pos->first));
      }
    }
  }

  if (!foundAny)
  {
    client->send("No multiple logins found.");
  }
}


void
UserHash::checkHostClones(const std::string & host)
{
  const unsigned int index = UserHash::hashFunc(host);

  std::time_t now = std::time(0);
  std::time_t oldest = now;
  std::time_t lastReport = 0;

  int cloneCount = 0;
  int reportedClones = 0;

  for (UserEntryList::iterator find = this->hosttable[index].begin();
    find != this->hosttable[index].end(); ++find)
  {
    if (server.same((*find)->getHost(), host) &&
        ((now - (*find)->getConnectTime()) <= UserHash::cloneMaxTime))
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

  if (((reportedClones == 0) && (cloneCount < UserHash::cloneMinCount)) ||
      ((now - lastReport) < UserHash::cloneReportInterval))
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
        ((now - (*find)->getConnectTime()) <= UserHash::cloneMaxTime) &&
        ((*find)->getReportTime() == 0))
    {
      ++cloneCount;

#ifdef USERHASH_DEBUG
      std::cout << "clone(" << cloneCount << "): " <<
	(*find)->output("%n %@ %i") << std::endl;
#endif /* USERHASH_DEBUG */

      std::string notice;
      if (cloneCount == 1)
      {
	notice1 = (*find)->output(UserHash::cloneReportFormat);
      }
      else
      {
	notice = (*find)->output(UserHash::cloneReportFormat);
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
          (*find)->getIP(), (*find)->getClass(), differentUser, false,
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


void
UserHash::checkIpClones(const BotSock::Address & ip)
{
  BotSock::Address subnet(ip & BotSock::ClassCNetMask);
  const unsigned int index = UserHash::hashFunc(ip);

  std::time_t now = std::time(0);
  std::time_t oldest = now;
  std::time_t lastReport = 0;

  int cloneCount = 0;
  int reportedClones = 0;

  for (UserEntryList::iterator find = this->iptable[index].begin();
    find != this->iptable[index].end(); ++find)
  {
    if (((*find)->getSubnet() == subnet) &&
        ((now - (*find)->getConnectTime()) <= UserHash::cloneMaxTime))
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

  if (((reportedClones == 0) && (cloneCount < UserHash::cloneMinCount)) ||
      ((now - lastReport) < UserHash::cloneReportInterval))
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
    if (((*find)->getSubnet() == subnet) && ((*find)->getReportTime() == 0) &&
        ((now - (*find)->getConnectTime()) <= UserHash::cloneMaxTime))
    {
      ++cloneCount;

#ifdef USERHASH_DEBUG
      std::cout << "clone(" << cloneCount << "): " <<
	(*find)->output("%n %@ %i") << std::endl;
#endif /* USERHASH_DEBUG */

      std::string notice;
      if (cloneCount == 1)
      {
	notice1 = (*find)->output(UserHash::cloneReportFormat);
      }
      else
      {
	notice = (*find)->output(UserHash::cloneReportFormat);
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
          (*find)->getIP(), (*find)->getClass(), differentUser, differentIp,
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


#ifdef USERHASH_DEBUG
void
UserHash::debugStatus(BotClient * client, const UserEntryTable & table,
    const std::string & label, const int userCount)
{
  int tableSize = table.size();
  if (tableSize > 0)
  {
    double average = userCount / tableSize;
    int max = 0;
    int empty = 0;
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
      if (0 == size)
      {
        ++empty;
      }
    }

    std::string notice(label);
    notice += " stddev: ";
    notice += boost::lexical_cast<std::string>(sqrt(square));
    notice += " (+";
    notice += boost::lexical_cast<std::string>(max);
    notice += ", -";
    notice += boost::lexical_cast<std::string>(empty);
    notice += ")";
    client->send(notice);
  }
}
#endif /* USERHASH_DEBUG */


void
UserHash::status(BotClient * client)
{
  client->send("Users: " + boost::lexical_cast<std::string>(this->userCount));
#ifdef USERHASH_DEBUG
  client->send("userEntryCount: " +
    boost::lexical_cast<std::string>(userEntryCount));
#endif /* USERHASH_DEBUG */

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

#ifdef USERHASH_DEBUG
  this->debugStatus(client, this->usertable, "usertable", this->userCount);
  this->debugStatus(client, this->hosttable, "hosttable", this->userCount);
  this->debugStatus(client, this->domaintable, "domaintable", this->userCount);
  this->debugStatus(client, this->iptable, "iptable", this->userCount);
#endif /* USERHASH_DEBUG */
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


void
UserHash::init(void)
{
  UserEntry::init();

  vars.insert("CLONE_MAX_TIME",
      Setting::IntegerSetting(UserHash::cloneMaxTime, 1));
  vars.insert("CLONE_MIN_COUNT",
      Setting::IntegerSetting(UserHash::cloneMinCount, 2));
  vars.insert("CLONE_REPORT_FORMAT",
      Setting::StringSetting(UserHash::cloneReportFormat));
  vars.insert("CLONE_REPORT_INTERVAL",
      Setting::IntegerSetting(UserHash::cloneReportInterval, 1));
  vars.insert("CTCPVERSION_ENABLE",
      Setting::BooleanSetting(UserHash::ctcpversionEnable));
  vars.insert("CTCPVERSION_TIMEOUT",
      Setting::IntegerSetting(UserHash::ctcpversionTimeout, 0));
  vars.insert("CTCPVERSION_TIMEOUT_ACTION",
      AutoAction::Setting(UserHash::ctcpversionTimeoutAction));
  vars.insert("CTCPVERSION_TIMEOUT_REASON",
      Setting::StringSetting(UserHash::ctcpversionTimeoutReason));
  vars.insert("MULTI_MIN", Setting::IntegerSetting(UserHash::multiMin, 1));
  vars.insert("OPER_IN_MULTI",
      Setting::BooleanSetting(UserHash::operInMulti));
  vars.insert("SEEDRAND_ACTION", AutoAction::Setting(UserHash::seedrandAction));
  vars.insert("SEEDRAND_FORMAT",
      Setting::StringSetting(UserHash::seedrandFormat));
  vars.insert("SEEDRAND_REASON",
      Setting::StringSetting(UserHash::seedrandReason));
  vars.insert("SEEDRAND_REPORT_MIN",
      Setting::IntegerSetting(UserHash::seedrandReportMin));
  vars.insert("TRAP_CONNECTS", Setting::BooleanSetting(UserHash::trapConnects));
  vars.insert("TRAP_CTCP_VERSIONS",
      Setting::BooleanSetting(UserHash::trapCtcpVersions));
  vars.insert("TRAP_NICK_CHANGES",
      Setting::BooleanSetting(UserHash::trapNickChanges));
  vars.insert("TRAP_NOTICES",
      Setting::BooleanSetting(UserHash::trapNotices));
  vars.insert("TRAP_PRIVMSGS",
      Setting::BooleanSetting(UserHash::trapPrivmsgs));
}

