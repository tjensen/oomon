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

    this->nuh_.reset(new RegExPattern(pattern));
  }
  else
  {
    std::string copy = line;

    if (this->parsePattern(copy))
    {
      this->reason_ = trimLeft(copy);
    }
    else
    {
      copy = line;

      this->n_.reset();
      this->u_.reset();
      this->h_.reset();
      this->uh_.reset();
      this->nuh_.reset();
      this->g_.reset();
      this->nuhg_.reset();
      this->c_.reset();
      this->version_.reset();
      this->privmsg_.reset();
      this->notice_.reset();

      std::string pattern = FirstWord(copy);
      std::string nick, userhost;
      Trap::split(pattern, nick, userhost);

      if (nick != "*")
      {
        this->n_.reset(new NickClusterPattern(nick));
      }
      if (userhost != "*@*")
      {
        this->uh_.reset(new ClusterPattern(userhost));
      }
      this->reason_ = copy;
    }
  }
}


Trap::Trap(const Trap & copy)
  : action_(copy.action_), timeout_(copy.timeout_), n_(copy.n_),
  u_(copy.u_), h_(copy.h_), uh_(copy.uh_), nuh_(copy.nuh_), g_(copy.g_),
  nuhg_(copy.nuhg_), c_(copy.c_), version_(copy.version_),
  privmsg_(copy.privmsg_), notice_(copy.notice_), reason_(copy.reason_),
  lastMatch_(copy.lastMatch_), matchCount_(copy.matchCount_)
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
  if (this->n_ && !this->n_->match(user->getNick()))
  {
    return false;
  }
  if (this->u_ && !this->u_->match(user->getUser()))
  {
    return false;
  }
  if (this->h_ && !this->h_->match(user->getHost()) &&
      !this->h_->match(user->getTextIP()))
  {
    return false;
  }
  if (this->uh_ && !this->uh_->match(user->getUserHost()) &&
      !this->uh_->match(user->getUserIP()))
  {
    return false;
  }
  if (this->nuh_ && !this->nuh_->match(user->getNickUserHost()) &&
      !this->nuh_->match(user->getNickUserIP()))
  {
    return false;
  }
  if (this->g_ && !this->g_->match(user->getGecos()))
  {
    return false;
  }
  if (this->nuhg_ && !this->nuhg_->match(user->getNickUserHostGecos()) &&
      !this->nuhg_->match(user->getNickUserIPGecos()))
  {
    return false;
  }
  if (this->c_ && !this->c_->match(user->getClass()))
  {
    return false;
  }
  if (this->version_ && !this->version_->match(version))
  {
    return false;
  }
  if (this->privmsg_ && !this->privmsg_->match(privmsg))
  {
    return false;
  }
  if (this->notice_ && !this->notice_->match(notice))
  {
    return false;
  }
  return true;
}


bool
Trap::patternsEqual(const PatternPtr & left, const PatternPtr & right)
{
  bool result = true;

  if (left && right)
  {
    if (left->get() != right->get())
    {
      result = false;
    }
  }
  else if (left || right)
  {
    result = false;
  }

  return result;
}


bool
Trap::operator==(const Trap & other) const
{
  return (Trap::patternsEqual(this->n_, other.n_) &&
      Trap::patternsEqual(this->u_, other.u_) &&
      Trap::patternsEqual(this->h_, other.h_) &&
      Trap::patternsEqual(this->uh_, other.uh_) &&
      Trap::patternsEqual(this->nuh_, other.nuh_) &&
      Trap::patternsEqual(this->g_, other.g_) &&
      Trap::patternsEqual(this->nuhg_, other.nuhg_) &&
      Trap::patternsEqual(this->c_, other.c_) &&
      Trap::patternsEqual(this->version_, other.version_) &&
      Trap::patternsEqual(this->privmsg_, other.privmsg_) &&
      Trap::patternsEqual(this->notice_, other.notice_));
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
      result += this->getPattern();
      break;
    case TRAP_KILL:
      result += ": ";
      result += this->getPattern();
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
      result += this->getPattern();
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



std::string
Trap::getPattern(void) const
{
  std::string result;

  if (this->n_)
  {
    result = "n=" + this->n_->get();
  }
  if (this->u_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "u=" + this->u_->get();
  }
  if (this->h_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "h=" + this->h_->get();
  }
  if (this->uh_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "uh=" + this->uh_->get();
  }
  if (this->nuh_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "nuh=" + this->nuh_->get();
  }
  if (this->g_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "g=" + this->g_->get();
  }
  if (this->nuhg_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "nuhg=" + this->nuhg_->get();
  }
  if (this->c_)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += "c=" + this->c_->get();
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
      break;
    }
  }
  if (pos == end)
  {
    traps.insert(std::make_pair(key ? key : TrapList::getMaxKey() + 100, trap));
  }

  return trap;
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
      pos->second.getTimeout() << " " << pos->second.getPattern() << " " <<
      pos->second.getReason() << std::endl;
  }
}


bool
Trap::parsePattern(std::string & pattern)
{
  while (!pattern.empty())
  {
    std::string::size_type equals = pattern.find('=');
    if (equals != std::string::npos)
    {
      std::string field(UpCase(pattern), 0, equals);
      pattern.erase(0, equals + 1);
      if ((0 == field.compare("N")) || (0 == field.compare("NICK")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->n_ = smartPattern(temp, true);
      }
      else if ((0 == field.compare("U")) || (0 == field.compare("USER")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->u_ = smartPattern(temp, false);
      }
      else if ((0 == field.compare("H")) || (0 == field.compare("HOST")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->h_ = smartPattern(temp, false);
      }
      else if ((0 == field.compare("UH")) || (0 == field.compare("USERHOST")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->uh_ = smartPattern(temp, false);
      }
      else if ((0 == field.compare("NUH")) ||
          (0 == field.compare("NICKUSERHOST")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->nuh_ = smartPattern(temp, false);
      }
      else if ((0 == field.compare("G")) || (0 == field.compare("GECOS")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->g_ = smartPattern(temp, false);
      }
      else if ((0 == field.compare("NUHG")) ||
          (0 == field.compare("NICKUSERHOSTGECOS")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->nuhg_ = smartPattern(temp, false);
      }
      else if ((0 == field.compare("C")) || (0 == field.compare("CLASS")))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->c_ = smartPattern(temp, false);
      }
      else if (0 == field.compare("VERSION"))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->version_ = smartPattern(temp, false);
      }
      else if (0 == field.compare("PRIVMSG"))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->privmsg_ = smartPattern(temp, false);
      }
      else if (0 == field.compare("NOTICE"))
      {
        std::string temp = grabPattern(pattern, " ,");
        this->notice_ = smartPattern(temp, false);
      }
      else
      {
        // WTF?
        return false;
      }
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

