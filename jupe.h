#ifndef __JUPE_H__
#define __JUPE_H__
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

// Std C++ Headers
#include <string>
#include <list>

// Std C Headers
#include <time.h>

// OOMon Headers
#include "strtype"
#include "vars.h"


class JupeJoinList
{
public:
  JupeJoinList() { };

  void clear() { this->list.clear(); };

  void onNotice(const std::string & notice);

  void status(StrList & output) const;

private:
  class JupeJoinEntry
  {
  public:
    std::string	userhost;
    std::string	channel;
    int		count;
    time_t	last;
  };

  std::list<JupeJoinEntry> list;

  void checkJupe(const std::string & nick, const JupeJoinEntry & entry) const;

  void add(const std::string & nick, const std::string & userhost,
    const std::string & channel, time_t now);
};


extern JupeJoinList jupeJoiners;


#endif /* __JUPE_H__ */

