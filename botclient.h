#ifndef __BOTCLIENT_H__
#define __BOTCLIENT_H__
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
#include <string>
#include <algorithm>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>

// OOMon Headers
#include "oomon.h"
#include "strtype"
#include "userflags.h"


class BotClient
{
public:
  typedef boost::shared_ptr<BotClient> ptr;

  BotClient(void) { }
  virtual ~BotClient(void) { }

  virtual void send(const std::string & text) = 0;
  virtual UserFlags flags(void) const = 0;
  virtual std::string handle(void) const = 0;
  virtual std::string bot(void) const = 0;
  virtual std::string id(void) const = 0;

private:
  class sender
  {
  public:
    sender(BotClient *target) : _target(target) { }
    void operator()(const std::string & text)
    {
      _target->send(text);
    }
  private:
    BotClient *_target;
  };

public:
  void send(const StrList & text)
  {
    std::for_each(text.begin(), text.end(), BotClient::sender(this));
  }
  std::string handleAndBot(void) const
  {
    return (this->handle() + '@' + this->bot());
  }
};

#endif /* __BOTCLIENT_H__ */

