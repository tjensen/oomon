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

// Std C++ headers
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <ctime>
#include <cstdlib>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// Std C headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>

// OOMon headers
#include "oomon.h"
#include "proxy.h"
#include "socks4.h"
#include "socks5.h"
#include "wingate.h"
#include "botexcept.h"
#include "http.h"
#include "httppost.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "irc.h"
#include "vars.h"
#include "format.h"
#include "defaults.h"


#ifdef DEBUG
# define PROXY_DEBUG
#endif


AutoAction Proxy::action(DEFAULT_SCAN_PROXY_ACTION,
    DEFAULT_SCAN_PROXY_ACTION_TIME);
std::string Proxy::reason(DEFAULT_SCAN_PROXY_REASON);
int Proxy::timeout(DEFAULT_SCAN_TIMEOUT);
std::string Proxy::exec(DEFAULT_SCAN_PROXY_EXEC);


Proxy::Proxy(const UserEntryPtr user)
  : BotSock(), detectedProxy_(false), user_(user), timeout_(0)
{
}


Proxy::~Proxy(void)
{
#ifdef PROXY_DEBUG
  std::cout << "Proxy checker ending..." << std::endl;
#endif
}


void
Proxy::detectedProxy(void)
{
  // We DEFINITELY do not want to cache this one!
  detectedProxy_ = true;

  std::string portString = boost::lexical_cast<std::string>(this->port());

  Format reason;
  reason.setStringToken('n', this->nick());
  reason.setStringToken('u', this->userhost());
  reason.setStringToken('i', this->textAddress());
  reason.setStringToken('p', portString);
  reason.setStringToken('t', this->typeName());

  std::string notice("Open ");
  notice += this->typeName();
  notice += " proxy detected for ";
  notice += this->nick();
  notice += "!";
  notice += this->userhost();
  notice += " [";
  notice += this->textAddress();
  notice += ":";
  notice += portString;
  notice += "]";
  ::SendAll(notice, UserFlags::OPER, WATCH_PROXYSCANS);
  Log::Write(notice);

  doAction(this->user_, Proxy::action, reason.format(Proxy::reason), false);

  if (!Proxy::exec.empty())
  {
    Format fmt;
    fmt.setStringToken('n', this->nick());
    fmt.setStringToken('u', this->userhost());
    fmt.setStringToken('i', this->textAddress());
    fmt.setStringToken('p', portString);
    fmt.setStringToken('t', this->typeName());

    std::string cmdline(fmt.format(Proxy::exec));
    int result = std::system(cmdline.c_str());
    std::cout << "External command returned " << result << std::endl;
  }
}


bool
Proxy::connect(const BotSock::Port port)
{
  this->port_ = port;

  return this->BotSock::connect(this->address(), this->port());
}


void
Proxy::setProxyTimeout(void)
{
  if (Proxy::timeout > 0)
  {
    this->timeout_ = time(0) + Proxy::timeout;
  }
}


void
Proxy::init(void)
{
  vars.insert("SCAN_PROXY_ACTION", AutoAction::Setting(Proxy::action));
  vars.insert("SCAN_PROXY_EXEC", Setting::StringSetting(Proxy::exec));
  vars.insert("SCAN_PROXY_REASON", Setting::StringSetting(Proxy::reason));
  vars.insert("SCAN_TIMEOUT", Setting::IntegerSetting(Proxy::timeout, 1));
}

