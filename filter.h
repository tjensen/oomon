#ifndef __FILTER_H__
#define __FILTER_H__
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
#include "strtype"
#include "oomon.h"
#include "botsock.h"
#include "botexcept.h"
#include "pattern.h"
#include "userentry.h"


class Filter
{
public:
  enum Field
  {
    FIELD_NICK, FIELD_USER, FIELD_HOST, FIELD_UH, FIELD_NUH, FIELD_IP,
    FIELD_GECOS, FIELD_NUHG, FIELD_CLASS,
    // Extended fields:
    FIELD_VERSION, FIELD_PRIVMSG, FIELD_NOTICE
  };

  Filter(const std::string & text, const bool extended = false);
  Filter(const std::string & text, const Filter::Field & defaultField,
      const bool extended = false);
  Filter(const Filter::Field & field, const PatternPtr & pattern);
  Filter(const Filter & copy);
  virtual ~Filter(void);

  void add(const Filter::Field & field, const PatternPtr & pattern);

  void parseFilter(std::string & text, const bool extended);

  bool matches(const UserEntryPtr & user, const std::string & version,
      const std::string & privmsg, const std::string & notice) const;
  bool matches(const UserEntryPtr & user) const;
  bool matches(const Filter::Field & field) const;

  std::string get(void) const;

  std::string rest(void) const
  {
    return this->rest_;
  }

  class bad_field : public OOMon::oomon_error
  {
  public:
    bad_field(const std::string & arg) : oomon_error(arg) { };
  };

private:
  typedef std::map<Field, PatternPtr> FieldMap;
  FieldMap fields_;
  std::string rest_;

  static Field fieldType(const std::string & field, const bool extended);
  static std::string fieldName(const Field & field);
};


#endif /* __FILTER_H__ */

