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
#include <fstream>
#include <string>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// OOMon Headers
#include "strtype"
#include "vars.h"
#include "defaults.h"
#include "main.h"
#include "log.h"
#include "util.h"
#include "config.h"
#include "botclient.h"


#ifdef DEBUG
# define VARS_DEBUG
#endif


bool SettingKey::weak(true);


Vars vars;


std::string
Setting::getBoolean(const bool * value)
{
  return *value ? "ON" : "OFF";
}


std::string
Setting::setBoolean(bool * value, const std::string & newValue)
{
  std::string result;

  if (Same(newValue, "ON"))
  {
    *value = true;
  }
  else if (Same(newValue, "OFF"))
  {
    *value = false;
  }
  else
  {
    result = "*** ON or OFF expected!";
  }

  return result;
}


Setting
Setting::BooleanSetting(bool & value)
{
  return Setting(boost::bind(Setting::getBoolean, &value),
      boost::bind(Setting::setBoolean, &value, _1));
}


std::string
Setting::getInteger(const int * value)
{
  return boost::lexical_cast<std::string>(*value);
}


std::string
Setting::setInteger(int * value, const int min, const int max,
    const std::string & newValue)
{
  std::string result;

  try
  {
    int temp = boost::lexical_cast<int>(newValue);

    if ((temp < min) || (temp > max))
    {
      result = "*** Numeric value between";
      result += boost::lexical_cast<std::string>(min);
      result += " and ";
      result += boost::lexical_cast<std::string>(max);
      result += " expected!";
    }
    else
    {
      *value = temp;
    }
  }
  catch (boost::bad_lexical_cast)
  {
    result = "*** Numeric value expected!";
  }

  return result;
}


Setting
Setting::IntegerSetting(int & value, const int min, const int max)
{
  return Setting(boost::bind(Setting::getInteger, &value),
      boost::bind(Setting::setInteger, &value, min, max, _1));
}


std::string
Setting::getString(const std::string * value)
{
  return *value;
}


std::string
Setting::setString(std::string * value, const std::string & newValue)
{
  *value = newValue;

  return "";
}


Setting
Setting::StringSetting(std::string & value)
{
  return Setting(boost::bind(Setting::getString, &value),
      boost::bind(Setting::setString, &value, _1));
}


Vars::Vars(void)
{
}


Vars::~Vars(void)
{
}


void
Vars::get(BotClient * client, const std::string & name) const
{
  const std::string ucName(UpCase(name));
  // Use strong string comparisons to find the lower bound, and weak
  // string comparisons to find the upper bound.
  SettingKey::weak = false;
  const Vars::VarMap::const_iterator begin = this->vm.lower_bound(ucName);
  SettingKey::weak = true;
  const Vars::VarMap::const_iterator end = this->vm.upper_bound(ucName);

  if (0 == std::distance(begin, end))
  {
    std::string error("*** No such variable: ");
    error += name;
    client->send(error);
  }
  else
  {
    for (Vars::VarMap::const_iterator pos = begin; pos != end; ++pos)
    {
      std::string notice("*** ");
      notice += pos->first.get();
      notice += ' ';

      std::string value(pos->second.get());
      if (value.empty())
      {
        notice += "is empty.";
      }
      else
      {
        notice += "= ";
        notice += value;
      }

      client->send(notice);
    }
  }
}


void
Vars::set(BotClient * client, const std::string & name,
    const std::string & value)
{
  const std::string ucName(UpCase(name));
  // Use strong string comparisons to find the lower bound, and weak
  // string comparisons to find the upper bound.
  SettingKey::weak = false;
  const Vars::VarMap::iterator begin = this->vm.lower_bound(ucName);
  SettingKey::weak = true;
  const Vars::VarMap::iterator end = this->vm.upper_bound(ucName);

  if (0 == std::distance(begin, end))
  {
    std::string error("*** No such variable: ");
    error += name;
    client->send(error);
  }
  else if ((std::distance(begin, end) > 1) &&
      (0 != ucName.compare(begin->first.get())))
  {
    std::string error("*** Ambiguous variable name: ");
    error += name;
    client->send(error);
  }
  else
  {
    Vars::VarMap::iterator pos(begin);
    std::string error(pos->second.set(value));

    if (error.empty())
    {
      std::string notice("*** ");
      notice += client->handleAndBot();
      notice += ' ';

      std::string value(pos->second.get());
      if (value.empty())
      {
        notice += "cleared ";
        notice += pos->first.get();
      }
      else
      {
        notice += "set ";
        notice += pos->first.get();
        notice += " to ";
        notice += value;
      }

      if (client->id() != "CONFIG")
      {
        Log::Write(notice);
        ::SendAll(notice, UserFlags::OPER);
        
        if (Config::autoSave())
        {
          config.saveSettings();
        }
      }
    }
    else
    {
      client->send(error);
    }
  }
}


void
Vars::save(std::ofstream & file) const
{
  for (Vars::VarMap::const_iterator pos = this->vm.begin();
      pos != this->vm.end(); ++pos)
  {
    if (pos->second.modified())
    {
      file << "SET ";

      std::string value(pos->second.get());
      if (value.empty())
      {
        file << '-' << pos->first.get();
      }
      else
      {
        file << pos->first.get() << ' ' << pos->second.get();
      }

      file << std::endl;
    }
  }
}


void
Vars::insert(const std::string & name, const Setting & setting)
{
  // We have to use "strong" string comparisons here because we're using
  // map as our container, which doesn't allow for multiple entries with
  // keys that can be evaluated to be "equal".
  SettingKey::weak = false;
  this->vm.insert(std::make_pair(UpCase(name), setting));
  SettingKey::weak = true;
}

