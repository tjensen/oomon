#ifndef __SOCKS5_H__
#define __SOCKS5_H__
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

#include <string>

#include "oomon.h"
#include "proxy.h"

class Socks5 : public Proxy
{
public:
  Socks5(const std::string & hostname, const std::string & nick,
    const std::string & userhost)
    : Proxy(hostname, nick, userhost), state(STATE_WAIT1)
  {
    this->setType(SOCKS5);
    this->setBinary(true);
  }

  virtual bool onConnect();

protected:
  virtual bool onRead(const char *text, const int size);
  virtual ProxyType getType() const { return Proxy::SOCKS5; };
  virtual std::string getTypeName() const { return "SOCKS5"; };

  enum { STATE_WAIT1, STATE_WAIT2 } state;
};

#endif /* __SOCKS5_H__ */

