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

// Std C Headers
#include <time.h>

// OOMon Headers
#include "strtype"
#include "botexcept.h"
#include "trap.h"
#include "util.h"
#include "irc.h"
#include "log.h"
#include "main.h"
#include "config.h"
#include "vars.h"
#include "pattern.h"
#include "arglist.h"


std::list<Trap> TrapList::traps;


Trap::Trap(const TrapAction action, const long timeout,
  const std::string & line)
  : _action(action), _timeout(timeout), _nick(0), _userhost(0), _gecos(0),
  _rePattern(0), _lastMatch(0), _matchCount(0)
{
  if ((line.length() > 0) && (line[0] == '/'))
  {
    // Broken format! :(
    this->_reason = line;
    std::string pattern = grabPattern(this->_reason);
    this->_reason = trimLeft(this->_reason);

    this->_rePattern = new RegExPattern(pattern);
  }
  else
  {
    std::string copy = line;

    if (Trap::parsePattern(copy, this->_nick, this->_userhost,
      this->_gecos))
    {
      this->_reason = trimLeft(copy);
    }
    else
    {
      copy = line;

      if (this->_nick)
      {
	delete this->_nick;
	this->_nick = 0;
      }
      if (this->_userhost)
      {
	delete this->_userhost;
	this->_userhost = 0;
      }
      if (this->_gecos)
      {
	delete this->_gecos;
	this->_gecos = 0;
      }

      std::string pattern = FirstWord(copy);
      std::string nick, userhost;
      Trap::split(pattern, nick, userhost);

      if (nick != "*")
      {
        this->_nick = new NickClusterPattern(nick);
      }
      if (userhost != "*@*")
      {
        this->_userhost = new ClusterPattern(userhost);
      }
      this->_reason = copy;
    }
  }
}


Trap::Trap(const Trap & copy)
  : _action(copy._action), _timeout(copy._timeout), _nick(0), _userhost(0),
  _gecos(0), _rePattern(0), _reason(copy._reason), _lastMatch(copy._lastMatch),
  _matchCount(copy._matchCount)
{
  if (NULL != copy._rePattern)
  {
    this->_rePattern = new RegExPattern(copy._rePattern->get());
  }
  if (NULL != copy._nick)
  {
    this->_nick = smartPattern(copy._nick->get(), true);
  }
  if (NULL != copy._userhost)
  {
    this->_userhost = smartPattern(copy._userhost->get(), false);
  }
  if (NULL != copy._gecos)
  {
    this->_gecos = smartPattern(copy._gecos->get(), false);
  }
}

Trap::~Trap(void)
{
  if (NULL != this->_rePattern)
  {
    delete this->_rePattern;
  }
  if (NULL != this->_nick)
  {
    delete this->_nick;
  }
  if (NULL != this->_userhost)
  {
    delete this->_userhost;
  }
  if (NULL != this->_gecos)
  {
    delete this->_gecos;
  }
}


void
Trap::updateStats(void)
{
  ++this->_matchCount;
  this->_lastMatch = time(NULL);
}


