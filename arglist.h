#ifndef __ARGLIST_H__
#define __ARGLIST_H__
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
#include <map>

// OOMon Headers
#include "strtype"


class ArgList
{
public:
  ArgList(std::string unaries, std::string binaries);
  virtual ~ArgList() { };

  int parseCommand(std::string & command);

  bool haveUnary(const std::string & unary) const;
  bool haveBinary(const std::string & binary, std::string & parameter) const;

  std::string getInvalid(void) const { return this->invalid; };

private:
  StrList unaries;
  StrList binaries;

  StrList haveUnaries;
  std::map<std::string, std::string> haveBinaries;

  std::string invalid;
};

#endif /* __ARGLIST_H__ */

