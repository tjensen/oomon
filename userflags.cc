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


#include "strtype"
#include "userflags.h"
#include "util.h"


UserFlags::UserFlags(void) : bits(0)
{
}


UserFlags::UserFlags(const Bit b)
{
  if (MAX_UF_BITS == b)
  {
    bits.set();
  }
  else
  {
    bits.set(b);
  }
}


UserFlags::UserFlags(const Bit b1, const Bit b2)
{
  this->bits = UserFlags(b1).bits;
  this->bits |= UserFlags(b2).bits;
}


UserFlags::UserFlags(const std::string & bits, const char separator) : bits(0)
{
  StrVector tmp;
  StrSplit(tmp, bits, std::string(1, separator), true);

  for (StrVector::iterator pos = tmp.begin(); pos != tmp.end(); ++pos)
  {
    std::string copy(UpCase(*pos));

    if (0 == copy.compare("AUTHED"))
      this->bits.set(UserFlags::AUTHED);
    else if (0 == copy.compare("CHANOP"))
      this->bits.set(UserFlags::CHANOP);
    else if (0 == copy.compare("CONN"))
      this->bits.set(UserFlags::CONN);
    else if (0 == copy.compare("DLINE"))
      this->bits.set(UserFlags::DLINE);
    else if (0 == copy.compare("GLINE"))
      this->bits.set(UserFlags::GLINE);
    else if (0 == copy.compare("KLINE"))
      this->bits.set(UserFlags::KLINE);
    else if (0 == copy.compare("MASTER"))
      this->bits.set(UserFlags::MASTER);
    else if (0 == copy.compare("NICK"))
      this->bits.set(UserFlags::NICK);
    else if (0 == copy.compare("OPER"))
      this->bits.set(UserFlags::OPER);
    else if (0 == copy.compare("REMOTE"))
      this->bits.set(UserFlags::REMOTE);
    else if (0 == copy.compare("WALLOPS"))
      this->bits.set(UserFlags::WALLOPS);
    else throw UserFlags::invalid_flag("Unknown flag: " + copy);
  }
}


UserFlags &
UserFlags::operator |= (const UserFlags & flags)
{
  this->bits |= flags.bits;

  return *this;
}


UserFlags &
UserFlags::operator &= (const UserFlags & flags)
{
  this->bits &= flags.bits;

  return *this;
}

UserFlags
UserFlags::operator & (const UserFlags & flags) const
{
  UserFlags tmp(flags);

  tmp.bits &= this->bits;

  return tmp;
}


bool
UserFlags::operator == (const UserFlags & flags) const
{
  return (this->bits == flags.bits);
}


bool
UserFlags::has(const Bit b) const
{
  return (this->bits.test(b));
}


UserFlags
UserFlags::NONE(void)
{
  return UserFlags();
}


UserFlags
UserFlags::ALL(void)
{
  return UserFlags(MAX_UF_BITS);
}


std::string
UserFlags::getName(const UserFlags::Bit b)
{
  if (b == UserFlags::AUTHED) return "AUTHED";
  else if (b == UserFlags::CHANOP) return "CHANOP";
  else if (b == UserFlags::CONN) return "CONN";
  else if (b == UserFlags::DLINE) return "DLINE";
  else if (b == UserFlags::GLINE) return "GLINE";
  else if (b == UserFlags::KLINE) return "KLINE";
  else if (b == UserFlags::MASTER) return "MASTER";
  else if (b == UserFlags::NICK) return "NICK";
  else if (b == UserFlags::OPER) return "OPER";
  else if (b == UserFlags::REMOTE) return "REMOTE";
  else if (b == UserFlags::WALLOPS) return "WALLOPS";
  else throw UserFlags::invalid_flag("Unknown flag");
}


std::string
UserFlags::getNames(const UserFlags & flags, const char separator)
{
  std::string result;

  for (int b = 0; b < MAX_UF_BITS; ++b)
  {
    if (flags.has(static_cast<Bit>(b)))
    {
      if (!result.empty()) result += separator;
      result += UserFlags::getName(static_cast<Bit>(b));
    }
  }

  if (result.empty())
  {
    result = "NONE";
  }

  return result;
}