bool
Trap::matches(const std::string & nick, const std::string & userhost,
  const std::string & ip, const std::string & gecos) const
{
  std::string::size_type at = userhost.find('@');

  std::string userip;
  if (std::string::npos != at)
  {
    userip = userhost.substr(0, at + 1) + ip;
  }

  if (NULL != this->_rePattern)
  {
    return (this->_rePattern->match(nick + '!' + userhost) ||
      this->_rePattern->match(nick + '!' + userip));
  }
  else
  {
    if ((this->_nick && !this->_nick->match(nick)) ||
      (this->_gecos && !this->_gecos->match(gecos)) ||
      (this->_userhost && !this->_userhost->match(userhost) &&
      !this->_userhost->match(userip)))
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
    result += ULongToStr(this->getMatchCount()) + " ";
  }

  if (showTime && (0 != this->getLastMatch()))
  {
    result += timeStamp(TIMESTAMP_LOG, this->getLastMatch()) + " ";
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
        result += ULongToStr(this->getTimeout()) + " " + this->getPattern() +
	  " (" + this->getReason() + ")";
      }
      else
      {
        result += this->getPattern() + " (" + this->getReason() + ")";
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
	result += ",";
      }
      result += "userhost=" + this->_userhost->get();
    }
    if (this->_gecos)
    {
      if (!result.empty())
      {
	result += ",";
      }
      result += "gecos=" + this->_gecos->get();
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
Trap::doAction(const std::string & nick, const std::string & userhost,
  const std::string & ip, const std::string & gecos) const
{
  std::string notice("*** Trapped: " + nick + "!" + userhost + " [" + ip + "]");

  BotSock::Address IP = BotSock::inet_addr(ip);

  Log::Write(notice);
  switch (this->getAction())
  {
    case TRAP_ECHO:
      ::SendAll(notice, UserFlags::OPER, WATCH_TRAPS);
      break;
    case TRAP_KILL:
      server.kill("Auto-Kill", nick, this->getReason());
      break;
    case TRAP_KLINE:
      server.kline("Auto-Kline", this->getTimeout(), klineMask(userhost),
        this->getReason());
      break;
    case TRAP_KLINE_HOST:
      {
        std::string::size_type at = userhost.find('@');
        if (at != std::string::npos)
        {
          std::string host = userhost.substr(at + 1, std::string::npos);
          server.kline("Auto-Kline", this->getTimeout(), "*@" + host,
            this->getReason());
        }
        else
        {
          std::cerr << "Userhost is missing @ symbol!" << std::endl;
        }
      }
      break;
    case TRAP_KLINE_DOMAIN:
      {
        std::string::size_type at = userhost.find('@');
        if (at != std::string::npos)
        {
          std::string domain = getDomain(userhost.substr(at + 1,
  	    std::string::npos), true);
          server.kline("Auto-Kline", this->getTimeout(), "*@" + domain,
            this->getReason());
        }
        else
        {
          std::cerr << "Userhost is missing @ symbol!" << std::endl;
        }
      }
      break;
    case TRAP_KLINE_IP:
      if (INADDR_NONE != IP)
      {
        server.kline("Auto-Kline", this->getTimeout(), "*@" + ip,
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
        std::string::size_type at = userhost.find('@');
        if (at != std::string::npos)
        {
          std::string user = userhost.substr(0, at);
          if ((user.length() > 1) && (user[0] == '~'))
          {
  	    user = user.substr(1);
	  }
	  if (INADDR_NONE != IP)
	  {
	    server.kline("Auto-Kline", this->getTimeout(), "*" + user + "@" +
	      classCMask(ip), this->getReason());
	  }
	  else
	  {
	    std::cerr << "Missing or invalid IP address -- unable to KLINE!" <<
              std::endl;
	  }
        }
        else
        {
          std::cerr << "Userhost is missing @ symbol!" << std::endl;
        }
      }
      break;
    case TRAP_KLINE_NET:
      if (INADDR_NONE != IP)
      {
        server.kline("Auto-Kline", this->getTimeout(), "*@" + classCMask(ip),
	  this->getReason());
      }
      else
      {
        std::cerr << "Missing or invalid IP address -- unable to KLINE!" <<
          std::endl;
      }
      break;
    case TRAP_DLINE_IP:
      if (INADDR_NONE != IP)
      {
        server.dline("Auto-Dline", this->getTimeout(), ip, this->getReason());
      }
      else
      {
        std::cerr << "Missing or invalid IP address -- unable to DLINE!" <<
	  std::endl;
      }
      break;
    case TRAP_DLINE_NET:
      if (INADDR_NONE != IP)
      {
        server.dline("Auto-Dline", this->getTimeout(), classCMask(ip),
	  this->getReason());
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
TrapList::cmd(StrList & output, std::string line, const bool master,
  const std::string & handle)
{
  ArgList args("-a -t", "");

  if (-1 == args.parseCommand(line))
  {
    output.push_back("*** Invalid parameter: " + args.getInvalid());
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
    TrapList::list(output, showCounts, showTimes);
  }
  else if (flag == "REMOVE")
  {
    if (master)
    {
      if (TrapList::remove(line))
      {
        modified = true;
        if (handle.size() > 0)
        {
          ::SendAll(handle + " removed trap: " + line, UserFlags::OPER,
	    WATCH_TRAPS);
          Log::Write(handle + " removed trap: " + line);
        }
      }
      else
      {
        output.push_back(std::string("*** No trap exists for: ") + line);
      }
    }
    else
    {
      output.push_back("*** You don't have access to remove traps!");
    }
  }
  else
  {
    if (master)
    {
      try
      {
        TrapAction actionType = TrapList::actionType(flag);

        Trap node = TrapList::add(actionType, timeout, line);

        modified = true;

        if (handle.length() > 0)
        {
          std::string notice(handle + " added trap " + node.getString());
          ::SendAll(notice, UserFlags::OPER, WATCH_TRAPS);
          Log::Write(notice);
        }
      }
      catch (OOMon::invalid_action & e)
      {
        output.push_back("*** Invalid TRAP action: " + e.what());
      }
      catch (OOMon::regex_error & e)
      {
        output.push_back("*** RegEx error: " + e.what());
      }
    }
    else
    {
      output.push_back("*** You don't have access to add traps!");
    }
  }

  if (modified && (handle.length() > 0) && vars[VAR_AUTO_SAVE]->getBool())
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
TrapList::match(const std::string & nick, const std::string & userhost,
  const std::string & ip, const std::string & gecos)
{
  if (!Config::IsOKHost(userhost, ip) && !Config::IsOper(userhost, ip))
  {
    for (std::list<Trap>::iterator pos = traps.begin(); pos != traps.end();
      ++pos)
    {
      try
      {
        if (pos->matches(nick, userhost, ip, gecos))
        {
	  pos->updateStats();
	  pos->doAction(nick, userhost, ip, gecos);
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
TrapList::list(StrList & output, bool showCounts, bool showTimes)
{
  int count = 0;

  for (std::list<Trap>::iterator pos = traps.begin(); pos != traps.end(); ++pos)
  {
    output.push_back(pos->getString(showCounts, showTimes));
    count++;
  }
  if (count > 0)
  {
    output.push_back("*** End of TRAP list.");
  }
  else
  {
    output.push_back("*** TRAP list is empty!");
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
Trap::parsePattern(std::string & pattern, Pattern* & nick,
  Pattern* & userhost, Pattern* & gecos)
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

