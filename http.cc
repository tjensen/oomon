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
#include "http.h"
#include "log.h"
#include "main.h"
#include "irc.h"
#include "engine.h"
#include "config.h"
#include "util.h"


#ifdef DEBUG
# define HTTP_DEBUG
#endif


Http::Http(const UserEntryPtr user) : Proxy(user)
{
  registerOnConnectHandler(boost::bind(&Http::onConnect, this));
  registerOnReadHandler(boost::bind(&Http::onRead, this, _1));
  this->setBuffering(true);
}


// OnConnect()
//
//
bool
Http::onConnect()
{
#ifdef HTTP_DEBUG
  std::cout << "HTTP CONNECT proxy detector connected to " <<
    this->textAddress() << ":" << this->getPort() << std::endl;
#endif

  std::string buffer("CONNECT ");
  buffer += config.serverAddress();
  buffer += ':';
  buffer += boost::lexical_cast<std::string>(config.serverPort());
  buffer += " HTTP/1.0\n\n";

#ifdef HTTP_DEBUG
  std::cout << "HTTP << " << buffer << std::endl;
#endif

  if (this->write(buffer + "\r\n\r\n") > 0)
  {
    return true;
  }

  return false;
}


bool
Http::onRead(std::string text)
{
#ifdef HTTP_DEBUG
  std::cout << "HTTP >> " << text << std::endl;
#endif

  if ((std::string::npos != text.find("HTTP/1.0 200 Connection established")) ||
    (std::string::npos != text.find("HTTP/1.1 200 Connection established")))
  {
#ifdef HTTP_DEBUG
    std::cout << "OPEN HTTP CONNECT PROXY DETECTED!" << std::endl;
#endif
    this->detectedProxy();
  }
  return false;
}

