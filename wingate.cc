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

// OOMon Headers
#include "oomon.h"
#include "wingate.h"
#include "log.h"
#include "main.h"
#include "engine.h"
#include "irc.h"


#ifdef DEBUG
# define WINGATE_DEBUG
#endif


WinGate::WinGate(const std::string & hostname, const std::string & nick,
  const std::string & userhost) : Proxy(hostname, nick, userhost)
{
  registerOnConnectHandler(boost::bind(&WinGate::onConnect, this));
  registerOnReadHandler(boost::bind(&WinGate::onRead, this, _1));
}


// OnConnect()
//
// Returns true if no errors occurred
//
bool
WinGate::onConnect()
{
#ifdef WINGATE_DEBUG
  std::cout << "WinGate proxy detector connected to " << this->getAddress() <<
    ":" << this->getPort() << std::endl;
#endif

  return true;
}


// onRead(text)
//
// Parses incoming data
//
// Returns true if no errors occurred
//
bool
WinGate::onRead(std::string text)
{
  if ((std::string::npos != text.find("WinGate>")) || (std::string::npos !=
    text.find("Enter : <host> [port] :")) || (std::string::npos !=
    text.find("Too many connected users - try again later")))
  {
#ifdef WINGATE_DEBUG
    std::cout << "OPEN WINGATE PROXY DETECTED!" << std::endl;
#endif
    this->detectedProxy();
    return false;
  }
  return true; 
}

