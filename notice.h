#ifndef __NOTICE_H__
#define __NOTICE_H__
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

// ===========================================================================
// File Description:
//
//  This file contains the generic server notice handler classes.  The
//  idea here is that OOMon watches several types of server notice types
//  to detect flooding, spamming, etc.  When enough notices are detected
//  for a particular user, some action is performed.
//
//  The NoticeList template class acts as a container for some class
//  derived from SimpleNoticeEntry.  Each derived class should query its
//  associated settings (in vars.cc) to determine when an action should
//  be taken.
//
//  The public interface to NoticeList includes three member functions:
//   clear()
//     Empty the container.
//   size()
//     Return the number of entries in the container.
//   onNotice(string)
//     Parse a server notice.
// ===========================================================================

// Std C++ Headers
#include <string>
#include <list>
#include <algorithm>
#include <ctime>

// Boost C++ Headers
#include <boost/bind.hpp>

// OOMon Headers
#include "strtype"
#include "vars.h"
#include "util.h"
#include "main.h"
#include "botsock.h"
#include "botexcept.h"


template<typename T>
class NoticeList
{
private:
  class NoticeEntry
  {
  public:
    NoticeEntry(const T & special, const int & count, const std::time_t & last)
    : special_(special), count_(count), last_(last) { }

    bool operator==(const NoticeEntry & rhs) const
    {
      return (this->special_ == rhs.special_);
    }

    void update(const std::time_t & now, const T & data)
    {
      this->special_.update(data);

      ++(this->count_);

      if (this->special_.triggered(this->count_, now - this->last_))
      {
	this->special_.execute();
	this->count_ = 0;
      }

      this->last_ = now;
    }

    bool expired(const std::time_t now) const
    {
      return this->special_.expired(now - this->last_);
    }

    T getSpecial(void) const { return this->special_; }
    int getCount(void) const { return this->count_; }
    std::time_t getLast(void) const { return this->last_; }

  private:
    T		special_;
    int		count_;
    std::time_t	last_;
  };

  std::list<NoticeEntry> list;

public:
  void clear(void)
  {
    this->list.clear();
  }

  int size(void) const { return this->list.size(); };

  bool onNotice(const std::string & notice)
  {
    std::time_t now = std::time(NULL);
    bool result = false;

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

      result = true;
    }
    catch (OOMon::notice_parse_error)
    {
      // Ignore the notice for now
    }
    return result;
  }

private:
  void expire(const std::time_t now)
  {
    this->list.remove_if(boost::bind(&NoticeEntry::expired, _1, now));
  }
};


class SimpleNoticeEntry
{
public:
  // Derived classes' constructors should take a string parameter
  // containing a server notice and parse the notice.
  virtual ~SimpleNoticeEntry(void) { }

  // Derived classes should implement triggered to determine if the
  // parameters, count and interval, indicate an action should be
  // taken.  They may also want to call this function to determine
  // if the user should be excluded from actions.
  bool triggered(const int & count, const std::time_t & interval) const
  {
    // Return true if the user is not an Oper and is not E: lined.
    return (!Config::IsOKHost(this->userhost) &&
      !Config::IsOper(this->userhost));
  }

  // Derived classes should implement execute to perform the desired
  // action (kill, kline, dline, etc.)
  virtual void execute(void) const = 0;

  // Derived classes should implement expired to return true if
  // the interval parameter indicates the entry has expired.
  virtual bool expired(const std::time_t interval) const = 0;

  // Derived classes may want to implement update to refresh their
  // private data.
  void update(const SimpleNoticeEntry & data)
  {
    this->nick = data.nick;
  }

  // Derived classes may want to overload the == operator to compare
  // additional private data.
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
    ::SendAll("*** " + notice, UserFlags::OPER, WATCH_FLOODERS);
  }

  bool triggered(const int & count, const std::time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_FLOODER_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_FLOODER_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Flooder detected: " + this->nick + " (" +
      this->userhost + ")", UserFlags::OPER, WATCH_FLOODERS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_FLOODER_ACTION]->getAction(),
      vars[VAR_FLOODER_ACTION]->getInt(),
      vars[VAR_FLOODER_REASON]->getString(), true);
  }

  bool expired(const std::time_t interval) const
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
    ::SendAll("*** " + notice, UserFlags::OPER, WATCH_SPAMBOTS);
  }

  bool triggered(const int & count, const std::time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_SPAMBOT_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_SPAMBOT_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Spambot detected: " + this->nick + " (" +
      this->userhost + ")", UserFlags::OPER, WATCH_SPAMBOTS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_SPAMBOT_ACTION]->getAction(),
      vars[VAR_SPAMBOT_ACTION]->getInt(),
      vars[VAR_SPAMBOT_REASON]->getString(), true);
  }

  bool expired(const std::time_t interval) const
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
    ::SendAll("*** " + notice, UserFlags::OPER, WATCH_TOOMANYS);
  }

  bool triggered(const int & count, const std::time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_TOOMANY_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_TOOMANY_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Too many connect attempts detected: " + this->nick + " (" +
      this->userhost + ")", UserFlags::OPER, WATCH_TOOMANYS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_TOOMANY_ACTION]->getAction(),
      vars[VAR_TOOMANY_ACTION]->getInt(),
      vars[VAR_TOOMANY_REASON]->getString(), true);
  }

  bool expired(const std::time_t interval) const
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


