#ifndef __WATCH_H__
#define __WATCH_H__
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
#include <set>

// OOMon Headers
#include "strtype"


enum Watch
{
  WATCH_CHAT, WATCH_CONNECTS, WATCH_CONNFLOOD, WATCH_DISCONNECTS, WATCH_DLINES,
  WATCH_DLINE_MATCHES, WATCH_FLOODERS, WATCH_GLINES, WATCH_GLINE_MATCHES,
  WATCH_INFOS, WATCH_JUPES, WATCH_KILLS, WATCH_KLINES, WATCH_KLINE_MATCHES,
  WATCH_LINKS, WATCH_MOTDS, WATCH_MSGS, WATCH_NICK_CHANGES, WATCH_SEEDRAND,
  WATCH_SPAMBOTS, WATCH_SPAMTRAP, WATCH_STATS, WATCH_TOOMANYS, WATCH_TRACES,
  WATCH_TRAPS, WATCH_WALLOPS, 

  WATCH_MIN = WATCH_CHAT, WATCH_MAX = WATCH_WALLOPS
};


class WatchSet
{
public:
  WatchSet(void) { };
  ~WatchSet(void) { };

  void clear(void) { this->contents.clear(); };

  std::set<Watch>::size_type size(void) const { return this->contents.size(); };

  void add(const Watch watch) { this->contents.insert(watch); };
  void add(const WatchSet & watches);

  void remove(const Watch watch) { this->contents.erase(watch); };
  void remove(const WatchSet & watches);

  bool has(const Watch watch) const
    { return (this->contents.end() != this->contents.find(watch)); };
  bool has(const WatchSet & watches) const;

  void set(StrList & output, std::string line);

  static WatchSet all(void);
  static WatchSet defaults(void);
  static std::string getWatchName(const Watch watch);
  static Watch getWatchValue(const std::string & watch);
  static std::string getWatchNames(const WatchSet & watches, bool distinct);
  static WatchSet getWatchValues(const std::string & watches);
  static WatchSet getWatchValues(StrVector & watches);

private:
  typedef std::set<Watch> _WatchSet;

  _WatchSet contents;
};


#endif /* __WATCH_H__ */
    
