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

#include <fstream>
#include <iostream>
#include <string>

#include "oomon.h"
#include "log.h"
#include "config.h"
#include "main.h"
#include "util.h"


#ifdef DEBUG
# define LOG_DEBUG
#endif


std::ofstream Log::logfile;
bool Log::IsOpen = false;

void Log::Start()
{
  if (!IsOpen)
  {
#ifdef LOG_DEBUG
    std::cout << "Opening log file for append" << std::endl;
#endif
    logfile.open(Config::GetLogFile().c_str(), std::ios::app);
    IsOpen = true;
  }
}

void Log::Write(const std::string & Text)
{
  if (IsOpen)
  {
    logfile << '[' << ::timeStamp(TIMESTAMP_LOG) << "] " << Text << std::endl;
  }
}

void Log::Stop()
{
  if (IsOpen)
  {
#ifdef LOG_DEBUG
    std::cout << "Closing log file" << std::endl;
#endif
    logfile.close();
    IsOpen = false;
  }
}
