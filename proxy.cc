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

// Std C headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
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


#ifdef DEBUG
# define PROXY_DEBUG
#endif


Proxy::Proxy(const std::string & hostname, const std::string & nick,
  const std::string & userhost)
  : BotSock(), _hostname(hostname), _nick(nick), _userhost(userhost),
  _detectedProxy(false)
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
  _detectedProxy = true;

  std::string notice = std::string("Open ") + this->typeName() +
    " proxy detected for " + this->nick() + "!" + this->userhost() +
    " [" + this->address() + ":" + IntToStr(this->port()) + "]";
  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);

  doAction(this->nick(), this->userhost(),
    BotSock::inet_addr(this->address()),
    vars[VAR_SCAN_PROXY_ACTION]->getAction(),
    vars[VAR_SCAN_PROXY_ACTION]->getInt(),
    vars[VAR_SCAN_PROXY_REASON]->getString(), false);
}


bool
Proxy::connect(const std::string & address, const BotSock::Port port)
{
  this->_address = address;
  this->_port = port;

  return this->BotSock::connect(_address, _port);
}

