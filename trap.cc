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
#include <fstream>
#include <string>
#include <ctime>

// OOMon Headers
#include "strtype"
#include "botexcept.h"
#include "botclient.h"
#include "trap.h"
#include "util.h"
#include "irc.h"
#include "log.h"
#include "main.h"
#include "config.h"
#include "vars.h"
#include "pattern.h"
#include "arglist.h"
#include "userentry.h"


TrapList::TrapMap TrapList::traps;


Trap::Trap(const TrapAction action, const long timeout,
  const std::string & line)
  : action_(action), timeout_(timeout), filter_(line, Filter::FIELD_NUH),
  lastMatch_(0), matchCount_(0), loaded_(false)
{
  std::string rest = this->filter_.rest();
  this->reason_ = trimLeft(rest);
}


Trap::Trap(const Trap & copy)
  : action_(copy.action_), timeout_(copy.timeout_), filter_(copy.filter_),
  reason_(copy.reason_), lastMatch_(copy.lastMatch_),
  matchCount_(copy.matchCount_), loaded_(copy.loaded_)
{
}

Trap::~Trap(void)
{
}


void
Trap::updateStats(void)
{
  ++this->matchCount_;
  this->lastMatch_ = std::time(NULL);
}


void
Trap::update(const TrapAction action, const long timeout,
  const std::string & reason)
{
  if ((this->action_ != action) || (this->timeout_ != timeout) ||
      (this->reason_ != reason))
  {
    this->matchCount_ = 0;
    this->lastMatch_ = 0;

    this->action_ = action;
    this->timeout_ = timeout;
    this->reason_ = reason;
  }
}


bool
Trap::matches(const UserEntryPtr user, const std::string & version,
  const std::string & privmsg, const std::string & notice) const
{
  return this->filter_.matches(user, version, privmsg, notice);
}


bool
Trap::operator==(const Trap & other) const
{
  return (0 == this->getFilter().compare(other.getFilter()));
}


bool
Trap::operator==(const std::string & pattern) const
{
  return (0 == this->getFilter().compare(pattern));
}


std::string
Trap::getString(bool showCount, bool showTime) const
{
  std::string result;

  if (showCount)
  {
    result += boost::lexical_cast<std::string>(this->getMatchCount());
    result += ' ';
  }

  if (showTime)
  {
    if (0 != this->getLastMatch())
    {
      result += timeStamp(TIMESTAMP_LOG, this->getLastMatch());
      result += ' ';
    }
    else
    {
      result += "never ";
    }
  }

  result += TrapList::actionString(this->getAction());
  TrapAction action = this->getAction();
  switch (action)
  {
    case TRAP_ECHO:
      result += ": ";
      result += this->filter_.get();
      break;
    case TRAP_KILL:
      result += ": ";
      result += this->filter_.get();
      result += " (";
      result += this->getReason();
      result += ')';
      break;
    case TRAP_DLINE_IP:
    case TRAP_DLINE_NET:
    case TRAP_KLINE:
    case TRAP_KLINE_HOST:
    case TRAP_KLINE_DOMAIN:
    case TRAP_KLINE_IP:
    case TRAP_KLINE_USERNET:
    case TRAP_KLINE_NET:
      if (this->getTimeout() > 0)
      {
        result += ' ';
        result += boost::lexical_cast<std::string>(this->getTimeout());
      }
      result += ": ";
      result += this->filter_.get();
      result += " (";
      result += this->getReason();
      result += ')';
      break;
    default:
      result = "Unknown TRAP type!";
      break;
  }

  return result;
}


std::string
TrapList::actionString(const TrapAction & action)
{
  switch (action)
  {
    case TRAP_ECHO:
      return "ECHO";
    case TRAP_KILL:
      return "KILL";
    case TRAP_KLINE:
      return "KLINE";
    case TRAP_KLINE_HOST:
      return "KLINE_HOST";
    case TRAP_KLINE_DOMAIN:
      return "KLINE_DOMAIN";
    case TRAP_KLINE_IP:
      return "KLINE_IP";
    case TRAP_KLINE_USERNET:
      return "KLINE_USERNET";
    case TRAP_KLINE_NET:
      return "KLINE_NET";
    case TRAP_DLINE_IP:
      return "DLINE_IP";
    case TRAP_DLINE_NET:
      return "DLINE_NET";
  }
  return "";
}


