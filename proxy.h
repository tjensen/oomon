#ifndef __PROXY_H__
#define __PROXY_H__
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
#include "botsock.h"


class Proxy : public BotSock
{
private:
  typedef boost::shared_ptr<Proxy> ProxyPtr;

public:
  enum ProxyType { SOCKS4, SOCKS5, WINGATE, HTTP };

  Proxy(const std::string & hostname, const std::string & nick,
    const std::string & userhost);
  virtual ~Proxy(void);

  std::string address(void) const { return this->_address; };
  BotSock::Port port(void) const { return this->_port; };
  virtual ProxyType type(void) const = 0;

  static void check(const std::string &, const std::string &,
    const std::string &, const std::string &);
  static bool connect(ProxyPtr newProxy, const std::string & address,
    const BotSock::Port port);
  static void processAll(const fd_set & readset, const fd_set & writeset);
  static void setAllFD(fd_set & readset, fd_set & writeset);
  static bool isChecking(const std::string &, const BotSock::Port, ProxyType);
  static bool isVerifiedClean(const std::string & address,
    const BotSock::Port port, const ProxyType type);
  static void addToCache(const std::string & address, const BotSock::Port port,
    const ProxyType type);
  static bool skipCheck(const std::string & address, const BotSock::Port & port,
    const ProxyType & type)
  {
    return (Proxy::isChecking(address, port, type) ||
      Proxy::isVerifiedClean(address, port, type));
  }

  static void status(StrList & output);

protected:
  void detectedProxy(void);

  std::string hostname(void) const { return this->_hostname; };
  std::string nick(void) const { return this->_nick; };
  std::string userhost(void) const { return this->_userhost; };

  virtual std::string typeName(void) const = 0;

  bool _detectedProxy;

private:
  typedef std::list<ProxyPtr> ProxyList;
  static ProxyList items;

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
      const BotSock::Port port, const ProxyType type) : _address(address),
      _port(port), _type(type) { }
    bool operator()(ProxyPtr proxy);
  private:
    const std::string & _address;
    const BotSock::Port & _port;
    const ProxyType _type;
  };

  class CacheEntry
  {
  public:
    CacheEntry(void) : _ip(INADDR_NONE) { };
    CacheEntry(const BotSock::Address & ip, const BotSock::Port & port,
      const ProxyType & type)
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
      if (!this->isEmpty() && ((now - _checked) > CACHE_EXPIRE))
      {
	return true;
      }
      return false;
    }

    void clear(void) { _ip = INADDR_NONE; };

  private:
    BotSock::Address _ip;
    BotSock::Port _port;
    ProxyType _type;
    time_t _checked;
  };
  typedef std::vector<CacheEntry> Cache;
  static Cache safeHosts;
  static const time_t CACHE_EXPIRE;
  static const ProxyList::size_type CACHE_SIZE;

  std::string _address, _hostname, _nick, _userhost;
  BotSock::Port _port;

  bool connect(const std::string & address, const BotSock::Port port);
};

#endif /* __PROXY_H__ */

