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

#ifndef OOMON_ACTION_H
#define OOMON_ACTION_H

// Std C++ Headers
#include <string>

// Boost C+ Headers
#include <boost/shared_ptr.hpp>

// OOMon Headers
#include "oomon.h"
#include "botclient.h"
#include "userentry.h"
#include "format.h"
#include "botexcept.h"


namespace Action
{
  class Nothing
  {
    public:
      Nothing(BotClient * client);

      virtual ~Nothing(void);

      virtual void operator()(const UserEntryPtr user);

    protected:
      BotClient * client_;
  };


  class List : public Nothing
  {
    public:
      List(BotClient * client, const FormatSet & formats);
      virtual ~List(void);

      virtual void operator()(const UserEntryPtr user);

    private:
      const FormatSet formats_;
  };

  class Kill : public Nothing
  {
    public:
      Kill(BotClient * client, const std::string & reason);
      virtual ~Kill(void);

      virtual void operator()(const UserEntryPtr user);

    private:
      const std::string reason_;
  };

  struct bad_action : OOMon::oomon_error
  {
    bad_action(const std::string & arg) : oomon_error(arg) { }
  };

  boost::shared_ptr<Nothing> parse(
          BotClient * client, std::string text, const FormatSet & formats);
};


typedef boost::shared_ptr<Action::Nothing> ActionPtr;


#endif /* OOMON_ACTION_H */

