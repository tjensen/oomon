#ifndef __PROXYLIST_H__
#define __PROXYLIST_H__
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

// Std C++ headers
#include <string>
#include <list>
#include <functional>

// Boost C++ headers
#include <boost/shared_ptr.hpp>

// Std C headers
#include <sys/types.h>
#include <unistd.h>

// OOMon headers
#include "strtype"
#include "oomon.h"
#include "proxy.h"
#include "botsock.h"


class ProxyList
{
private:
  typedef boost::shared_ptr<Proxy> ProxyPtr;

public:
  ProxyList(void) : safeHosts(ProxyList::CACHE_SIZE) { }
  virtual ~ProxyList(void) { }

  void check(const std::string &, const std::string &,
    const std::string &, const std::string &);
  bool connect(ProxyPtr newProxy, const std::string & address,
    const BotSock::Port port);
  void processAll(const fd_set & readset, const fd_set & writeset);
  void setAllFD(fd_set & readset, fd_set & writeset);
  bool isChecking(const std::string &, const BotSock::Port,
    Proxy::Protocol) const;
  bool isVerifiedClean(const std::string & address,
    const BotSock::Port port, const Proxy::Protocol type) const;
  void addToCache(const std::string & address, const BotSock::Port port,
    const Proxy::Protocol type);
  bool skipCheck(const std::string & address, const BotSock::Port & port,
    const Proxy::Protocol & type) const
  {
    return (this->isChecking(address, port, type) ||
      this->isVerifiedClean(address, port, type));
  }

  void status(StrList & output) const;

  static const time_t CACHE_EXPIRE;

private:
  typedef std::list<ProxyPtr> SockList;
  SockList scanners;

  class ProxyProcessor : public std::unary_function<ProxyPtr, bool>
  {
  public:
    explicit ProxyProcessor(const fd_set & readset, const fd_set & writeset)
      : _readset(readset), _writeset(writeset) { }
    bool operator()(ProxyPtr proxy);
  private:
    const fd_set & _readset;
    const fd_set & _writeset;
  };

  class ProxyIsChecking : public std::unary_function<ProxyPtr, bool>
  {
  public:
    explicit ProxyIsChecking(const std::string & address,
      const BotSock::Port port, const Proxy::Protocol type) : _address(address),
      _port(port), _type(type) { }
    bool operator()(ProxyPtr proxy);
  private:
    const std::string & _address;
    const BotSock::Port & _port;
    const Proxy::Protocol _type;
  };

  class CacheEntry
  {
  public:
    CacheEntry(void) : _ip(INADDR_NONE) { };
    CacheEntry(const BotSock::Address & ip, const BotSock::Port & port,
      const Proxy::Protocol & type)
      : _ip(ip), _port(port), _type(type), _checked(time(NULL)) { };

    bool isEmpty(void) const { return (INADDR_NONE == _ip); };
    bool operator==(const CacheEntry & rhs) const
    {
      if (rhs.isEmpty())
      {
	return this->isEmpty();
      }
      else
      {
        return ((_ip == rhs._ip) && (_port == rhs._port) &&
	  (_type == rhs._type));
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
	return (_checked < rhs._checked);
      }
    }
    bool isExpired(const time_t now) const
    {
      if (!this->isEmpty() && ((now - _checked) > ProxyList::CACHE_EXPIRE))
      {
	return true;
      }
      return false;
    }

    void clear(void) { _ip = INADDR_NONE; };

  private:
    BotSock::Address _ip;
    BotSock::Port _port;
    Proxy::Protocol _type;
    time_t _checked;
  };

  typedef std::vector<CacheEntry> Cache;
  Cache safeHosts;
  static const SockList::size_type CACHE_SIZE;
};


extern ProxyList proxies;


#endif /* __PROXYLIST_H__ */

