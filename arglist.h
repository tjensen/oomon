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
#include <set>
#include <map>

// OOMon Headers
#include "strtype"
#include "pattern.h"


class ArgList
{
public:
  ArgList(std::string unaries = "", std::string binaries = "");
  virtual ~ArgList() { };

  void addPatterns(std::string patterns);

  int parseCommand(std::string & command);

  bool haveUnary(const std::string & unary) const;
  bool haveBinary(const std::string & binary, std::string & parameter) const;
  bool havePattern(const std::string & pattern, PatternPtr & parameter) const;

  std::string getInvalid(void) const { return this->invalid_; };

private:
  typedef std::set<std::string> StrSet;
  StrSet unaries_;
  StrSet binaries_;
  StrSet patterns_;

  StrSet haveUnaries_;
  std::map<std::string, std::string> haveBinaries_;
  std::map<std::string, PatternPtr> havePatterns_;

  std::string invalid_;
};

#endif /* __ARGLIST_H__ */

