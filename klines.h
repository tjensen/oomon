#ifndef _KLINES_H_
#define _KLINES_H_
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

// C++ Headers
#include <string>
#include <list>

// OOMon Headers
#include "strtype"
#include "pattern.h"
#include "util.h"

class KlineItem
{
public:
  KlineItem(const std::string & userhost = "",
    const std::string & reason = "", const bool temporary = false)
    : userhost_(userhost), reason_(reason), temporary_(temporary) { };

  bool operator==(const std::string & other) const
  {
    return Same(this->userhost_, other);
  }
  bool operator==(const KlineItem & other) const
  {
    return Same(this->userhost_, other.userhost_);
  }

  std::string getUserhost(void) const { return this->userhost_; };
  std::string getReason(void) const { return this->reason_; };
  bool isTemporary(void) const { return this->temporary_; };

private:
  const std::string userhost_;
  const std::string reason_;
  const bool temporary_;
};


class KlineList
{
public:
  static void init(void);

  typedef std::list<KlineItem>::size_type size_type;

  KlineList(const char lineType = 'K') { this->lineType = lineType; }
  void Clear();
  void Add(const std::string &, const std::string &,
    const bool temporary = false);
  void find(class BotClient * client, const Pattern *userhost,
    const bool count = false, const bool searchPerms = true,
    const bool searchTemps = true, const bool searchReason = false) const;
  int findAndRemove(const std::string & from, const Pattern *userhost,
    const bool searchPerms = true, const bool searchTemps = true,
    const bool searchReason = false);
  void Remove(const std::string &);
  bool parseAndAdd(std::string text);
  bool onExpireNotice(std::string text);
  bool parseAndRemove(std::string text);
  KlineList::size_type size() const { return Klines.size(); };
  KlineList::size_type permSize() const;

private:
  std::list<KlineItem> Klines; 
  char lineType;

  static bool extraInfo;
};

#endif
