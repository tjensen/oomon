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

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// OOMon Headers
#include "userentry.h"
#include "botsock.h"
#include "irc.h"
#include "seedrand.h"
#include "vars.h"
#include "userflags.h"
#include "autoaction.h"
#include "main.h"
#include "log.h"
#include "trap.h"
#include "util.h"


#ifdef DEBUG_USERHASH
extern unsigned long userEntryCount;
#endif


UserEntry::UserEntry(const std::string & aNick,
  const std::string & aUser, const std::string & aHost,
  const std::string & aFakeHost, const std::string & aUserClass,
  const std::string & aGecos, const BotSock::Address anIp,
  const std::time_t aConnectTime, const bool oper)
  : user(aUser), host(aHost), fakeHost(aFakeHost),
  domain(::getDomain(aHost, false)), userClass(::server.downCase(aUserClass)),
  gecos(aGecos), ip(anIp), connectTime(aConnectTime), reportTime(0),
  versioned(0), isOper(oper)
{
  this->setNick(aNick);
#ifdef DEBUG_USERHASH
  ++userEntryCount;
#endif
}


UserEntry::~UserEntry(void)
{
#ifdef DEBUG_USERHASH
  --userEntryCount;
#endif
}


void
UserEntry::version(void)
{
  this->versioned = std::time(NULL);
  server.ctcp(this->nick, "VERSION");
}


void
UserEntry::hasVersion(const std::string & version)
{
  this->versioned = 0;
}


bool
UserEntry::matches(const std::string & lcNick) const
{
  return (0 == lcNick.compare(server.downCase(this->getNick())));
}


bool
UserEntry::matches(const std::string & lcNick, const std::string & lcUser,
  const std::string & lcHost) const
{
  if (vars[VAR_BROKEN_HOSTNAME_MUNGING]->getBool())
  {
    return (this->matches(lcNick) &&
      (0 == lcUser.compare(server.downCase(this->getUser()))));
  }
  else
  {
    return (this->matches(lcNick) &&
      (0 == lcUser.compare(server.downCase(this->getUser()))) &&
      ((0 == lcHost.compare(server.downCase(this->getHost()))) ||
       (!this->getFakeHost().empty() &&
	(0 == lcHost.compare(server.downCase(this->getFakeHost()))))));
  }
}


void
UserEntry::checkVersionTimeout(const std::time_t now, const std::time_t timeout)
{
  if (this->versioned > 0)
  {
    std::time_t elapsed = now - this->versioned;

    if (elapsed > timeout)
    {
      std::string userhost(this->getUserHost());

      std::string notice("*** No CTCP VERSION reply from ");
      notice += this->getNick();
      notice += " (";
      notice += userhost;
      notice += ") in ";
      notice += boost::lexical_cast<std::string>(elapsed);
      notice += " seconds.";

      ::SendAll(notice, UserFlags::OPER, WATCH_CTCPVERSIONS);
      Log::Write(notice);

      doAction(this->getNick(), userhost, this->getIP(),
	vars[VAR_CTCPVERSION_TIMEOUT_ACTION]->getAction(),
	vars[VAR_CTCPVERSION_TIMEOUT_ACTION]->getInt(),
	vars[VAR_CTCPVERSION_TIMEOUT_REASON]->getString(), false);

      this->versioned = 0;
    }
  }
}


void
UserEntry::setNick(const std::string & aNick)
{
  this->nick = aNick;
  this->randScore = ::seedrandScore(aNick);
}


std::string
UserEntry::output(const std::string & format) const
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
	  if (this->getIP() != INADDR_NONE)
	  {
            result += '[' + BotSock::inet_ntoa(this->getIP()) + ']';
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

