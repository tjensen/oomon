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

// OOMon Headers
#include "strtype"
#include "filter.h"
#include "format.h"
#include "util.h"


Filter::Filter(const std::string & text, const bool extended) : rest_(text)
{
  this->parseFilter(this->rest_, extended);
}


Filter::Filter(const std::string & text, const Filter::Field & defaultField,
    const bool extended) : rest_(text)
{
  std::string::size_type equals = this->rest_.find('=');

  if (std::string::npos == equals)
  {
    std::string pattern(grabPattern(this->rest_, " "));
    this->add(defaultField, smartPattern(pattern, FIELD_NICK == defaultField));
  }
  else
  {
    this->rest_ = text;
    this->parseFilter(this->rest_, extended);
  }
}


Filter::Filter(const Filter::Field & field, const PatternPtr & pattern)
{
  this->add(field, pattern);
}


Filter::Filter(const Filter & copy) : fields_(copy.fields_), rest_(copy.rest_)
{
}


Filter::~Filter(void)
{
}


void
Filter::add(const Filter::Field & field, const PatternPtr & pattern)
{
  this->fields_[field] = pattern;
}


void
Filter::parseFilter(std::string & text, const bool extended)
{
  while (!text.empty())
  {
    std::string::size_type equals = text.find('=');
    if (std::string::npos == equals)
    {
      throw Filter::bad_field("*** Filter is missing field name");
    }
    else
    {
      std::string fieldName(text, 0, equals);
      text.erase(0, equals + 1);

      Filter::Field field(Filter::fieldType(fieldName, extended));
      std::string pattern(grabPattern(text, " ,"));
      this->add(field, smartPattern(pattern, FIELD_NICK == field));
    }

    if (!text.empty())
    {
      if (text[0] == ' ')
      {
        return;
      }
      else if (text[0] == ',')
      {
        text.erase(0, 1);
      }
    }
  }
}


bool
Filter::matches(const UserEntryPtr user, const std::string & version,
    const std::string & privmsg, const std::string & notice) const
{
  for (FieldMap::const_iterator pos = this->fields_.begin();
      pos != this->fields_.end(); ++pos)
  {
    switch (pos->first)
    {
      case FIELD_NICK:
        if (!pos->second->match(user->getNick()))
        {
          return false;
        }
        break;

      case FIELD_USER:
        if (!pos->second->match(user->getUser()))
        {
          return false;
        }
        break;

      case FIELD_HOST:
        if (!pos->second->match(user->getHost()))
        {
          return false;
        }
        break;

      case FIELD_UH:
        if (!pos->second->match(user->getUserHost()) &&
            !pos->second->match(user->getUserIP()))
        {
          return false;
        }
        break;

      case FIELD_NUH:
        if (!pos->second->match(user->getNickUserHost()) &&
            !pos->second->match(user->getNickUserIP()))
        {
          return false;
        }
        break;

      case FIELD_IP:
        if (!pos->second->match(user->getTextIP()))
        {
          return false;
        }
        break;

      case FIELD_GECOS:
        if (!pos->second->match(user->getGecos()))
        {
          return false;
        }
        break;

      case FIELD_NUHG:
        if (!pos->second->match(user->getNickUserHostGecos()) &&
            !pos->second->match(user->getNickUserIPGecos()))
        {
          return false;
        }
        break;

      case FIELD_CLASS:
        if (!pos->second->match(user->getClass()))
        {
          return false;
        }
        break;

      case FIELD_VERSION:
        if (!pos->second->match(version))
        {
          return false;
        }
        break;

      case FIELD_PRIVMSG:
        if (!pos->second->match(privmsg))
        {
          return false;
        }
        break;

      case FIELD_NOTICE:
        if (!pos->second->match(notice))
        {
          return false;
        }
        break;
    }
  }

  return true;
}


bool
Filter::matches(const UserEntryPtr user) const
{
  return this->matches(user, "", "", "");
}


