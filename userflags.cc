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


#include "userflags.h"


const UserFlags UserFlags::NONE;


UserFlags::UserFlags(void) : bits(0)
{
}


UserFlags::UserFlags(const Bit b) : bits(1 << b)
{
}


UserFlags::UserFlags(const Bit b1, const Bit b2)
{
  this->bits = UserFlags(b1).bits;
  this->bits |= UserFlags(b2).bits;
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

