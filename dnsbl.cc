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

// Boost C++ Headers
#include <boost/bind.hpp>

// Std C Headers
#include <netdb.h>
#include <errno.h>

// OOMon Headers
#include "oomon.h"
#include "adnswrap.h"
#include "botsock.h"
#include "dnsbl.h"
#include "vars.h"
#include "autoaction.h"
#include "log.h"
#include "main.h"
#include "botclient.h"


Dnsbl dnsbl;


bool
Dnsbl::checkZone(const BotSock::Address & addr, const std::string & nick,
  const std::string & userhost, const std::string & zone)
{
  if (!zone.empty())
  {
#ifdef HAVE_LIBADNS

    QueryPtr temp(new Query(addr, nick, userhost));

    int ret = adns.submit_rbl(addr, zone, temp->query);

    if (0 == ret)
    {
      this->_queries.push_back(temp);
    }
    else
    {
      std::cerr << "adns.submit_rbl() returned error: " << ret << std::endl;
    }

#else

    std::string::size_type pos = 0;
    std::string ip(BotSock::inet_ntoa(addr));

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

    if (NULL != result)
    {
      Dnsbl::openProxyDetected(addr, nick, userhost);

      return true;
    }

#endif /* HAVE_LIBADNS */
  }

  return false;
}

bool
Dnsbl::check(const BotSock::Address & addr, const std::string & nick,
  const std::string & userhost)
{
  StrVector zones;

  StrSplit(zones, vars[VAR_DNSBL_PROXY_ZONE]->getString(), " ,", true);

  return (zones.end() != std::find_if(zones.begin(), zones.end(),
    boost::bind(&Dnsbl::checkZone, this, addr, nick, userhost, _1)));
}


#ifdef HAVE_LIBADNS
bool
Dnsbl::Query::process(void)
{
  bool finished = false;

  adns_answer * answer;

  int ret = adns.check(this->query, answer);

  if (0 == ret)
  {
    // Request completed successfully
    if (0 == answer->status)
    {
#ifdef DNSBL_DEBUG
      std::cout << "DNSBL query succeeded!" << std::endl;
#endif /* DNSBL_DEBUG */
      Dnsbl::openProxyDetected(this->_addr, this->_nick, this->_userhost);
    }
    else
    {
#ifdef DNSBL_DEBUG
      std::cout << "DNSBL query failed!" << std::endl;
#endif /* DNSBL_DEBUG */
    }
    free(answer);
    finished = true;
  }
  else if (ESRCH == ret)
  {
    // No appropriate requests are outstanding!
    std::cerr << "adns.check() returned ESRCH!" << std::endl;
    finished = true;
  }

  return finished;
}


void
Dnsbl::process(void)
{
  this->_queries.remove_if(boost::bind(&Dnsbl::Query::process, _1));
}
#endif /* HAVE_LIBADNS */


void
Dnsbl::status(BotClient * client) const
{
#ifdef DNSBL_DEBUG
  client->send("DNSBL queries: " + IntToStr(this->_queries.size()));
#endif /* DNSBL_DEBUG */
}


void
Dnsbl::openProxyDetected(const BotSock::Address & addr,
  const std::string & nick, const std::string & userhost)
{
  // This is a proxy listed by the DNSBL
  std::string notice("DNSBL Open Proxy detected for ");
  notice += nick;
  notice += '!';
  notice += userhost;
  notice += " [";
  notice += BotSock::inet_ntoa(addr);
  notice += ']';
  Log::Write(notice);
  ::SendAll(notice, UserFlags::OPER);

  doAction(nick, userhost, addr, vars[VAR_DNSBL_PROXY_ACTION]->getAction(),
    vars[VAR_DNSBL_PROXY_ACTION]->getInt(),
    vars[VAR_DNSBL_PROXY_REASON]->getString(), false);
}

