#ifndef __BOTEXCEPT_H__
#define __BOTEXCEPT_H__
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

// Std C++ Headers
#include <string>
#include <stdexcept>

// Std C Headers
#include <errno.h>

// OOMon Headers
#include "oomon.h"
#include "botdb.h"


namespace OOMon
{

  class oomon_error
  {
  public:
    oomon_error(const std::string & arg) { _what = arg; };
    virtual ~oomon_error(void) { };
    virtual std::string what() const { return _what; };

  private:
    std::string _what;
  };

  class regex_error : public oomon_error
  {
  public:
    regex_error(const std::string & arg) : oomon_error(arg) { };
  };

  class errno_error : public oomon_error
  {
  public:
    errno_error(const std::string & arg) : oomon_error(arg)
      { this->_errno = errno; };
#if defined(HAVE_STRERROR)
    std::string why() const { return ::strerror(this->_errno); };
#else
    std::string why() const { return std::string("error"); };
#endif
    int getErrno() const { return this->_errno; };

  private:
    int _errno;
  };

  class botsock_error : public oomon_error {
  public:
    botsock_error(const std::string & arg): oomon_error(arg) { };
  };

  class open_error : public errno_error {
  public:
    open_error(const std::string & arg): errno_error(arg) { };
  };

  class close_error : public errno_error {
  public:
    close_error(const std::string & arg): errno_error(arg) { };
  };

  class socket_error : public errno_error {
  public:
    socket_error(const std::string & arg): errno_error(arg) { };
  };

  class accept_error : public errno_error {
  public:
    accept_error(const std::string & arg): errno_error(arg) { };
  };

  class timeout_error : public botsock_error {
  public:
    timeout_error(const std::string & arg): botsock_error(arg) { };
  };

  class ready_for_accept : public botsock_error {
  public:
    ready_for_accept(const std::string & arg): botsock_error(arg) { };
  };

  class botdb_error : public oomon_error {
  public:
    botdb_error(const std::string & arg): oomon_error(arg)
      { this->_errno = BotDB::db_errno(); };
    std::string why() const { return BotDB::strerror(this->_errno); };

  private:
    int _errno;
  };

  class norecord_error : public oomon_error {
  public:
    norecord_error(const std::string & arg): oomon_error(arg) { };
  };

  class watch_error : public oomon_error {
  public:
    watch_error(const std::string & arg): oomon_error(arg) { };
  };

  class invalid_watch_name : public watch_error {
  public:
    invalid_watch_name(const std::string & arg): watch_error(arg) { };
  };

  class invalid_watch_value : public watch_error {
  public:
    invalid_watch_value(const std::string & arg): watch_error(arg) { };
  };

  class invalid_action : public oomon_error {
  public:
    invalid_action(const std::string & arg): oomon_error(arg) { };
  };

  class notice_parse_error : public oomon_error {
  public:
    notice_parse_error(const std::string & arg): oomon_error(arg) { };
  };
};

#endif /* __BOTEXCEPT_H__ */

