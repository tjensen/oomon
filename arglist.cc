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

#include <string>
#include <set>
#include <map>
#include <algorithm>

#include "strtype"
#include "arglist.h"
#include "pattern.h"
#include "util.h"


ArgList::ArgList(std::string unaries, std::string binaries)
{
  while (unaries.length() > 0)
  {
    unaries = trimLeft(unaries);

    std::string::size_type space = unaries.find_first_of(" \t");

    this->unaries_.insert(UpCase(unaries.substr(0, space)));

    unaries.erase(0, space);
  }

  while (binaries.length() > 0)
  {
    binaries = trimLeft(binaries);

    std::string::size_type space = binaries.find_first_of(" \t");

    this->binaries_.insert(UpCase(binaries.substr(0, space)));

    binaries.erase(0, space);
  }
}


void
ArgList::addPatterns(std::string patterns)
{
  while (patterns.length() > 0)
  {
    patterns = trimLeft(patterns);

    std::string::size_type space = patterns.find_first_of(" \t");

    this->patterns_.insert(UpCase(patterns.substr(0, space)));

    patterns.erase(0, space);
  }
}


int
ArgList::parseCommand(std::string & command)
{
  int result = 0;

  this->haveUnaries_.clear();
  this->haveBinaries_.clear();
  this->invalid_.erase();

  command = trimLeft(command);

  while ((command.length() > 0) && (command[0] == '-'))
  {
    std::string arg = FirstWord(command);

    // Check unaries
    if (this->unaries_.end() != this->unaries_.find(UpCase(arg)))
    {
      this->haveUnaries_.insert(UpCase(arg));
      ++result;
    }
    // Check binaries
    else if (this->binaries_.end() != this->binaries_.find(UpCase(arg)))
    {
      this->haveBinaries_[UpCase(arg)] = FirstWord(command);
      ++result;
    }
    // Check pattern binaries
    else if (this->patterns_.end() != this->patterns_.find(UpCase(arg)))
    {
      this->havePatterns_[UpCase(arg)] = smartPattern(grabPattern(command),
          false);
      command = trimLeft(command);
      ++result;
    }
    // Anything else is invalid
    else
    {
      this->invalid_ = arg;
      return -1;
    }
  }

  return result;
}


bool
ArgList::haveUnary(const std::string & unary) const
{
  return (this->haveUnaries_.end() != this->haveUnaries_.find(UpCase(unary)));
}


bool
ArgList::haveBinary(const std::string & binary, std::string & parameter) const
{
  std::map<std::string, std::string>::const_iterator pos =
    this->haveBinaries_.find(UpCase(binary));

  if (pos != this->haveBinaries_.end())
  {
    parameter = pos->second;
    return true;
  }

  return false;
}


bool
ArgList::havePattern(const std::string & pattern, PatternPtr & parameter) const
{
  std::map<std::string, PatternPtr>::const_iterator pos =
    this->havePatterns_.find(UpCase(pattern));

  if (pos != this->havePatterns_.end())
  {
    parameter = pos->second;
    return true;
  }

  return false;
}

