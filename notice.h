#ifndef __NOTICE_H__
#define __NOTICE_H__
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
#include <list>
#include <algorithm>
#include <functional>

// Std C Headers
#include <time.h>

// OOMon Headers
#include "strtype"
#include "vars.h"
#include "util.h"
#include "main.h"
#include "botexcept.h"


template<typename T>
class NoticeList
{
private:
  class NoticeEntry
  {
  public:
    NoticeEntry(const T & special, const int & count, const time_t & last)
    : _special(special), _count(count), _last(last) { }

    bool operator==(const NoticeEntry & rhs) const { return (this->_special == rhs._special); }

    void update(const time_t & now, const T & data)
    {
      this->_special.update(data);

      ++(this->_count);

      if (this->_special.triggered(this->_count, now - this->_last))
      {
	this->_special.execute();
	this->_count = 0;
      }

      this->_last = now;
    }

    bool expired(const time_t now) const
    {
      return this->_special.expired(now - this->_last);
    }

    T getSpecial(void) const { return this->_special; }
    int getCount(void) const { return this->_count; }
    time_t getLast(void) const { return this->_last; }

  private:
    T		_special;
    int		_count;
    time_t	_last;
  };

  std::list<NoticeEntry> list;

public:
  void clear(void)
  {
    this->list.clear();
  }

  void onNotice(const std::string & notice)
  {
    time_t now = time(NULL);

    this->expire(now);

    try
    {
      T temp(notice);
      NoticeEntry item(temp, 1, now);

      typename std::list<NoticeEntry>::iterator i =
	std::find(this->list.begin(), this->list.end(), item);

      if (i == this->list.end())
      {
	this->list.push_back(item);
      }
      else
      {
	i->update(now, temp);
      }
    }
    catch (OOMon::notice_parse_error)
    {
      // Ignore the notice for now
    }
  }

  int size(void) const { return this->list.size(); };

private:
  void expire(const time_t now)
  {
    this->list.remove_if(std::bind2nd(std::mem_fun_ref(&NoticeEntry::expired), now));
  }
};


class SimpleNoticeEntry
{
public:
  virtual ~SimpleNoticeEntry(void) { }

  bool triggered(const int & count, const time_t & interval) const
  {
    return (!Config::IsOKHost(this->userhost) &&
      !Config::IsOper(this->userhost));
  }

  virtual void execute(void) const = 0;

  virtual bool expired(const time_t interval) const = 0;

  void update(const SimpleNoticeEntry & data)
  {
    this->nick = data.nick;
  }

  bool operator==(const SimpleNoticeEntry & rhs) const
  {
    return Same(this->userhost, rhs.userhost);
  }

protected:
  std::string nick;
  std::string userhost;
};


class FloodNoticeEntry : public SimpleNoticeEntry
{
public:
  explicit FloodNoticeEntry(const std::string & notice)
  {
    // Possible Flooder MH-MP3-06[wil8d@211.206.83.10] on irc.limelight.us target: MetalHead
    std::string text(notice);
  
    if (DownCase(FirstWord(text)) != "possible")
      throw OOMon::notice_parse_error("flood notice");
    if (DownCase(FirstWord(text)) != "flooder")
      throw OOMon::notice_parse_error("flood notice");

    std::string nuh = FirstWord(text);

    this->nick = chopNick(nuh);
    this->userhost = chopUserhost(nuh);

    if (DownCase(FirstWord(text)) != "on")
      throw OOMon::notice_parse_error("flood notice");

    this->server = FirstWord(text);

    if (DownCase(FirstWord(text)) != "target:")
      throw OOMon::notice_parse_error("flood notice");

    this->target = FirstWord(text);

    Log::Write("*** " + notice);
    ::SendAll("*** " + notice, UF_OPER, WATCH_FLOODERS);
  }

  bool triggered(const int & count, const time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_FLOODER_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_FLOODER_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Flooder detected: " + this->nick + " (" +
      this->userhost + ")", UF_OPER, WATCH_FLOODERS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_FLOODER_ACTION]->getAction(),
      vars[VAR_FLOODER_ACTION]->getInt(),
      vars[VAR_FLOODER_REASON]->getString(), true);
  }

  bool expired(const time_t interval) const
  {
    return (interval > vars[VAR_FLOODER_MAX_TIME]->getInt());
  }

  void update(const FloodNoticeEntry & data)
  {
    SimpleNoticeEntry::update(data);

    this->server = data.server;
    this->target = data.target;
  }

protected:
  std::string server;
  std::string target;
};