TrapAction
TrapList::actionType(const std::string & text)
{
  if (Same("ECHO", text))
    return TRAP_ECHO;
  else if (Same("KILL", text))
    return TRAP_KILL;
  else if (Same("KLINE", text) || Same("KLINE_USERDOMAIN", text))
    return TRAP_KLINE;
  else if (Same("KLINE_HOST", text) || Same("KLINEHOST", text))
    return TRAP_KLINE_HOST;
  else if (Same("KLINE_DOMAIN", text))
    return TRAP_KLINE_DOMAIN;
  else if (Same("KLINE_IP", text))
    return TRAP_KLINE_IP;
  else if (Same("KLINE_USERNET", text))
    return TRAP_KLINE_USERNET;
  else if (Same("KLINE_NET", text))
    return TRAP_KLINE_NET;
  else if (Same("DLINE_IP", text) || Same("DLINE", text))
    return TRAP_DLINE_IP;
  else if (Same("DLINE_NET", text))
    return TRAP_DLINE_NET;

  throw OOMon::invalid_action(text);
}


void
Trap::doAction(const TrapKey key, const UserEntryPtr user) const
{
  std::string notice("*** Trapped ");
  notice += boost::lexical_cast<std::string>(key);
  notice += ": ";
  notice += user->getNickUserHost();
  notice += " [";
  notice += user->getTextIP();
  notice += "]";

  Log::Write(notice);
  switch (this->getAction())
  {
    case TRAP_ECHO:
      ::SendAll(notice, UserFlags::OPER, WATCH_TRAPS);
      break;
    case TRAP_KILL:
      server.kill("Auto-Kill", user->getNick(), this->getReason());
      break;
    case TRAP_KLINE:
      server.kline("Auto-Kline", this->getTimeout(),
	klineMask(user->getUserHost()), this->getReason());
      break;
    case TRAP_KLINE_HOST:
      {
	server.kline("Auto-Kline", this->getTimeout(), "*@" + user->getHost(),
	  this->getReason());
      }
      break;
    case TRAP_KLINE_DOMAIN:
      {
	std::string domain = getDomain(user->getHost(), true);
	server.kline("Auto-Kline", this->getTimeout(), "*@" + domain,
	  this->getReason());
      }
      break;
    case TRAP_KLINE_IP:
      if (INADDR_NONE != user->getIP())
      {
        server.kline("Auto-Kline", this->getTimeout(), "*@" + user->getTextIP(),
          this->getReason());
      }
      else
      {
        std::cerr << "Missing or invalid IP address -- unable to KLINE!" <<
	  std::endl;
      }
      break;
    case TRAP_KLINE_USERNET:
      {
	std::string username = user->getUser();
	if ((username.length() > 1) && (username[0] == '~'))
	{
	  username = username.substr(1);
	}
	if (INADDR_NONE != user->getIP())
	{
	  server.kline("Auto-Kline", this->getTimeout(), "*" + username + "@" +
	    classCMask(user->getTextIP()), this->getReason());
	}
	else
	{
	  std::cerr << "Missing or invalid IP address -- unable to KLINE!" <<
	    std::endl;
	}
      }
      break;
    case TRAP_KLINE_NET:
      if (INADDR_NONE != user->getIP())
      {
        server.kline("Auto-Kline", this->getTimeout(), "*@" +
	  classCMask(user->getTextIP()), this->getReason());
      }
      else
      {
        std::cerr << "Missing or invalid IP address -- unable to KLINE!" <<
          std::endl;
      }
      break;
    case TRAP_DLINE_IP:
      if (INADDR_NONE != user->getIP())
      {
        server.dline("Auto-Dline", this->getTimeout(), user->getTextIP(),
	  this->getReason());
      }
      else
      {
        std::cerr << "Missing or invalid IP address -- unable to DLINE!" <<
	  std::endl;
      }
      break;
    case TRAP_DLINE_NET:
      if (INADDR_NONE != user->getIP())
      {
        server.dline("Auto-Dline", this->getTimeout(),
	  classCMask(user->getTextIP()), this->getReason());
      }
      else
      {
        std::cerr << "Missing or invalid IP address -- unable to DLINE!" <<
          std::endl;
      }
      break;
    default:
      std::cerr << "Unknown Trap type!" << std::endl;
      break;
  }
}


