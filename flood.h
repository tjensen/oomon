#ifndef __FLOOD_H__
#define __FLOOD_H__
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
#include <list>
#include <ctime>

// OOMon Headers
#include "engine.h"
#include "watch.h"


class FloodList
{
public:
  FloodList(const std::string & type, const AutoAction & action,
      const std::string & reason, const int & maxCount, const int & maxTime,
      const Watch watch) : type_(type), action_(action), reason_(reason),
                           maxCount_(maxCount), maxTime_(maxTime),
                           watch_(watch) { }

  void clear() { this->list.clear(); };

  bool onNotice(const std::string & notice, std::string text,
    std::time_t now = std::time(NULL));

  int size() const { return this->list.size(); };

private:
  class FloodEntry
  {
  public:
    std::string	userhost;
    int		count;
    std::time_t	last;
  };

  std::list<FloodEntry> list;

  std::string type_;
  const AutoAction & action_;
  const std::string & reason_;
  const int & maxCount_;
  const int & maxTime_;
  const Watch watch_;

  void addFlood(const std::string & nick, const std::string & userhost,
    std::time_t now, bool local);
};

#endif /* __FLOOD_H__ */