class ConnectEntry : public SimpleNoticeEntry
{
public:
  explicit ConnectEntry(const std::string & notice)
  {
    // nick user@host ip
    std::string text(notice);

    this->nick = FirstWord(text);
    this->userhost = FirstWord(text);

    std::string::size_type at = this->userhost.find('@');
    if (std::string::npos == at)
    {
      this->host = this->userhost;
    }
    else
    {
      this->host = this->userhost.substr(at + 1);
    }

    this->ip = BotSock::inet_addr(FirstWord(text));
  }

  bool triggered(const int & count, const std::time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_CONNECT_FLOOD_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_CONNECT_FLOOD_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Connect flooder detected: " + this->nick + " (" +
      this->userhost + ")", UserFlags::OPER, WATCH_CONNFLOOD);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_CONNECT_FLOOD_ACTION]->getAction(),
      vars[VAR_CONNECT_FLOOD_ACTION]->getInt(),
      vars[VAR_CONNECT_FLOOD_REASON]->getString(), true);
  }

  bool expired(const std::time_t interval) const
  {
    return (interval > vars[VAR_CONNECT_FLOOD_MAX_TIME]->getInt());
  }

  void update(const ConnectEntry & data)
  {
    SimpleNoticeEntry::update(data);

    this->host = data.host;
    this->ip = data.ip;
  }

  bool operator==(const ConnectEntry & rhs) const
  {
    if ((INADDR_NONE != this->ip) && (INADDR_NONE != rhs.ip))
    {
      return (this->ip == rhs.ip);
    }
    else
    {
      return Same(this->host, rhs.host);
    }
  }

protected:
  std::string host;
  BotSock::Address ip;
};


class OperFailNoticeEntry : public SimpleNoticeEntry
{
public:
  explicit OperFailNoticeEntry(const std::string & notice)
  {
    // Failed OPER attempt - host mismatch by ToastTEST (toast@cs6669210-179.austin.rr.com)
    std::string copy = notice;

    if (server.downCase(FirstWord(copy)) != "failed")
      throw OOMon::notice_parse_error("operfail notice");

    if (server.downCase(FirstWord(copy)) != "oper")
      throw OOMon::notice_parse_error("operfail notice");

    if (server.downCase(FirstWord(copy)) != "attempt")
      throw OOMon::notice_parse_error("operfail notice");

    if (server.downCase(FirstWord(copy)) != "-")
      throw OOMon::notice_parse_error("operfail notice");

    if (server.downCase(FirstWord(copy)) != "host")
      throw OOMon::notice_parse_error("operfail notice");

    if (server.downCase(FirstWord(copy)) != "mismatch")
      throw OOMon::notice_parse_error("operfail notice");

    if (server.downCase(FirstWord(copy)) != "by")
      throw OOMon::notice_parse_error("operfail notice");

    this->nick = FirstWord(copy);

    this->userhost = FirstWord(copy);
    // Remove the parentheses surrounding the user@host
    if (this->userhost.length() >= 2)
    {
      this->userhost = this->userhost.substr(1, this->userhost.length() - 2);
    }

    Log::Write("*** " + notice);
    ::SendAll("*** " + notice, UserFlags::OPER, WATCH_OPERFAILS);
  }

  bool triggered(const int & count, const std::time_t & interval) const
  {
    return (SimpleNoticeEntry::triggered(count, interval) &&
      (count > vars[VAR_OPERFAIL_MAX_COUNT]->getInt()) &&
      (interval <= vars[VAR_OPERFAIL_MAX_TIME]->getInt()));
  }

  virtual void execute(void) const
  {
    ::SendAll("*** Too many failed oper attempts detected: " + this->nick +
      " (" + this->userhost + ")", UserFlags::OPER, WATCH_OPERFAILS);

    doAction(this->nick, this->userhost,
      ::users.getIP(this->nick, this->userhost),
      vars[VAR_OPERFAIL_ACTION]->getAction(),
      vars[VAR_OPERFAIL_ACTION]->getInt(),
      vars[VAR_OPERFAIL_REASON]->getString(), true);
  }

  bool expired(const std::time_t interval) const
  {
    return (interval > vars[VAR_OPERFAIL_MAX_TIME]->getInt());
  }

  void update(const OperFailNoticeEntry & data)
  {
    SimpleNoticeEntry::update(data);
  }
};


#endif /* __NOTICE_H__ */

