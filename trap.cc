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


std::list<Trap> TrapList::traps;


Trap::Trap(const TrapAction action, const long timeout,
  const std::string & line)
  : _action(action), _timeout(timeout), _matchCount(0)
{
  if ((line.length() > 0) && (line[0] == '/'))
  {
    // Broken format! :(
    this->_reason = line;
    std::string pattern = grabPattern(this->_reason);
    this->_reason = trimLeft(this->_reason);

    this->_rePattern.reset(new RegExPattern(pattern));
  }
  else
  {
    std::string copy = line;

    if (Trap::parsePattern(copy, this->_nick, this->_userhost,
      this->_gecos, this->_version, this->_privmsg, this->_notice))
    {
      this->_reason = trimLeft(copy);
    }
    else
    {
      copy = line;

      this->_nick.reset();
      this->_userhost.reset();
      this->_gecos.reset();
      this->_version.reset();
      this->_privmsg.reset();
      this->_notice.reset();

      std::string pattern = FirstWord(copy);
      std::string nick, userhost;
      Trap::split(pattern, nick, userhost);

      if (nick != "*")
      {
        this->_nick.reset(new NickClusterPattern(nick));
      }
      if (userhost != "*@*")
      {
        this->_userhost.reset(new ClusterPattern(userhost));
      }
      this->_reason = copy;
    }
  }
}


Trap::Trap(const Trap & copy)
  : _action(copy._action), _timeout(copy._timeout), _nick(copy._nick),
  _userhost(copy._userhost), _gecos(copy._gecos), _version(copy._version),
  _privmsg(copy._privmsg), _notice(copy._notice), _rePattern(copy._rePattern),
  _reason(copy._reason), _lastMatch(copy._lastMatch),
  _matchCount(copy._matchCount)
{
}

Trap::~Trap(void)
{
}


void
Trap::updateStats(void)
{
  ++this->_matchCount;
  this->_lastMatch = std::time(NULL);
}


bool
Trap::matches(const UserEntryPtr user, const std::string & version,
  const std::string & privmsg, const std::string & notice) const
{
  if (NULL != this->_rePattern)
  {
    return (this->_rePattern->match(user->getNickUserHost()) ||
      this->_rePattern->match(user->getNickUserIP()));
  }
  else
  {
    if ((this->_nick && !this->_nick->match(user->getNick())) ||
      (this->_gecos && !this->_gecos->match(user->getGecos())) ||
      (this->_version && !this->_version->match(version)) ||
      (this->_privmsg && !this->_privmsg->match(privmsg)) ||
      (this->_notice && !this->_notice->match(notice)) ||
      (this->_userhost && !this->_userhost->match(user->getUserHost()) &&
      !this->_userhost->match(user->getUserIP())))
    {
      return false;
    }

    return true;
  }
}


bool
Trap::operator==(const Trap & other) const
{
  if (this->_rePattern)
  {
    if (other._rePattern)
    {
      return (other._rePattern->get() == this->_rePattern->get());
    }
    else
    {
      return false;
    }
  }
  else
  {
    if (this->_nick)
    {
      if (!other._nick || (this->_nick->get() != other._nick->get()))
      {
	return false;
      }
    }
    else
    {
      if (other._nick)
      {
	return false;
      }
    }
    if (this->_userhost)
    {
      if (!other._userhost ||
	(this->_userhost->get() != other._userhost->get()))
      {
	return false;
      }
    }
    else
    {
      if (other._userhost)
      {
	return false;
      }
    }
    if (this->_gecos)
    {
      if (!other._gecos || (this->_gecos->get() != other._gecos->get()))
      {
	return false;
      }
    }
    else
    {
      if (other._gecos)
      {
	return false;
      }
    }
    if (this->_version)
    {
      if (!other._version || (this->_version->get() != other._version->get()))
      {
	return false;
      }
    }
    else
    {
      if (other._version)
      {
	return false;
      }
    }
    if (this->_privmsg)
    {
      if (!other._privmsg || (this->_privmsg->get() != other._privmsg->get()))
      {
	return false;
      }
    }
    else
    {
      if (other._privmsg)
      {
	return false;
      }
    }
    if (this->_notice)
    {
      if (!other._notice || (this->_notice->get() != other._notice->get()))
      {
	return false;
      }
    }
    else
    {
      if (other._notice)
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

  if (NULL != this->_rePattern)
  {
    result = this->_rePattern->get();
  }
  else
  {
    if (this->_nick)
    {
      result = "nick=" + this->_nick->get();
    }
    if (this->_userhost)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "userhost=" + this->_userhost->get();
    }
    if (this->_gecos)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "gecos=" + this->_gecos->get();
    }
    if (this->_version)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "version=";
      result += this->_version->get();
    }
    if (this->_privmsg)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "privmsg=";
      result += this->_privmsg->get();
    }
    if (this->_notice)
    {
      if (!result.empty())
      {
	result += ',';
      }
      result += "notice=";
      result += this->_notice->get();
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
TrapList::add(const TrapAction action, const long timeout,
  const std::string & line)
{
  Trap trap(action, timeout, line);

  traps.remove(trap);

  traps.push_back(trap);

  return trap;
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

  std::string copy = line;
  std::string aword = FirstWord(copy);
  if (isNumeric(aword))
  {
    timeout = atol(aword.c_str());
    line = copy;
  }

  if (flag == "" || flag == "LIST")
  {
    TrapList::list(client, showCounts, showTimes);
  }
  else if (flag == "REMOVE")
  {
    if (client->flags().has(UserFlags::MASTER))
    {
      if (TrapList::remove(line))
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
        client->send(std::string("*** No trap exists for: ") + line);
      }
    }
    else
    {
      client->send("*** You don't have access to remove traps!");
    }
  }
  else
  {
    if (client->flags().has(UserFlags::MASTER))
    {
      try
      {
        TrapAction actionType = TrapList::actionType(flag);

        Trap node = TrapList::add(actionType, timeout, line);

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
TrapList::remove(const Trap & item)
{
  std::list<Trap>::size_type before = traps.size();

  traps.remove(item);

  return (traps.size() == before);
}


bool
TrapList::remove(const std::string & pattern)
{
  bool found = false;

  for (std::list<Trap>::iterator pos = traps.begin(); pos != traps.end(); )
  {
    if (*pos == pattern)
    {
      found = true;
      pos = traps.erase(pos);
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
    for (std::list<Trap>::iterator pos = traps.begin(); pos != traps.end();
      ++pos)
    {
      try
      {
        if (pos->matches(user, version, privmsg, notice))
        {
	  pos->updateStats();
	  pos->doAction(user);
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

  for (std::list<Trap>::iterator pos = traps.begin(); pos != traps.end(); ++pos)
  {
    client->send(pos->getString(showCounts, showTimes));
    count++;
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
  for (std::list<Trap>::iterator pos = TrapList::traps.begin();
    pos != TrapList::traps.end(); ++pos)
  {
    file << "TRAP " << TrapList::actionString(pos->getAction()) << " " <<
      pos->getTimeout() << " " << pos->getPattern() << " " <<
      pos->getReason() << std::endl;
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

