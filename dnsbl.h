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

#ifndef __DNSBL_H__
#define __DNSBL_H__

// Std C++ Headers
#include <string>
#include <list>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>

// OOMon Headers
#include "adnswrap.h"


class Dnsbl
{
public:
  Dnsbl(void) { }
  virtual ~Dnsbl(void) { }

  bool check(const BotSock::Address & addr, const std::string & nick,
    const std::string & userhost);

  static void openProxyDetected(const BotSock::Address & addr,
    const std::string & nick, const std::string & userhost,
    const std::string & zone);

#ifdef HAVE_LIBADNS
  void process(void);
#endif /* HAVE_LIBADNS */

  void status(class BotClient * client) const;

private:
  bool checkZone(const BotSock::Address & addr, const std::string & nick,
    const std::string & userhost, const std::string & zone);

#ifdef HAVE_LIBADNS
  class Query
  {
  public:
    Query(const BotSock::Address & addr, const std::string & nick,
      const std::string & userhost, const std::string & zone) : addr_(addr),
      nick_(nick), userhost_(userhost), zone_(zone) { }

    bool process(void);

    Adns::Query query;

  private:
    const BotSock::Address addr_;
    const std::string nick_;
    const std::string userhost_;
    const std::string zone_;
  };

  typedef boost::shared_ptr<Query> QueryPtr;
  typedef std::list<QueryPtr> QueryList;

  QueryList queries_;
#endif /* HAVE_LIBADNS */
};


extern Dnsbl dnsbl;


#endif /* __DNSBL_H__ */

