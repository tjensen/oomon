#ifndef __USERFLAGS_H__
#define __USERFLAGS_H__
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
#include <bitset>

// OOMon Headers
#include "botexcept.h"


class UserFlags
{
public:
  enum Bit
  {
    AUTHED, CHANOP, OPER, WALLOPS, CONN, NICK, KLINE, GLINE, DLINE, REMOTE,
    MASTER,

    MAX_UF_BITS
  };

  UserFlags(void);
  UserFlags(const Bit b);
  UserFlags(const Bit ba, const Bit bb);
  UserFlags(const std::string & bits, const char separator);

  ~UserFlags() { }

  UserFlags & operator |= (const UserFlags & flags);
  UserFlags & operator &= (const UserFlags & flags);
  UserFlags operator & (const UserFlags & flags) const;
  bool operator == (const UserFlags & flags) const;

  bool has(const Bit b) const;

  static UserFlags NONE(void);
  static UserFlags ALL(void);

  static std::string getName(const Bit b);
  static std::string getNames(const UserFlags & flags, const char separator);

  class invalid_flag : public OOMon::oomon_error
  {
  public:
    invalid_flag(const std::string & arg) : oomon_error(arg) { }
  };

private:
  std::bitset<MAX_UF_BITS> bits;
};


#endif /* __USERFLAGS_H__ */

