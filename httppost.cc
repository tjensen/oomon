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

// Boost C++ Headers
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

// OOMon Headers
#include "oomon.h"
#include "httppost.h"
#include "log.h"
#include "main.h"
#include "irc.h"
#include "engine.h"
#include "config.h"
#include "util.h"


#if defined(DEBUG) || defined(PROXY_DEBUG)
# define HTTPPOST_DEBUG
#endif


HttpPost::HttpPost(const UserEntryPtr user) : Proxy(user)
{
  registerOnConnectHandler(boost::bind(&HttpPost::onConnect, this));
  registerOnReadHandler(boost::bind(&HttpPost::onRead, this, _1));
  this->setBuffering(true);
}


bool
HttpPost::onConnect(void)
{
#ifdef HTTPPOST_DEBUG
  std::cout << "HTTP POST proxy detector connected to " <<
    this->textAddress() << ":" << this->port() << std::endl;
#endif

  this->setProxyTimeout();

  BotSock::Port port = config.proxyTargetPort();
  BotSock::Address dst = config.proxyTargetAddress();
  if (INADDR_NONE == dst)
  {
    port = config.serverPort();
    dst = server.getRemoteAddress();
  }

  std::string buffer("POST http://");
  buffer += BotSock::inet_ntoa(dst);
  buffer += ':';
  buffer += boost::lexical_cast<std::string>(port);
  buffer += " HTTP/1.0\r\n";

  StrVector sendLines(config.proxySendLines());
  if (sendLines.empty())
  {
    buffer += "\r\n";
  }
  else
  {
    std::string extra;
    for (StrVector::iterator pos = sendLines.begin(); pos != sendLines.end();
        ++pos)
    {
      extra += *pos;
      extra += "\r\n";
    }

    buffer += "Content-Length: ";
    buffer += boost::lexical_cast<std::string>(extra.length());
    buffer += "\r\n\r\n";
    buffer += extra;
  }

#ifdef HTTPPOST_DEBUG
  std::cout << "HTTP_POST << " << buffer << std::endl;
#endif

  if (this->write(buffer) > 0)
  {
    return true;
  }

  return false;
}


bool
HttpPost::onRead(std::string text)
{
#ifdef HTTPPOST_DEBUG
  std::cout << "HTTP_POST >> " << text << std::endl;
#endif

  if (config.isOpenProxyLine(text))
  {
#ifdef HTTPPOST_DEBUG
    std::cout << "OPEN HTTP POST PROXY DETECTED!" << std::endl;
#endif
    this->detectedProxy();
  }

  return false;
}

