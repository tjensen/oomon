#ifndef __USERDB_H__
#define __USERDB_H__
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

// OOMon Headers
#include "botdb.h"


class UserDB : private BotDB
{
public:
  UserDB(const std::string & file) : BotDB(file) { this->_file = file; };

  bool getEcho(const std::string & handle);
  void setEcho(const std::string & handle, const bool value);

  std::string getWatches(const std::string & handle);
  void setWatches(const std::string & handle, const std::string & watches);

  std::string getFile() const { return this->_file; };

private:
  std::string _file;
};


extern UserDB *userConfig;


#endif /* __USERDB_H__ */

