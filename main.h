#ifndef _MAIN_H_
#define _MAIN_H_
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
#include <csignal>

// OOMon Headers
#include "oomon.h"
#include "userflags.h"
#include "watch.h"


enum OOMonExitCode
{
  EXIT_NOERROR, EXIT_ALREADY_RUNNING, EXIT_CMDLINE_ERROR, EXIT_CONFIG_ERROR,
  EXIT_FORK_ERROR, EXIT_BOTDB_ERROR
};


RETSIGTYPE gracefuldie(int sig);
void ReloadConfig(const std::string & from);
void SendAll(const std::string & message,
  const class UserFlags flags = UserFlags::NONE(),
  const class WatchSet & watches = WatchSet(),
  const class BotClient * skip = 0);
void motd(class BotClient * client);
std::string getUptime(void);

#endif

