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
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <cerrno>

// Boost C++ Headers
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

// Std C Headers
#include <netdb.h>

// OOMon Headers
#include "oomon.h"
#include "adnswrap.h"
#include "botsock.h"
#include "dnsbl.h"
#include "vars.h"
#include "autoaction.h"
#include "log.h"
#include "main.h"
#include "watch.h"
#include "botclient.h"
#include "format.h"
#include "defaults.h"


AutoAction Dnsbl::action(DEFAULT_DNSBL_PROXY_ACTION,
    DEFAULT_DNSBL_PROXY_ACTION_TIME);
bool Dnsbl::enable(DEFAULT_DNSBL_PROXY_ENABLE);
std::string Dnsbl::reason(DEFAULT_DNSBL_PROXY_REASON);
StrVector Dnsbl::zones;


Dnsbl dnsbl;


void
Dnsbl::checkZone(const UserEntryPtr user, std::string zone,
    StrVector otherZones, CleanFunction cleanFunction)
{
  bool clean = true;

  do
  {
    if (!zone.empty())
    {

#ifdef HAVE_LIBADNS

      QueryPtr temp(new Query(user, zone, otherZones, cleanFunction,
            boost::bind(&Dnsbl::checkZone, this, _1, _2, _3, _4)));

      int ret = adns.submit_rbl(user->getIP(), zone, temp->query);

      if (0 == ret)
      {
        this->queries_.push_back(temp);
        otherZones.clear();
        clean = false;
      }
      else
      {
        std::cerr << "adns.submit_rbl() returned error: " << ret << std::endl;
        clean = true;
      }

#else

      std::string::size_type pos = 0;
      std::string ip(user->getTextIP());

      std::string lookup(zone);

      // Reverse the order of the octets and use them to prefix the blackhole
      // zone.
      int dots = CountChars(ip, '.');
      while (dots >= 0)
      {
        std::string::size_type nextDot = ip.find('.', pos);
        lookup = ip.substr(pos, nextDot - pos) + "." + lookup;
        pos = nextDot + 1;
        dots--;
      }

      struct hostent *result = gethostbyname(lookup.c_str());

      if (NULL == result)
      {
# ifdef DNSBL_DEBUG
        std::cout << "DNSBL query (zone: " << zone << ") failed!" <<
          std::endl;
# endif /* DNSBL_DEBUG */
        clean = true;
      }
      else
      {
# ifdef DNSBL_DEBUG
        std::cout << "DNSBL query (zone: " << zone << ") succeeded!" <<
          std::endl;
# endif /* DNSBL_DEBUG */

        Dnsbl::openProxyDetected(user, zone);
        otherZones.clear();
        clean = false;
      }

#endif /* HAVE_LIBADNS */

    }

    if (otherZones.empty())
    {
      zone.erase();
    }
    else
    {
      zone = otherZones.front();
      otherZones.erase(otherZones.begin());
    }

  } while (!zone.empty() || !otherZones.empty());

  if (clean)
  {
    cleanFunction(user);
  }
}


void
Dnsbl::check(const UserEntryPtr user, CleanFunction cleanFunction)
{
  if (Dnsbl::enable)
  {
    StrVector copy(Dnsbl::zones);

    if (copy.empty())
    {
      cleanFunction(user);
    }
    else
    {
      std::string zone(copy.front());
      copy.erase(copy.begin());

      this->checkZone(user, zone, copy, cleanFunction);
    }
  }
  else
  {
    cleanFunction(user);
  }
}


#ifdef HAVE_LIBADNS
bool
Dnsbl::Query::process(void)
{
  bool finished = false;

  try
  {
    Adns::Answer answer = adns.check(this->query);

    // Request completed successfully
    if (0 == answer.status())
    {
#ifdef DNSBL_DEBUG
      std::cout << "DNSBL query (zone: " << this->zone_ << ") succeeded!" <<
        std::endl;
#endif /* DNSBL_DEBUG */
      Dnsbl::openProxyDetected(this->user_, this->zone_);
    }
    else
    {
#ifdef DNSBL_DEBUG
      std::cout << "DNSBL query (zone: " << this->zone_ << ") failed!" <<
        std::endl;
#endif /* DNSBL_DEBUG */
      if (this->other_.empty())
      {
        // No more zones to check -- host is not listed in DNSBL
        this->clean_(this->user_);
      }
      else
      {
        // Check other zones
        std::string zone(this->other_.front());
        this->other_.erase(this->other_.begin());

        this->checker_(this->user_, zone, this->other_, this->clean_);
      }
    }
    finished = true;
  }
  catch (Adns::adns_error & e)
  {
    if (ESRCH == e.error())
    {
      // No appropriate requests are outstanding!
      std::cerr << "adns.check() returned ESRCH!" << std::endl;
      finished = true;
    }
  }

  return finished;
}


void
Dnsbl::process(void)
{
  this->queries_.remove_if(boost::bind(&Dnsbl::Query::process, _1));
}
#endif /* HAVE_LIBADNS */


void
Dnsbl::status(BotClient * client) const
{
#ifdef HAVE_LIBADNS
  if (Dnsbl::enable || !this->queries_.empty())
  {
    client->send("DNSBL queries: " +
        boost::lexical_cast<std::string>(this->queries_.size()));
  }
#endif /* HAVE_LIBADNS */
}


void
Dnsbl::openProxyDetected(const UserEntryPtr user, const std::string & zone)
{
  Format reason;
  reason.setStringToken('n', user->getNick());
  reason.setStringToken('u', user->getUserHost());
  reason.setStringToken('i', user->getTextIP());
  reason.setStringToken('z', zone);

  // This is a proxy listed by the DNSBL
  std::string notice("DNSBL Open Proxy detected for ");
  notice += user->getNick();
  notice += '!';
  notice += user->getUserHost();
  notice += " [";
  notice += user->getTextIP();
  notice += ']';
  Log::Write(notice);
  ::SendAll(notice, UserFlags::OPER, WATCH_DNSBL);

  doAction(user, Dnsbl::action, reason.format(Dnsbl::reason), false);
}


std::string
Dnsbl::getZones(void)
{
  std::string result;

  for (StrVector::const_iterator pos = Dnsbl::zones.begin();
      pos != Dnsbl::zones.end(); ++pos)
  {
    if (!result.empty())
    {
      result += ", ";
    }

    result += *pos;
  }

  return result;
}


std::string
Dnsbl::setZones(const std::string & value)
{
  std::string result;

  Dnsbl::zones.clear();

  StrSplit(Dnsbl::zones, value, " ,", true);

  return result;
}


void
Dnsbl::init(void)
{
  Dnsbl::setZones(DEFAULT_DNSBL_PROXY_ZONE);

  vars.insert("DNSBL_PROXY_ACTION", AutoAction::Setting(Dnsbl::action));
  vars.insert("DNSBL_PROXY_ENABLE", Setting::BooleanSetting(Dnsbl::enable));
  vars.insert("DNSBL_PROXY_REASON", Setting::StringSetting(Dnsbl::reason));
  vars.insert("DNSBL_PROXY_ZONE", Setting(Dnsbl::getZones, Dnsbl::setZones));
}

