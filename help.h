#ifndef __HELP_H__
#define __HELP_H__
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
#include <list>

// OOMon Headers
#include "strtype"
#include "helptopic.h"

class Help
{
private:
    
#if __cplusplus >= 201103L // C++ v11 or newer    
  static thread_local std::list<HelpTopic> topics;
  static thread_local StrList topicList;
#else
  static std::list<HelpTopic> topics;
  static StrList topicList;  
#endif

  static bool haveTopic(const std::string &);
  static bool getIndex(class BotClient * client);

public:
  static void getHelp(class BotClient * client, const std::string & topic);
  static void flush(void);
};

#endif /* __HELP_H__ */