bool
Filter::matches(const Filter::Field & field) const
{
  bool result(false);

  switch (field)
  {
    case FIELD_NICK:
      result = ((this->fields_.end() != this->fields_.find(FIELD_NICK)) ||
          (this->fields_.end() != this->fields_.find(FIELD_UH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUHG)));
      break;
    case FIELD_USER:
      result = ((this->fields_.end() != this->fields_.find(FIELD_USER)) ||
          (this->fields_.end() != this->fields_.find(FIELD_UH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUHG)));
      break;
    case FIELD_HOST:
      result = ((this->fields_.end() != this->fields_.find(FIELD_HOST)) ||
          (this->fields_.end() != this->fields_.find(FIELD_UH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUHG)));
      break;
    case FIELD_IP:
      result = ((this->fields_.end() != this->fields_.find(FIELD_IP)) ||
          (this->fields_.end() != this->fields_.find(FIELD_UH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUH)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUHG)));
      break;
    case FIELD_GECOS:
      result = ((this->fields_.end() != this->fields_.find(FIELD_GECOS)) ||
          (this->fields_.end() != this->fields_.find(FIELD_NUHG)));
      break;
    case FIELD_UH:
    case FIELD_NUH:
    case FIELD_NUHG:
    case FIELD_CLASS:
    case FIELD_VERSION:
    case FIELD_PRIVMSG:
    case FIELD_NOTICE:
      result = (this->fields_.end() != this->fields_.find(field));
      break;
  }

  return result;
}


FormatSet
Filter::formats(void) const
{
  FormatSet result;

  if (this->matches(FIELD_NICK))
  {
    result.set(FORMAT_NICK);
  }
  if (this->matches(FIELD_USER) || this->matches(FIELD_HOST))
  {
    result.set(FORMAT_USERHOST);
  }
  if (this->matches(FIELD_IP))
  {
    result.set(FORMAT_IP);
  }
  if (this->matches(FIELD_CLASS))
  {
    result.set(FORMAT_CLASS);
  }
  if (this->matches(FIELD_GECOS))
  {
    result.set(FORMAT_GECOS);
  }

  return result;
}


std::string
Filter::get(void) const
{
  std::string result;

  for (FieldMap::const_iterator pos = this->fields_.begin();
      pos != this->fields_.end(); ++pos)
  {
    if (!result.empty())
    {
      result += ',';
    }
    result += Filter::fieldName(pos->first);
    result += '=';
    result += pos->second->get();
  }

  return result;
}


Filter::Field
Filter::fieldType(const std::string & field, const bool extended)
{
  std::string uc(UpCase(field));

  if ((0 == uc.compare("N")) || (0 == uc.compare("NICK")))
  {
    return Filter::FIELD_NICK;
  }
  else if ((0 == uc.compare("U")) || (0 == uc.compare("USER")))
  {
    return Filter::FIELD_USER;
  }
  else if ((0 == uc.compare("H")) || (0 == uc.compare("HOST")))
  {
    return Filter::FIELD_HOST;
  }
  else if ((0 == uc.compare("UH")) || (0 == uc.compare("USERHOST")))
  {
    return Filter::FIELD_UH;
  }
  else if ((0 == uc.compare("NUH")) || (0 == uc.compare("NICKUSERHOST")))
  {
    return Filter::FIELD_NUH;
  }
  else if (0 == uc.compare("IP"))
  {
    return Filter::FIELD_IP;
  }
  else if ((0 == uc.compare("G")) || (0 == uc.compare("GECOS")))
  {
    return Filter::FIELD_GECOS;
  }
  else if ((0 == uc.compare("NUHG")) || (0 == uc.compare("NICKUSERHOSTGECOS")))
  {
    return Filter::FIELD_NUHG;
  }
  else if ((0 == uc.compare("C")) || (0 == uc.compare("CLASS")))
  {
    return Filter::FIELD_CLASS;
  }
  else if (extended && (0 == uc.compare("VERSION")))
  {
    return Filter::FIELD_VERSION;
  }
  else if (extended && (0 == uc.compare("PRIVMSG")))
  {
    return Filter::FIELD_PRIVMSG;
  }
  else if (extended && (0 == uc.compare("NOTICE")))
  {
    return Filter::FIELD_NOTICE;
  }

  throw Filter::bad_field("*** Unknown field: " + uc);
}


std::string
Filter::fieldName(const Filter::Field & field)
{
  switch (field)
  {
    case FIELD_NICK:
      return "n";
    case FIELD_USER:
      return "u";
    case FIELD_HOST:
      return "h";
    case FIELD_UH:
      return "uh";
    case FIELD_NUH:
      return "nuh";
    case FIELD_IP:
      return "ip";
    case FIELD_GECOS:
      return "g";
    case FIELD_NUHG:
      return "nuhg";
    case FIELD_CLASS:
      return "c";
    case FIELD_VERSION:
      return "version";
    case FIELD_PRIVMSG:
      return "privmsg";
    case FIELD_NOTICE:
      return "notice";
  }

  throw Filter::bad_field("*** Bad field type");
}

