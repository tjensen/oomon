#ifndef __PROXY_H__
#define __PROXY_H__
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
#include <string>

// Std C headers
#include <sys/types.h>
#include <unistd.h>

// OOMon headers
#include "strtype"
#include "oomon.h"
#include "botsock.h"
#include "userentry.h"


class Proxy : public BotSock
{
public:
  enum Protocol { SOCKS4, SOCKS5, WINGATE, HTTP_CONNECT, HTTP_POST };

  Proxy(const UserEntryPtr user);
  virtual ~Proxy(void);

  bool connect(const BotSock::Port port);

  BotSock::Address address(void) const { return this->user_->getIP(); }
  std::string textAddress(void) const { return this->user_->getTextIP(); }
  BotSock::Port port(void) const { return this->port_; }
  virtual Proxy::Protocol type(void) const = 0;

  bool expired(void) const
  {
    return ((0 != this->timeout_) && (time(0) >= this->timeout_));
  }
  void setProxyTimeout(void);

protected:
  void detectedProxy(void);

  std::string hostname(void) const { return this->user_->getHost(); }
  std::string nick(void) const { return this->user_->getNick(); }
  std::string userhost(void) const { return this->user_->getUserHost(); }

  virtual std::string typeName(void) const = 0;

  bool detectedProxy_;

private:
  const UserEntryPtr user_;
  BotSock::Port port_;
  std::time_t timeout_;
};

#endif /* __PROXY_H__ */

