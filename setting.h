#ifndef __SETTING_H__
#define __SETTING_H__
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

// Std C Headers
#include <limits.h>

// OOMon Headers
#include "autoaction.h"
#include "util.h"
#include "irc.h"


class Setting
{
public:
  explicit Setting(const std::string & name)
  {
    this->_changed = false;
    this->_name = name;
  }
  virtual ~Setting() { };

  std::string getName() const { return this->_name; };
  void setName(const std::string & name) { this->_name = name; };

  bool hasChanged() const { return this->_changed; };

  virtual std::string get() const = 0;
  virtual std::string setString(std::string value) = 0;

  std::string set(const std::string & value)
  {
    std::string error = this->setString(value);
    if (error.length() == 0)
    {
      this->_changed = true;
    }
    return error;
  };

  virtual std::string getString() const { return ""; };
  virtual bool getBool() const { return false; };
  virtual int getInt() const { return 0; };
  virtual AutoAction getAction() const { return ACTION_NOTHING; };

private:
  std::string _name;
  bool _changed;
};


class StrSetting : public Setting
{
public:
  StrSetting(const std::string & name, const std::string & value)
    : Setting(name)
  {
    this->_value = value;
  }

  virtual std::string get() const { return this->_value; };
  virtual std::string setString(std::string value)
  {
      this->_value = value;
      return "";
  }

  virtual std::string getString() const { return this->get(); };

private:
  std::string _value;
};


class BoolSetting : public Setting
{
public:
  BoolSetting(const std::string & name, bool value,
    const std::string & trueString = "ON",
    const std::string & falseString = "OFF") : Setting(name)
  {
    this->_value = value;
    this->_trueString = trueString;
    this->_falseString = falseString;
  }

  virtual std::string get() const
  {
    if (this->_value)
    {
      return _trueString;
    }
    else
    {
      return _falseString;
    }
  };
  virtual std::string setString(std::string value)
  {
    if (Same(value, _trueString))
    {
      this->_value = true;
      return "";
    }
    else if (Same(value, _falseString))
    {
      this->_value = false;
      return "";
    }
    else
    {
      return "*** " + this->_trueString + " or " + this->_falseString +
	" expected!";
    }
  }

  virtual bool getBool() const { return this->_value; };

private:
  std::string _trueString, _falseString;
  bool _value;
};


class IntSetting : public Setting
{
public:
  IntSetting(const std::string & name, int value, int min = INT_MIN,
    int max = INT_MAX) : Setting(name)
  {
    this->_value = value;
    this->_min = min;
    this->_max = max;
  }

  virtual std::string get() const { return IntToStr(this->_value); };
  virtual std::string setString(std::string value)
  {
    if (isNumeric(value))
    {
      int num = atoi(value.c_str());
      if ((num >= this->_min) && (num <= this->_max))
      {
        this->_value = atoi(value.c_str());
        return "";
      }
      else
      {
        return "*** Numeric value between " + IntToStr(this->_min) + " and " +
	  IntToStr(this->_max) + " expected!";
      }
    }
    else
    {
      return "*** Numeric value expected!";
    }
  }

  virtual int getInt() const { return this->_value; };

private:
  int _value, _min, _max;
};


class ActionSetting : public Setting
{
public:
  ActionSetting(const std::string & name, const AutoAction & value,
    int duration = 0) : Setting(name)
  {
    this->_value = value;
    this->_duration = duration;
  }

  virtual std::string get() const
  {
    std::string duration = "";
    if (this->getInt() > 0)
    {
      duration = " " + IntToStr(this->getInt());
    }

    switch (this->_value)
    {
      case ACTION_KILL:
	return "KILL";
      case ACTION_KLINE:
	return "KLINE" + duration;
      case ACTION_KLINE_HOST:
	return "KLINE_HOST" + duration;
      case ACTION_KLINE_DOMAIN:
	return "KLINE_DOMAIN" + duration;
      case ACTION_KLINE_IP:
	return "KLINE_IP" + duration;
      case ACTION_KLINE_USERNET:
	return "KLINE_USERNET" + duration;
      case ACTION_KLINE_NET:
	return "KLINE_NET" + duration;
      case ACTION_SMART_KLINE:
	return "SMART_KLINE" + duration;
      case ACTION_SMART_KLINE_IP:
	return "SMART_KLINE_IP" + duration;
      case ACTION_DLINE_IP:
	return "DLINE_IP" + duration;
      case ACTION_DLINE_NET:
	return "DLINE_NET" + duration;
      default:
	return "NOTHING";
    }
  };
  virtual std::string setString(std::string value)
  {
    std::string action = UpCase(FirstWord(value));

    if (action == "NOTHING")
      this->_value = ACTION_NOTHING;
    else if (action == "KILL")
      this->_value = ACTION_KILL;
    else if ((action == "KLINE") || (action == "KLINE_USERDOMAIN"))
      this->_value = ACTION_KLINE;
    else if (action == "KLINE_HOST")
      this->_value = ACTION_KLINE_HOST;
    else if (action == "KLINE_DOMAIN")
      this->_value = ACTION_KLINE_DOMAIN;
    else if (action == "KLINE_IP")
      this->_value = ACTION_KLINE_IP;
    else if (action == "KLINE_USERNET")
      this->_value = ACTION_KLINE_USERNET;
    else if (action == "KLINE_NET")
      this->_value = ACTION_KLINE_NET;
    else if (action == "SMART_KLINE")
      this->_value = ACTION_SMART_KLINE;
    else if (action == "SMART_KLINE_IP")
      this->_value = ACTION_SMART_KLINE_IP;
    else if (action == "DLINE_IP")
      this->_value = ACTION_DLINE_IP;
    else if (action == "DLINE_NET")
      this->_value = ACTION_DLINE_NET;
    else
      return "*** NOTHING, KILL, KLINE, KLINE_HOST, KLINE_DOMAIN, DLINE_IP, or DLINE_NET expected!";

    this->_duration = 0;

    std::string duration = FirstWord(value);
    if (isNumeric(duration))
    {
      int num = atoi(duration.c_str());
      if (num > 0)
      {
	this->_duration = num;
      }
    }

    return "";
  }

  virtual AutoAction getAction() const { return this->_value; };
  virtual int getInt() const { return this->_duration; };

private:
  AutoAction _value;
  int _duration;
};

class UmodeSetting : public StrSetting
{
public:
  UmodeSetting(const std::string & name, const std::string & value)
    : StrSetting(name, value)
  { };
  virtual std::string setString(std::string value)
  {
    std::string oldValue = this->get();
    std::string result = this->StrSetting::setString(value);
    if ((this->get() != oldValue) && server.isConnected())
    {
      server.umode(this->get());
    }
    return result;
  }
};

class ServerTimeoutSetting : public IntSetting
{
public:
  ServerTimeoutSetting(const std::string & name, int value)
    : IntSetting(name, value, 30, INT_MAX) { }

  virtual std::string setString(std::string value)
  {
    int oldValue = this->getInt();

    std::string result = IntSetting::setString(value);

    int newValue = this->getInt();

    if (server.isConnected() && (newValue != oldValue))
    {
      server.setTimeout(newValue);
    }

    return result;
  }
};

#endif /* __SETTING_H_ */

