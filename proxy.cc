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
#include "config.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "irc.h"
#include "vars.h"
#include "format.h"


#ifdef DEBUG
# define PROXY_DEBUG
#endif


Proxy::Proxy(const UserEntryPtr user)
  : BotSock(), detectedProxy_(false), user_(user)
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

  doAction(this->nick(), this->userhost(), this->address(),
      vars[VAR_SCAN_PROXY_ACTION]->getAction(),
      vars[VAR_SCAN_PROXY_ACTION]->getInt(),
      reason.format(vars[VAR_SCAN_PROXY_REASON]->getString()), false);
}


bool
Proxy::connect(const BotSock::Address & address, const BotSock::Port port)
{
  this->port_ = port;

  return this->BotSock::connect(this->address(), this->port());
}

