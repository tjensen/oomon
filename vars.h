#ifndef OOMON_VARS_H__
#define OOMON_VARS_H__
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
#include <map>

// Boost C++ Headers
#include <boost/function.hpp>

// Std C Headers
#include <limits.h>

// OOMon Headers
#include "util.h"


class Setting
{
  public:
    typedef boost::function<std::string(void)> GetFunction;
    typedef boost::function<std::string(std::string)> SetFunction;

    Setting(const GetFunction getter, const SetFunction setter)
      : getter_(getter), setter_(setter), modified_(false) { }
    Setting(const Setting & copy)
      : getter_(copy.getter_), setter_(copy.setter_),
      modified_(copy.modified_) { }
    ~Setting(void) { }

    std::string get(void) const
    {
      return this->getter_();
    }
    std::string set(const std::string & value)
    {
      std::string result(this->setter_(value));

      if (result.empty())
      {
        this->modified_ = true;
      }

      return result;
    }

    bool modified(void) const
    {
      return this->modified_;
    }

    static std::string getBoolean(const bool * value);
    static std::string setBoolean(bool * value, const std::string & newValue);
    static Setting BooleanSetting(bool & value);

    static std::string getInteger(const int * value);
    static std::string setInteger(int * value, const int min, const int max,
        const std::string & newValue);
    static Setting IntegerSetting(int & value, const int min = INT_MIN,
        const int max = INT_MAX);

    static std::string getString(const std::string * value);
    static std::string setString(std::string * value,
        const std::string & newValue);
    static Setting StringSetting(std::string & value);

  private:
    const GetFunction getter_;
    const SetFunction setter_;
    bool modified_;
};


class SettingKey
{
  public:
    SettingKey(const std::string & name) : name_(UpCase(name)) { }

    const std::string & get(void) const { return this->name_; }

    bool operator<(const SettingKey & right) const
    {
      bool result;

      if (SettingKey::weak)
      {
        std::string part(right.name_, 0, this->name_.length());

        result = (this->name_.compare(part) < 0);
      }
      else
      {
        result = (this->name_.compare(right.name_) < 0);
      }

      return result;
    }

    // Weak comparisons -- When enabled, only compare the number of
    // characters present in the left hand string.  When disabled,
    // compare all characters of both strings.
    static bool weak;

  private:
    const std::string name_;
};


class Vars
{
  public:
    Vars(void);
    ~Vars(void);

    void get(class BotClient * client, const std::string & name) const;
    void set(class BotClient * client, const std::string & name,
        const std::string & value);
    void save(std::ofstream & file) const;

    void insert(const std::string & name, const Setting & setting);

  private:
    // We're using std::map rather unconventionally here, so it might not
    // work the same on all platforms.  If that's the case, we'll have to
    // refactor this code to use a different container.
    typedef std::map<SettingKey, Setting> VarMap;
    VarMap vm;
};


extern Vars vars;


#endif /* OOMON_VARS_H__ */

