#ifndef __PROXYLIST_H__
#define __PROXYLIST_H__
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

// Std C++ headers
#include <string>
#include <list>

// Boost C++ headers
#include <boost/shared_ptr.hpp>

// Std C headers
#include <sys/types.h>
#include <unistd.h>

// OOMon headers
#include "oomon.h"
#include "proxy.h"
#include "botsock.h"
#include "userentry.h"


class ProxyList
{
private:
  typedef boost::shared_ptr<Proxy> ProxyPtr;

public:
  ProxyList(void);
  virtual ~ProxyList(void) { }

  void check(const BotSock::Address & address, const UserEntryPtr user);
  bool connect(ProxyPtr newProxy, const BotSock::Address & address,
      const BotSock::Port port);
  void processAll(const fd_set & readset, const fd_set & writeset);
  void setAllFD(fd_set & readset, fd_set & writeset);
  bool isChecking(const BotSock::Address &, const BotSock::Port,
      Proxy::Protocol) const;
  bool isVerifiedClean(const BotSock::Address & address,
      const BotSock::Port port, const Proxy::Protocol type);
  void addToCache(const BotSock::Address & address, const BotSock::Port port,
      const Proxy::Protocol type);
  bool skipCheck(const BotSock::Address & address, const BotSock::Port & port,
      const Proxy::Protocol & type)
  {
    return (this->isChecking(address, port, type) ||
        this->isVerifiedClean(address, port, type));
  }

  void status(class BotClient * client) const;

  static const std::time_t CACHE_EXPIRE;

private:
  void initiateCheck(const Proxy::Protocol type, const std::string & port,
    const BotSock::Address & address, const UserEntryPtr user);

  typedef std::list<ProxyPtr> SockList;
  SockList scanners;

  class ProxyProcessor : public std::unary_function<ProxyPtr, bool>
  {
  public:
    explicit ProxyProcessor(const fd_set & readset, const fd_set & writeset)
      : readset_(readset), writeset_(writeset) { }
    bool operator()(ProxyPtr proxy);
  private:
    const fd_set & readset_;
    const fd_set & writeset_;
  };

  class ProxyIsChecking : public std::unary_function<ProxyPtr, bool>
  {
  public:
    explicit ProxyIsChecking(const BotSock::Address & address,
        const BotSock::Port port, const Proxy::Protocol type)
      : address_(address), port_(port), type_(type) { }
    bool operator()(ProxyPtr proxy);
  private:
    const BotSock::Address & address_;
    const BotSock::Port & port_;
    const Proxy::Protocol type_;
  };

  class CacheEntry
  {
  public:
    CacheEntry(void) : ip_(INADDR_NONE) { };
    CacheEntry(const BotSock::Address & ip, const BotSock::Port & port,
        const Proxy::Protocol & type)
      : ip_(ip), port_(port), type_(type), checked_(std::time(NULL)) { }

    bool isEmpty(void) const { return (INADDR_NONE == ip_); };
    bool operator==(const CacheEntry & rhs) const
    {
      if (rhs.isEmpty())
      {
	return this->isEmpty();
      }
      else
      {
        return ((ip_ == rhs.ip_) && (port_ == rhs.port_) &&
	  (type_ == rhs.type_));
      }
    }
    bool operator<(const CacheEntry & rhs) const
    {
      if (this->isEmpty())
      {
	return true;
      }
      else
      {
	return (checked_ < rhs.checked_);
      }
    }
    bool isExpired(const std::time_t now) const
    {
      if (!this->isEmpty() && ((now - checked_) > ProxyList::CACHE_EXPIRE))
      {
	return true;
      }
      return false;
    }

    void update(void)
    {
      this->checked_ = std::time(NULL);
    }

    void clear(void) { ip_ = INADDR_NONE; };

  private:
    BotSock::Address ip_;
    BotSock::Port port_;
    Proxy::Protocol type_;
    std::time_t checked_;
  };

  typedef std::vector<CacheEntry> Cache;
  Cache safeHosts;
  mutable unsigned long cacheHits, cacheMisses;
  static const SockList::size_type CACHE_SIZE;
};


extern ProxyList proxies;


#endif /* __PROXYLIST_H__ */

