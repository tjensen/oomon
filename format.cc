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
#include "oomon.h"
#include "format.h"
#include "util.h"


Format::Format(void)
{
}


Format::~Format(void)
{
}


void
Format::setStringToken(const char token, const std::string & value)
{
  if ((token != 0) && (token != '%'))
  {
    this->tokens.insert(std::make_pair(UpCase(token), value));
  }
}


std::string
Format::format(const std::string & text) const
{
  std::string result;

  for (std::string::size_type idx = 0; idx < text.length(); ++idx)
  {
    if (text[idx] == '%')
    {
      ++idx;

      if (text[idx] == '%')
      {
	result += '%';
      }
      else
      {
	Format::TokenMap::const_iterator pos = tokens.find(UpCase(text[idx]));

	if (pos == tokens.end())
	{
	  result += '%';
	  result += text[idx];
	}
	else
	{
	  result += pos->second;
	}
      }
    }
    else
    {
      result += text[idx];
    }
  }

  return result;
}

