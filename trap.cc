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
  : action_(action), timeout_(timeout), lastMatch_(0), matchCount_(0)
{
  if ((line.length() > 0) && (line[0] == '/'))
  {
    // Broken format! :(
    this->reason_ = line;
    std::string pattern = grabPattern(this->reason_);
    this->reason_ = trimLeft(this->reason_);

    this->rePattern_.reset(new RegExPattern(pattern));
  }
  else
  {
    std::string copy = line;

    if (Trap::parsePattern(copy, this->nick_, this->userhost_, this->gecos_,
      this->version_, this->privmsg_, this->notice_))
    {
      this->reason_ = trimLeft(copy);
    }
    else
    {
      copy = line;

      this->nick_.reset();
      this->userhost_.reset();
      this->gecos_.reset();
      this->version_.reset();
      this->privmsg_.reset();
      this->notice_.reset();

      std::string pattern = FirstWord(copy);
      std::string nick, userhost;
      Trap::split(pattern, nick, userhost);

      if (nick != "*")
      {
        this->nick_.reset(new NickClusterPattern(nick));
      }
      if (userhost != "*@*")
      {
        this->userhost_.reset(new ClusterPattern(userhost));
      }
      this->reason_ = copy;
    }
  }
}


Trap::Trap(const Trap & copy)
  : action_(copy.action_), timeout_(copy.timeout_), nick_(copy.nick_),
  userhost_(copy.userhost_), gecos_(copy.gecos_), version_(copy.version_),
  privmsg_(copy.privmsg_), notice_(copy.notice_), rePattern_(copy.rePattern_),
  reason_(copy.reason_), lastMatch_(copy.lastMatch_),
  matchCount_(copy.matchCount_)
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


bool
Trap::matches(const UserEntryPtr user, const std::string & version,
  const std::string & privmsg, const std::string & notice) const
{
  if (NULL != this->rePattern_)
  {
    return (this->rePattern_->match(user->getNickUserHost()) ||
      this->rePattern_->match(user->getNickUserIP()));
  }
  else
  {
    if ((this->nick_ && !this->nick_->match(user->getNick())) ||
      (this->gecos_ && !this->gecos_->match(user->getGecos())) ||
      (this->version_ && !this->version_->match(version)) ||
      (this->privmsg_ && !this->privmsg_->match(privmsg)) ||
      (this->notice_ && !this->notice_->match(notice)) ||
      (this->userhost_ && !this->userhost_->match(user->getUserHost()) &&
      !this->userhost_->match(user->getUserIP())))
    {
      return false;
    }

    return true;
  }
}


bool
Trap::operator==(const Trap & other) const
{
  if (this->rePattern_)
  {
    if (other.rePattern_)
    {
      return (other.rePattern_->get() == this->rePattern_->get());
    }
    else
    {
      return false;
    }
  }
  else
  {
    if (this->nick_)
    {
      if (!other.nick_ || (this->nick_->get() != other.nick_->get()))
      {
	return false;
      }
    }
    else
    {
      if (other.nick_)
      {
	return false;
      }
    }
    if (this->userhost_)
    {
      if (!other.userhost_ ||
	(this->userhost_->get() != other.userhost_->get()))
      {
	return false;
      }
    }
    else
    {
      if (other.userhost_)
      {
	return false;
      }
    }
    if (this->gecos_)
    {
      if (!other.gecos_ || (this->gecos_->get() != other.gecos_->get()))
      {
	return false;
      }
    }
    else
    {
      if (other.gecos_)
      {
	return false;
      }
    }
    if (this->version_)
    {
      if (!other.version_ || (this->version_->get() != other.version_->get()))
      {
	return false;
      }
    }
    else
    {
      if (other.version_)
      {
	return false;
      }
    }
    if (this->privmsg_)
    {
      if (!other.privmsg_ || (this->privmsg_->get() != other.privmsg_->get()))
      {
	return false;
      }
    }
    else
    {
      if (other.privmsg_)
      {
	return false;
      }
    }
    if (this->notice_)
    {
      if (!other.notice_ || (this->notice_->get() != other.notice_->get()))
      {
	return false;
      }
    }
    else
    {
      if (other.notice_)
      {
	return false;
      }
    }
    return true;
  }
}