Trap
TrapList::add(const TrapKey key, const TrapAction action,
  const long timeout, const std::string & line)
{
  Trap trap(action, timeout, line);

  const TrapMap::iterator begin = key ? traps.lower_bound(key) : traps.begin();
  const TrapMap::iterator end = key ? traps.upper_bound(key) : traps.end();
  TrapMap::iterator pos;
  for (pos = begin; pos != end; ++pos)
  {
    if (pos->second == trap)
    {
      pos->second.update(action, timeout, trap.getReason());
      break;
    }
  }
  if (pos == end)
  {
    pos = traps.insert(std::make_pair(key ? key : TrapList::getMaxKey() + 100,
      trap));
  }

  pos->second.loaded(true);

  return pos->second;
}


TrapKey
TrapList::getMaxKey(void)
{
  TrapMap::reverse_iterator last = traps.rbegin();
  TrapKey result = 0;

  if (last != traps.rend())
  {
    result = last->first;
  }

  return result;
}


void
TrapList::cmd(BotClient * client, std::string line)
{
  ArgList args("-a -t", "");

  if (-1 == args.parseCommand(line))
  {
    client->send("*** Invalid parameter: " + args.getInvalid());
    return;
  }

  bool showCounts = args.haveUnary("-a");
  bool showTimes = args.haveUnary("-t");

  std::string flag = UpCase(FirstWord(line));
  long timeout = 0;
  bool modified = false;

  if (flag == "" || flag == "LIST")
  {
    TrapList::list(client, showCounts, showTimes);
  }
  else if (flag == "REMOVE")
  {
    if (client->flags().has(UserFlags::MASTER))
    {
      std::string copy = line;
      std::string val = FirstWord(copy);
      bool success = false;
      try
      {
        TrapKey key = boost::lexical_cast<TrapKey>(val);
        success = TrapList::remove(key);
      }
      catch (boost::bad_lexical_cast)
      {
        success = TrapList::remove(line);
      }
      if (success)
      {
        modified = true;

        if (0 != client->id().compare("CONFIG"))
        {
          std::string notice("*** ");
          notice += client->handleAndBot();
          notice += " removed trap: ";
          notice += line;
          ::SendAll(notice, UserFlags::OPER, WATCH_TRAPS);
          Log::Write(notice);
        }
      }
      else
      {
        client->send(std::string("*** No such trap: ") + line);
      }
    }
    else
    {
      client->send("*** You don't have access to remove traps!");
    }
  }
  else // command == "ADD"
  {
    TrapKey key = 0;

    try
    {
      key = boost::lexical_cast<TrapKey>(flag);
      flag = UpCase(FirstWord(line));
    }
    catch (boost::bad_lexical_cast)
    {
      // user didn't specify a key
    }
    std::string copy = line;
    std::string aword = FirstWord(copy);
    if (isNumeric(aword))
    {
      timeout = atol(aword.c_str());
      line = copy;
    }

    if (client->flags().has(UserFlags::MASTER))
    {
      try
      {
        TrapAction actionType = TrapList::actionType(flag);

        Trap node = TrapList::add(key, actionType, timeout, line);

        modified = true;

        if (0 != client->id().compare("CONFIG"))
        {
          std::string notice("*** ");
          notice += client->handleAndBot();
	  notice +=" added trap ";
	  notice += node.getString();
          ::SendAll(notice, UserFlags::OPER, WATCH_TRAPS);
          Log::Write(notice);
        }
      }
      catch (OOMon::invalid_action & e)
      {
        client->send("*** Invalid TRAP action: " + e.what());
      }
      catch (OOMon::regex_error & e)
      {
        client->send("*** RegEx error: " + e.what());
      }
      catch (Filter::bad_field & e)
      {
        client->send(e.what());
      }
    }
    else
    {
      client->send("*** You don't have access to add traps!");
    }
  }

  if (modified && (0 != client->id().compare("CONFIG")) &&
    vars[VAR_AUTO_SAVE]->getBool())
  {
    config.saveSettings();
  }
}


