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
#include <boost/function.hpp>

// OOMon Headers
#include "strtype"
#include "adnswrap.h"
#include "userentry.h"
#include "autoaction.h"


class Dnsbl
{
public:
  static void init(void);

  Dnsbl(void) { }
  virtual ~Dnsbl(void) { }

  typedef boost::function<void(const UserEntryPtr)> CleanFunction;

  void check(const UserEntryPtr user, CleanFunction cleanFunction);

  static void openProxyDetected(const UserEntryPtr user,
      const std::string & zone);

#ifdef HAVE_LIBADNS
  void process(void);
#endif /* HAVE_LIBADNS */

  void status(class BotClient * client) const;

  static std::string getZones(void);
  static std::string setZones(const std::string & value);

private:
  typedef boost::function<void(const UserEntryPtr, std::string,
      StrVector, CleanFunction)> CheckerFunction;

  void checkZone(const UserEntryPtr user, std::string zone,
      StrVector otherZones, CleanFunction cleanFunction);

#ifdef HAVE_LIBADNS
  class Query
  {
  public:
    Query(const UserEntryPtr user, const std::string & zone,
        StrVector otherZones, CleanFunction cleanFunction,
        CheckerFunction checker) : user_(user), zone_(zone), other_(otherZones),
                                   clean_(cleanFunction), checker_(checker) { }

    bool process(void);

    Adns::Query query;

  private:
    const UserEntryPtr user_;
    const std::string zone_;
    StrVector other_;
    CleanFunction clean_;
    CheckerFunction checker_;
  };

  typedef boost::shared_ptr<Query> QueryPtr;
  typedef std::list<QueryPtr> QueryList;

  QueryList queries_;
#endif /* HAVE_LIBADNS */

  static AutoAction action;
  static bool enable;
  static std::string reason;
  static StrVector zones;
};


extern Dnsbl dnsbl;


#endif /* __DNSBL_H__ */