bool
Trap::operator==(const std::string & pattern) const
{
  return (this->getPattern() == pattern);
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

  if (showTime && (0 != this->getLastMatch()))
  {
    result += timeStamp(TIMESTAMP_LOG, this->getLastMatch());
    result += ' ';
  }

  result += TrapList::actionString(this->getAction()) + ": ";
  TrapAction action = this->getAction();
  switch (action)
  {
    case TRAP_ECHO:
      result += this->getPattern();
      break;
    case TRAP_KILL:
      result += this->getPattern() + " (" + this->getReason() + ")";
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
        result += boost::lexical_cast<std::string>(this->getTimeout());
	result += ' ';
	result += this->getPattern();
	result += " (";
	result += this->getReason();
	result += ')';
      }
      else
      {
        result += this->getPattern();
	result += " (";
	result += this->getReason();
	result += ')';
      }
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



std::string
Trap::getPattern(void) const
{
  std::string result;

  if (NULL != this->rePattern_)
  {
    result = this->rePattern_->get();
  }
  else
  {
    if (this->nick_)
    {
      result = "nick=" + this->nick_->get();
    }
    if (this->userhost_)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "userhost=" + this->userhost_->get();
    }
    if (this->gecos_)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "gecos=" + this->gecos_->get();
    }
    if (this->version_)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "version=";
      result += this->version_->get();
    }
    if (this->privmsg_)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "privmsg=";
      result += this->privmsg_->get();
    }
    if (this->notice_)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "notice=";
      result += this->notice_->get();
    }
  }
  return result;
}



void
Trap::split(const std::string & pattern, std::string & nick,
  std::string & userhost)
{
  std::string::size_type bang = pattern.find('!');
  std::string::size_type at = pattern.find('@');

  if (bang != std::string::npos)
  {
    nick = pattern.substr(0, bang);
    userhost = pattern.substr(bang + 1, std::string::npos);
  }
  else if (at != std::string::npos)
  {
    nick = "*";
    userhost = pattern;
  }
  else
  {
    nick = pattern;
    userhost = "*@*";
  }
}


void
Trap::doAction(const UserEntryPtr user) const
{
  std::string notice("*** Trapped: ");
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
TrapList::add(const unsigned int key, const TrapAction action,
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
      break;
    }
  }
  if (pos == end)
  {
    traps.insert(std::make_pair(key ? key : TrapList::getMaxKey() + 100, trap));
  }

  return trap;
}


unsigned int
TrapList::getMaxKey(void)
{
  TrapMap::reverse_iterator last = traps.rbegin();
  unsigned int result = 0;

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
        unsigned int key = boost::lexical_cast<unsigned int>(val);
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
          std::string notice(client->handleAndBot());
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
    unsigned int key = 0;

    try
    {
      key = boost::lexical_cast<unsigned int>(flag);
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
          std::string notice(client->handleAndBot());
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
    }
    else
    {
      client->send("*** You don't have access to add traps!");
    }
  }

  if (modified && (0 != client->id().compare("CONFIG")) &&
    vars[VAR_AUTO_SAVE]->getBool())
  {
    Config::saveSettings();
  }
}


bool
TrapList::remove(const unsigned int key)
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
  if (!Config::IsOKHost(user->getUserHost(), user->getIP()) &&
    !Config::IsOper(user->getUserHost(), user->getIP()))
  {
    for (TrapMap::iterator pos = traps.begin(); pos != traps.end(); ++pos)
    {
      try
      {
        if (pos->second.matches(user, version, privmsg, notice))
        {
	  pos->second.updateStats();
	  pos->second.doAction(user);
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
      pos->second.getTimeout() << " " << pos->second.getPattern() << " " <<
      pos->second.getReason() << std::endl;
  }
}


bool
Trap::parsePattern(std::string & pattern, PatternPtr & nick,
  PatternPtr & userhost, PatternPtr & gecos, PatternPtr & version,
  PatternPtr & privmsg, PatternPtr & notice)
{
  while (!pattern.empty())
  {
    if ((pattern.length() >= 5) && Same("nick=", pattern.substr(0, 5)))
    {
      pattern.erase(0, 5);
      std::string temp = grabPattern(pattern, " ,");
      nick = smartPattern(temp, true);
    }
    else if ((pattern.length() >= 9) && Same("userhost=", pattern.substr(0, 9)))
    {
      pattern.erase(0, 9);
      std::string temp = grabPattern(pattern, " ,");
      userhost = smartPattern(temp, false);
    }
    else if ((pattern.length() >= 6) && Same("gecos=", pattern.substr(0, 6)))
    {
      pattern.erase(0, 6);
      std::string temp = grabPattern(pattern, " ,");
      gecos = smartPattern(temp, false);
    }
    else if ((pattern.length() >= 8) && Same("version=", pattern.substr(0, 8)))
    {
      pattern.erase(0, 8);
      std::string temp = grabPattern(pattern, " ,");
      version = smartPattern(temp, false);
    }
    else if ((pattern.length() >= 8) && Same("privmsg=", pattern.substr(0, 8)))
    {
      pattern.erase(0, 8);
      std::string temp = grabPattern(pattern, " ,");
      privmsg = smartPattern(temp, false);
    }
    else if ((pattern.length() >= 7) && Same("notice=", pattern.substr(0, 7)))
    {
      pattern.erase(0, 7);
      std::string temp = grabPattern(pattern, " ,");
      notice = smartPattern(temp, false);
    }
    else
    {
      // WTF?
      return false;
    }
    if (!pattern.empty())
    {
      if (pattern[0] == ' ')
      {
	return true;
      }
      else if (pattern[0] == ',')
      {
	pattern.erase(0, 1);
      }
    }
  }
  return true;
}