bool
TrapList::remove(const TrapKey key)
{
  const TrapMap::iterator begin = traps.lower_bound(key);
  const TrapMap::iterator end = traps.upper_bound(key);

  traps.erase(begin, end);

  return (begin != end);
}

bool
TrapList::remove(const std::string & pattern)
{
  bool found = false;

  for (TrapMap::iterator pos = traps.begin(); pos != traps.end(); )
  {
    if (pos->second == pattern)
    {
      found = true;
      traps.erase(pos);
      break;
    }
    else
    {
      ++pos;
    }
  }

  return found;
}

void
TrapList::match(const UserEntryPtr user, const std::string & version,
  const std::string & privmsg, const std::string & notice)
{
  if (!user->getOper() && !config.isExcluded(user) && !config.isOper(user))
  {
    for (TrapMap::iterator pos = traps.begin(); pos != traps.end(); ++pos)
    {
      try
      {
        if (pos->second.matches(user, version, privmsg, notice))
        {
	  pos->second.updateStats();
	  pos->second.doAction(pos->first, user);
        }
      }
      catch (OOMon::regex_error & e)
      {
        std::cerr << "RegEx error in TrapList::match(): " + e.what() <<
	  std::endl;
        Log::Write("RegEx error in TrapList::match(): " + e.what());
      }
    }
  }
}

void
TrapList::match(const UserEntryPtr user)
{
  TrapList::match(user, "", "", "");
}

void
TrapList::matchCtcpVersion(const UserEntryPtr user, const std::string & version)
{
  TrapList::match(user, version, "", "");
}

void
TrapList::matchPrivmsg(const UserEntryPtr user, const std::string & privmsg)
{
  TrapList::match(user, "", privmsg, "");
}

void
TrapList::matchNotice(const UserEntryPtr user, const std::string & notice)
{
  TrapList::match(user, "", "", notice);
}

void
TrapList::list(BotClient * client, bool showCounts, bool showTimes)
{
  int count = 0;

  for (TrapMap::iterator pos = traps.begin(); pos != traps.end(); ++pos)
  {
    std::string text(boost::lexical_cast<std::string>(pos->first));
    text += ' ';
    text += pos->second.getString(showCounts, showTimes);
    client->send(text);
    ++count;
  }
  if (count > 0)
  {
    client->send("*** End of TRAP list.");
  }
  else
  {
    client->send("*** TRAP list is empty!");
  }
}


void
TrapList::save(std::ofstream & file)
{
  for (TrapMap::iterator pos = TrapList::traps.begin();
    pos != TrapList::traps.end(); ++pos)
  {
    file << "TRAP " << pos->first << " " <<
      TrapList::actionString(pos->second.getAction()) << " " <<
      pos->second.getTimeout() << " " << pos->second.getFilter() << " " <<
      pos->second.getReason() << std::endl;
  }
}


void
TrapList::preLoad(void)
{
  for (TrapMap::iterator pos = TrapList::traps.begin();
    pos != TrapList::traps.end(); ++pos)
  {
    pos->second.loaded(false);
  }
}


void
TrapList::postLoad(void)
{
  TrapMap copy;

  for (TrapMap::const_iterator pos = traps.begin(); pos != traps.end(); ++pos)
  {
    if (pos->second.loaded())
    {
      copy.insert(*pos);
    }
  }

  traps.swap(copy);
}

