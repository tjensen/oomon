#ifndef __WATCH_H__
#define __WATCH_H__
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

// ===========================================================================
// File Description:
//
//  This file contains the WatchSet class, which is used to determine what
//  types of messages to send to a user of OOMon.
//
//  Each OOMon user is able to configure his or her watches through the use
//  of the ".watch" command.  All messages sent by OOMon have watch types
//  associated with them.  If a user's watches do not include the watch
//  types of the message, the user will not receive the message.
// ===========================================================================

// Std C++ Headers
#include <string>
#include <bitset>

// OOMon Headers
#include "strtype"


enum Watch
{
  WATCH_CHAT, WATCH_CONNECTS, WATCH_CONNFLOOD, WATCH_DISCONNECTS, WATCH_DLINES,
  WATCH_DLINE_MATCHES, WATCH_FLOODERS, WATCH_GLINES, WATCH_GLINE_MATCHES,
  WATCH_INFOS, WATCH_JUPES, WATCH_KILLS, WATCH_KLINES, WATCH_KLINE_MATCHES,
  WATCH_LINKS, WATCH_MOTDS, WATCH_MSGS, WATCH_NICK_CHANGES, WATCH_OPERFAILS,
  WATCH_SEEDRAND, WATCH_SPAMBOTS, WATCH_SPAMTRAP, WATCH_STATS, WATCH_TOOMANYS,
  WATCH_TRACES, WATCH_TRAPS, WATCH_WALLOPS, NUM_WATCHES,

  WATCH_MIN = WATCH_CHAT, WATCH_MAX = WATCH_WALLOPS
};


class WatchSet
{
public:
  WatchSet(void) { };
  WatchSet(const Watch watch) { this->contents.set(watch); }
  ~WatchSet(void) { };

  // Remove all watch types from this set.
  void clear(void) { this->contents.reset(); };

  // Add a watch type or set of watch types to this set.
  void add(const Watch watch) { this->contents.set(watch); };
  void add(const WatchSet & watches) { this->contents |= watches.contents; };

  // Return true if this set contains the desired watch type or set of
  // watch types.
  bool has(const Watch watch) const
    { return this->contents.test(watch); };
  bool has(const WatchSet & watches) const
    { return (watches.contents == (this->contents & watches.contents)); };

  void set(class BotClient * output, std::string line, const bool noisy = true);

  // Return a set of all possible watch types
  static WatchSet all(void);

  // Return a set of the default watch types
  static WatchSet defaults(void);

  // Convert a watch type to its symbolic name
  static std::string getWatchName(const Watch watch);

  // Convert a symbolic name to a watch type
  static Watch getWatchValue(const std::string & watch);

  // Return a list of symbolic names corresponding to a set of watch
  // types.  If distinct is true, prepend watch types that are in the
  // set with '+' and ones that aren't in the set with '-'.
  static std::string getWatchNames(const WatchSet & watches, bool distinct,
    char separator = ' ');

  // Convert a space-separated list of symbolic names to a set of watch
  // types.
  static WatchSet getWatchValues(const std::string & watches,
    const char separator = ' ');

  // Convert vector of symbolic names to a set of watch types.
  static WatchSet getWatchValues(StrVector & watches);

private:
  typedef std::bitset<NUM_WATCHES> _WatchSet;

  // Return the number of watch types in this set.
  size_t count(void) const { return this->contents.count(); };

  // Return true if set is empty
  bool none(void) const { return this->contents.none(); };

  // Remove a watch type or set of watch types from this set.
  void remove(const Watch watch) { this->contents.reset(watch); };
  void remove(const WatchSet & watches)
    { this->contents &= ~watches.contents; };

  _WatchSet contents;
};


#endif /* __WATCH_H__ */

