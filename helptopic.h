#ifndef __HELPTOPIC_H__
#define __HELPTOPIC_H__
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

// OOMon Headers
#include "strtype"

class HelpTopic
{
private:
  StrVector topics;
  StrList syntax;
  StrList description;
  StrList example;
  StrList helpLink;
  StrList subTopic;
  int flags;
  bool error;

public:
  HelpTopic(void);
  HelpTopic(const HelpTopic &);
  virtual ~HelpTopic(void);

  bool operator==(const std::string & topic) const;
  bool operator!=(const std::string & topic) const
  {
    return !operator==(topic);
  }

  bool hadError(void) const { return error; }
  StrList getHelp(void);
  StrList getHelp(const std::string &);
};

#endif /* __HELPTOPIC_H__ */

