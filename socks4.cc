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
#include <fstream>
#include <string>
#include <cerrno>

// Boost C++ Headers
#include <boost/bind.hpp>

// Std C Headers
#include <stdio.h>				// for sprintf(4)
#include <sys/types.h> 				// for getpwuid(3)
#include <pwd.h>				// for getpwuid(3)

// OOMon Headers
#include "oomon.h"
#include "socks4.h"
#include "log.h"
#include "main.h"
#include "engine.h"
#include "irc.h"
#include "util.h"
#include "config.h"


#if defined(DEBUG) || defined(PROXY_DEBUG)
# define SOCKS4_DEBUG
#endif


Socks4::Socks4(const UserEntryPtr user) : Proxy(user)
{
  registerOnConnectHandler(boost::bind(&Socks4::onConnect, this));
  registerOnBinaryReadHandler(boost::bind(&Socks4::onRead, this, _1, _2));
  this->setBinary(true);
}


// OnConnect()
//
//
bool
Socks4::onConnect()
{
#ifdef SOCKS4_DEBUG
  std::cout << "SOCKS4 proxy detector connected to " << this->textAddress() <<
    ":" << this->port() << std::endl;
#endif

  this->setProxyTimeout();

  struct passwd *tmppw = getpwuid(getuid());

  const char *tmp;

  if (tmppw && tmppw->pw_name)
    tmp = tmppw->pw_name;
  else
    tmp = "";

  BotSock::Port port = config.proxyTargetPort();
  BotSock::Address dst = ntohl(config.proxyTargetAddress());
  if (INADDR_NONE == dst)
  {
    port = config.serverPort();
    dst = ntohl(server.getRemoteAddress());
  }

  char buff[128];

#ifdef HAVE_SNPRINTF
  int m = ::snprintf(buff, sizeof(buff),
#else
  int m = ::sprintf(buff,
#endif
    "%c%c%c%c%c%c%c%c%s%c", 4, 1, (port >> 8) & 0xff,
    port & 0xff, (dst >> 24) & 0xff, (dst >> 16) & 0xff, (dst >> 8) & 0xff,
    dst & 0xff, tmp, 0);

  int n = this->write(buff, m);

  if (n < 0)
  {
    std::cerr << "Error: Write " << errno <<
#if defined(HAVE_STRERROR)
      " (" << ::strerror(errno) << ")" <<
#endif
      std::endl;
    return false;
  }
  else if (n == 0)
  {
    return false;
  }
#ifdef SOCKS4_DEBUG
  std::cout << "Socks4 << " << hexDump(buff, m) << std::endl;
#endif
  return true;
}


bool
Socks4::onRead(const char *text, const int size)
{
#ifdef SOCKS4_DEBUG
  std::cout << "Socks4 returned: " << hexDump(text, size) << std::endl;
#endif

  if ((size >= 2) && ((text[0] == 0) || (text[0] == 4)) && (text[1] == 90))
  {
#ifdef SOCKS4_DEBUG
    std::cout << "OPEN SOCKS4 PROXY DETECTED!" << std::endl;
#endif
    this->detectedProxy();
  }
  return false;
}

