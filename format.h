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

#ifndef __FORMAT_H__
#define __FORMAT_H__

// Std C++ Headers
#include <string>
#include <map>
#include <bitset>


class Format
{
public:
  Format(void);
  ~Format(void);

  void setStringToken(const char token, const std::string & value);

  std::string format(const std::string & text) const;

private:
  typedef std::map<char, std::string> TokenMap;
  TokenMap tokens;
};


enum FormatType
{
  FORMAT_NICK, FORMAT_USERHOST, FORMAT_IP, FORMAT_CLASS, FORMAT_GECOS,

  MAX_FORMAT
};


typedef std::bitset<MAX_FORMAT> FormatSet;


#endif /* __FORMAT_H__ */

