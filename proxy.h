#ifndef __PROXY_H__
#define __PROXY_H__
// ===========================================================================
// OOMon - Objected Oriented Monitor Bot
// Copyright (C) 2003  Timothy L. Jensen
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
#include <string>
#include <list>
#include <functional>

// Std C headers
#include <sys/types.h>
#include <unistd.h>

// OOMon headers
#include "strtype"
#include "oomon.h"
#include "botsock.h"


class Proxy : public BotSock
{
public:
  enum Protocol { SOCKS4, SOCKS5, WINGATE, HTTP };

  Proxy(const std::string & hostname, const std::string & nick,
    const std::string & userhost);
  virtual ~Proxy(void);

  bool connect(const std::string & address, const BotSock::Port port);

  std::string address(void) const { return this->_address; };
  BotSock::Port port(void) const { return this->_port; };
  virtual Proxy::Protocol type(void) const = 0;

protected:
  void detectedProxy(void);

  std::string hostname(void) const { return this->_hostname; };
  std::string nick(void) const { return this->_nick; };
  std::string userhost(void) const { return this->_userhost; };

  virtual std::string typeName(void) const = 0;

  bool _detectedProxy;

private:
  std::string _address, _hostname, _nick, _userhost;
  BotSock::Port _port;
};

#endif /* __PROXY_H__ */