class SpambotNoticeEntry : public SimpleNoticeEntry
{
public:
  explicit SpambotNoticeEntry(const std::string & notice)
  {
    // User DruggDeal (CRaSH@pool-141-154-236-242.bos.east.verizon.net) trying to join #movies is a possible spambot
    // User DruggDeal (CRaSH@pool-141-154-236-242.bos.east.verizon.net) is a possible spambot
    std::string text(notice);
  
    if (DownCase(FirstWord(text)) != "user")
      throw OOMon::notice_parse_error("spambot notice");

    this->nick = FirstWord(text);

    this->userhost = FirstWord(text);
    // Remove the parentheses surrounding the user@host
    if (this->userhost.length() >= 2)
    {
      this->userhost = this->userhost.substr(1, this->userhost.length() - 2);
    }

    std::string word = DownCase(FirstWord(text));

    if (word == "trying")
    {
      if (DownCase(FirstWord(text)) != "to")
        throw OOMon::notice_parse_error("spambot notice");

      if (DownCase(FirstWord(text)) != "join")
        throw OOMon::notice_parse_error("spambot notice");

      this->channel = FirstWord(text);

      word = DownCase(FirstWord(text));
    }

    if (word != "is")
      throw OOMon::notice_parse_error("spambot notice");

    if (DownCase(FirstWord(text)) != "a")
      throw OOMon::notice_parse_error("spambot notice");

    if (DownCase(FirstWord(text)) != "possible")
      throw OOMon::notice_parse_error("spambot notice");

    if (DownCase(FirstWord(text)) != "spambot")
      throw OOMon::notice_parse_error("spambot notice");

    Log::Write("*** " + notice);
    ::SendAll("*** " + notice, UF_OPER, WATCH_SPAMBOTS);
  }

  bool triggered(const int & count, const time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_SPAMBOT_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_SPAMBOT_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Spambot detected: " + this->nick + " (" +
      this->userhost + ")", UF_OPER, WATCH_SPAMBOTS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_SPAMBOT_ACTION]->getAction(),
      vars[VAR_SPAMBOT_ACTION]->getInt(),
      vars[VAR_SPAMBOT_REASON]->getString(), true);
  }

  bool expired(const time_t interval) const
  {
    return (interval > vars[VAR_SPAMBOT_MAX_TIME]->getInt());
  }

  void update(const SpambotNoticeEntry & data)
  {
    SimpleNoticeEntry::update(data);

    this->channel = data.channel;
  }

protected:
  std::string channel;
};

class TooManyConnNoticeEntry : public SimpleNoticeEntry
{
public:
  explicit TooManyConnNoticeEntry(const std::string & notice)
  {
    // Too many local connections for zqggt[zqggt@63.153.4.192]
    std::string text(notice);
  
    if (DownCase(FirstWord(text)) != "too")
      throw OOMon::notice_parse_error("toomany notice");

    if (DownCase(FirstWord(text)) != "many")
      throw OOMon::notice_parse_error("toomany notice");

    if (DownCase(FirstWord(text)) != "local")
      throw OOMon::notice_parse_error("toomany notice");

    if (DownCase(FirstWord(text)) != "connections")
      throw OOMon::notice_parse_error("toomany notice");

    if (DownCase(FirstWord(text)) != "for")
      throw OOMon::notice_parse_error("toomany notice");

    std::string nuh = FirstWord(text);

    this->nick = chopNick(nuh);
    this->userhost = chopUserhost(nuh);

    std::string::size_type at = this->userhost.find('@');

    if (std::string::npos == at)
      throw OOMon::notice_parse_error("toomany notice");

    this->user = userhost.substr(0, at);
    this->host = userhost.substr(at + 1);

    Log::Write("*** " + notice);
    ::SendAll("*** " + notice, UF_OPER, WATCH_TOOMANYS);
  }

  bool triggered(const int & count, const time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_TOOMANY_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_TOOMANY_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Too many connect attempts detected: " + this->nick + " (" +
      this->userhost + ")", UF_OPER, WATCH_TOOMANYS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_TOOMANY_ACTION]->getAction(),
      vars[VAR_TOOMANY_ACTION]->getInt(),
      vars[VAR_TOOMANY_REASON]->getString(), true);
  }

  bool expired(const time_t interval) const
  {
    return (interval > vars[VAR_TOOMANY_MAX_TIME]->getInt());
  }

  void update(const TooManyConnNoticeEntry & data)
  {
    SimpleNoticeEntry::update(data);

    this->user = data.user;
    this->host = data.host;
  }

  bool operator==(const TooManyConnNoticeEntry & rhs) const
  {
    if (vars[VAR_TOOMANY_IGNORE_USERNAME]->getBool())
    {
      return Same(this->host, rhs.host);
    }
    else
    {
      return SimpleNoticeEntry::operator==(rhs);
    }
  }

protected:
  std::string user;
  std::string host;
};


#endif /* __NOTICE_H__ */

