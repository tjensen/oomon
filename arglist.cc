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

#include <string>
#include <map>
#include <algorithm>
#include <functional>

#include "strtype"
#include "arglist.h"
#include "util.h"


ArgList::ArgList(std::string unaries, std::string binaries)
{
  unaries = UpCase(unaries);
  binaries = UpCase(binaries);

  while (unaries.length() > 0)
  {
    unaries = trimLeft(unaries);

    std::string::size_type space = unaries.find_first_of(" \t");

    this->unaries.push_back(unaries.substr(0, space));

    unaries.erase(0, space);
  }

  while (binaries.length() > 0)
  {
    binaries = trimLeft(binaries);

    std::string::size_type space = binaries.find_first_of(" \t");

    this->binaries.push_back(binaries.substr(0, space));

    binaries.erase(0, space);
  }
}


int
ArgList::parseCommand(std::string & command)
{
  int result = 0;

  this->haveUnaries.clear();
  this->haveBinaries.clear();
  invalid = "";

  command = trimLeft(command);

  while ((command.length() > 0) && (command[0] == '-'))
  {
    std::string arg = FirstWord(command);

    bool foundIt = false;

    // Check unaries

    if (this->unaries.end() != std::find(this->unaries.begin(),
      this->unaries.end(), UpCase(arg)))
    {
      foundIt = true;
      this->haveUnaries.push_back(UpCase(arg));
      result++;
    }

    // Check binaries
    if (!foundIt)
    {
      if (this->binaries.end() != std::find(this->binaries.begin(),
	this->binaries.end(), UpCase(arg)))
      {
        foundIt = true;
        this->haveBinaries[UpCase(arg)] = FirstWord(command);
        result++;
      }
    }

    // Anything else is invalid
    if (!foundIt)
    {
      this->invalid = arg;
      return -1;
    }
  }

  return result;
}


bool
ArgList::haveUnary(const std::string & unary) const
{
  return (this->haveUnaries.end() != std::find(this->haveUnaries.begin(),
    this->haveUnaries.end(), UpCase(unary)));
}


bool
ArgList::haveBinary(const std::string & binary, std::string & parameter) const
{
  std::map<std::string, std::string>::const_iterator pos =
    this->haveBinaries.find(UpCase(binary));

  if (pos != this->haveBinaries.end())
  {
    parameter = pos->second;
    return true;
  }

  return false;
}

