#ifndef __SETTING_H__
#define __SETTING_H__
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

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

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
    this->changed_ = false;
    this->name_ = name;
  }
  virtual ~Setting() { };

  std::string getName() const { return this->name_; };
  void setName(const std::string & name) { this->name_ = name; };

  bool hasChanged() const { return this->changed_; };

  virtual std::string get() const = 0;
  virtual std::string setString(std::string value) = 0;

  std::string set(const std::string & value)
  {
    std::string error = this->setString(value);
    if (error.length() == 0)
    {
      this->changed_ = true;
    }
    return error;
  };

  virtual std::string getString() const { return ""; };
  virtual bool getBool() const { return false; };
  virtual int getInt() const { return 0; };
  virtual AutoAction getAction() const { return ACTION_NOTHING; };

private:
  std::string name_;
  bool changed_;
};


class StrSetting : public Setting
{
public:
  StrSetting(const std::string & name, const std::string & value)
    : Setting(name)
  {
    this->value_ = value;
  }

  virtual std::string get() const { return this->value_; };
  virtual std::string setString(std::string value)
  {
      this->value_ = value;
      return "";
  }

  virtual std::string getString() const { return this->get(); };

private:
  std::string value_;
};


class BoolSetting : public Setting
{
public:
  BoolSetting(const std::string & name, bool value,
    const std::string & trueString = "ON",
    const std::string & falseString = "OFF") : Setting(name)
  {
    this->value_ = value;
    this->trueString_ = trueString;
    this->falseString_ = falseString;
  }

  virtual std::string get() const
  {
    if (this->value_)
    {
      return trueString_;
    }
    else
    {
      return falseString_;
    }
  };
  virtual std::string setString(std::string value)
  {
    if (Same(value, trueString_))
    {
      this->value_ = true;
      return "";
    }
    else if (Same(value, falseString_))
    {
      this->value_ = false;
      return "";
    }
    else
    {
      return "*** " + this->trueString_ + " or " + this->falseString_ +
	" expected!";
    }
  }

  virtual bool getBool() const { return this->value_; };

private:
  std::string trueString_, falseString_;
  bool value_;
};


class IntSetting : public Setting
{
public:
  IntSetting(const std::string & name, int value, int min = INT_MIN,
    int max = INT_MAX) : Setting(name)
  {
    this->value_ = value;
    this->min_ = min;
    this->max_ = max;
  }

  virtual std::string get() const
  {
    return boost::lexical_cast<std::string>(this->value_);
  }
  virtual std::string setString(std::string value)
  {
    try
    {
      int num = boost::lexical_cast<int>(value);
      if ((num >= this->min_) && (num <= this->max_))
      {
        this->value_ = num;
        return "";
      }
      else
      {
        return "*** Numeric value between " +
	  boost::lexical_cast<std::string>(this->min_) + " and " +
	  boost::lexical_cast<std::string>(this->max_) + " expected!";
      }
    }
    catch (boost::bad_lexical_cast)
    {
      return "*** Numeric value expected!";
    }
  }

  virtual int getInt() const { return this->value_; };

private:
  int value_, min_, max_;
};


class ActionSetting : public Setting
{
public:
  ActionSetting(const std::string & name, const AutoAction & value,
    int duration = 0) : Setting(name)
  {
    this->value_ = value;
    this->duration_ = duration;
  }

  virtual std::string get() const
  {
    std::string duration;
    if (this->getInt() > 0)
    {
      duration += ' ';
      duration += boost::lexical_cast<std::string>(this->getInt());
    }

    switch (this->value_)
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
      case ACTION_SMART_KLINE_HOST:
	return "SMART_KLINE_HOST" + duration;
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
      this->value_ = ACTION_NOTHING;
    else if (action == "KILL")
      this->value_ = ACTION_KILL;
    else if ((action == "KLINE") || (action == "KLINE_USERDOMAIN"))
      this->value_ = ACTION_KLINE;
    else if (action == "KLINE_HOST")
      this->value_ = ACTION_KLINE_HOST;
    else if (action == "KLINE_DOMAIN")
      this->value_ = ACTION_KLINE_DOMAIN;
    else if (action == "KLINE_IP")
      this->value_ = ACTION_KLINE_IP;
    else if (action == "KLINE_USERNET")
      this->value_ = ACTION_KLINE_USERNET;
    else if (action == "KLINE_NET")
      this->value_ = ACTION_KLINE_NET;
    else if (action == "SMART_KLINE")
      this->value_ = ACTION_SMART_KLINE;
    else if (action == "SMART_KLINE_HOST")
      this->value_ = ACTION_SMART_KLINE_HOST;
    else if (action == "SMART_KLINE_IP")
      this->value_ = ACTION_SMART_KLINE_IP;
    else if (action == "DLINE_IP")
      this->value_ = ACTION_DLINE_IP;
    else if (action == "DLINE_NET")
      this->value_ = ACTION_DLINE_NET;
    else
      return "*** NOTHING, KILL, KLINE, KLINE_HOST, KLINE_DOMAIN, KLINE_IP, KLINE_USERNET, KLINE_NET, SMART_KLINE, SMART_KLINE_HOST, SMART_KLINE_IP, DLINE_IP, or DLINE_NET expected!";

    std::string duration = FirstWord(value);
    try
    {
      this->duration_ = boost::lexical_cast<int>(duration);
    }
    catch (boost::bad_lexical_cast)
    {
      this->duration_ = 0;
    }

    return "";
  }

  virtual AutoAction getAction() const { return this->value_; };
  virtual int getInt() const { return this->duration_; };

private:
  AutoAction value_;
  int duration_;
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

