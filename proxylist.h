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
#include <set>
#include <vector>
#include <deque>

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
  static void init(void);

  ProxyList(void);
  virtual ~ProxyList(void) { }

  void check(const UserEntryPtr user);
  bool connect(ProxyPtr newProxy, const BotSock::Port port);

  void preSelect(fd_set & readset, fd_set & writeset);
  void postSelect(const fd_set & readset, const fd_set & writeset);

  bool isChecking(const BotSock::Address &, const BotSock::Port,
      Proxy::Protocol) const;
  bool isVerifiedClean(const BotSock::Address & address,
      const BotSock::Port port, const Proxy::Protocol type);
  void addToCache(const BotSock::Address & address, const BotSock::Port port,
      const Proxy::Protocol type);
  bool skipCheck(const BotSock::Address & address, const BotSock::Port & port,
      const Proxy::Protocol & type);

  void status(class BotClient * client) const;

  static int cacheExpire;

private:
  bool initiateCheck(const UserEntryPtr user, const BotSock::Port port,
      const Proxy::Protocol protocol);

  typedef std::set<BotSock::Port> PortSet;

  void enqueueScans(const UserEntryPtr user, const ProxyList::PortSet & ports,
      const Proxy::Protocol protocol);

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
    CacheEntry(void) : ip_(INADDR_NONE) { }
    CacheEntry(const BotSock::Address & ip, const BotSock::Port & port,
        const Proxy::Protocol & type)
      : ip_(ip), port_(port), type_(type), checked_(std::time(NULL)) { }

    bool isEmpty(void) const { return (INADDR_NONE == ip_); }
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
      if (!this->isEmpty() && ((now - checked_) > ProxyList::cacheExpire))
      {
	return true;
      }
      return false;
    }

    void update(void)
    {
      this->checked_ = std::time(NULL);
    }

    void clear(void) { ip_ = INADDR_NONE; }

  private:
    BotSock::Address ip_;
    BotSock::Port port_;
    Proxy::Protocol type_;
    std::time_t checked_;
  };

  struct QueueNode
  {
    QueueNode(const UserEntryPtr user_, const BotSock::Port port_,
        const Proxy::Protocol protocol_)
      : user(user_), port(port_), protocol(protocol_) { }
    const UserEntryPtr user;
    const BotSock::Port port;
    const Proxy::Protocol protocol;
  };

  class ProxyIsQueued : public std::unary_function<QueueNode, bool>
  {
  public:
    explicit ProxyIsQueued(const BotSock::Address & address,
        const BotSock::Port port, const Proxy::Protocol protocol)
      : address_(address), port_(port), protocol_(protocol) { }
    bool operator()(const QueueNode & proxy) const;
  private:
    const BotSock::Address address_;
    const BotSock::Port port_;
    const Proxy::Protocol protocol_;
  };

  typedef std::vector<CacheEntry> Cache;
  typedef std::deque<QueueNode> Queue;
  Cache safeHosts;
  Queue queuedScans;
  mutable unsigned long cacheHits, cacheMisses;
  static const SockList::size_type CACHE_SIZE;

  static std::string getPorts(const ProxyList::PortSet * ports);
  static std::string setPorts(ProxyList::PortSet * ports,
      const std::string & newValue);

  static std::string getCacheSize(void);
  static std::string setCacheSize(const std::string & newValue);

  static bool enable;
  static bool cacheEnable;
  static int maxCount;
  static ProxyList::PortSet httpConnectPorts;
  static ProxyList::PortSet httpPostPorts;
  static ProxyList::PortSet socks4Ports;
  static ProxyList::PortSet socks5Ports;
  static ProxyList::PortSet wingatePorts;
  static ProxyList::Cache::size_type cacheSize;
};


extern ProxyList proxies;


#endif /* __PROXYLIST_H__ */

