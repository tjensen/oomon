#ifndef __BOTDB_H__
#define __BOTDB_H__
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
#include <iostream>

// OOMon Headers
#include "oomon.h"

// Conditional Headers
#include <sys/stat.h>
#if defined(HAVE_LIBGDBM)
# include <gdbm.h>
#elif defined(HAVE_BSDDB)
# include <sys/types.h>
# include <limits.h>
# include <fcntl.h>
# include <db.h>
#else
# include <map>
#endif


class BotDB
{
public:
  BotDB(const std::string & file, int mode = (S_IRUSR | S_IWUSR));
  virtual ~BotDB();

  int del(const std::string & key);
  int fd();
  int get(const std::string & key, std::string & data);
  int put(const std::string & key, const std::string & data);
  int sync();

  static int db_errno();
  static std::string strerror(const int db_errno);

private:
#if defined(HAVE_LIBGDBM)
  GDBM_FILE db;
#elif defined(HAVE_BSDDB)
  DB *db;
  static int errno_;
#else
  typedef std::map<std::string, std::string> DBMap;
  DBMap db;
#endif
};


#endif /* __BOTDB_H__ */

