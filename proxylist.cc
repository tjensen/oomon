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
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <ctime>

// Boost C++ Headers
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

// Std C headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>

// OOMon headers
#include "oomon.h"
#include "proxylist.h"
#include "botsock.h"
#include "socks4.h"
#include "socks5.h"
#include "wingate.h"
#include "botexcept.h"
#include "botclient.h"
#include "http.h"
#include "httppost.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "irc.h"
#include "vars.h"
#include "defaults.h"


#ifdef DEBUG
# define PROXYLIST_DEBUG
#endif


bool ProxyList::enable(DEFAULT_SCAN_FOR_PROXIES);
bool ProxyList::cacheEnable(DEFAULT_SCAN_CACHE);
int ProxyList::cacheExpire(DEFAULT_SCAN_CACHE_EXPIRE);
int ProxyList::maxCount(DEFAULT_SCAN_MAX_COUNT);
ProxyList::PortList ProxyList::httpConnectPorts;
ProxyList::PortList ProxyList::httpPostPorts;
ProxyList::PortList ProxyList::socks4Ports;
ProxyList::PortList ProxyList::socks5Ports;
ProxyList::PortList ProxyList::wingatePorts;
ProxyList::Cache::size_type ProxyList::cacheSize(DEFAULT_SCAN_CACHE_SIZE);


ProxyList proxies;


ProxyList::ProxyList(void) : safeHosts(ProxyList::cacheSize), cacheHits(0),
  cacheMisses(0)
{
}


// connect(address, port)
//
// Attempts to connect to a proxy
//
// Returns true if no errors occurred
//
bool
ProxyList::connect(ProxyPtr newProxy, const BotSock::Port port)
{
#ifdef PROXYLIST_DEBUG
  std::cout << "Creating proxy connection to address " <<
    newProxy->address() << ":" << port << std::endl;
#endif

  newProxy->bindTo(config.proxyVhost());
  newProxy->setTimeout(PROXY_IDLE_MAX);

  if (!newProxy->connect(port))
  {
#ifdef PROXYLIST_DEBUG
    std::cout << "Connect to proxy failed (other)" << std::endl;
#endif
    return false;
  }

  this->scanners.push_back(newProxy);

  return true;
}


bool
ProxyList::initiateCheck(const UserEntryPtr user, const BotSock::Port port,
    Proxy::Protocol protocol)
{
  bool result = false;

  try
  {
    ProxyPtr newProxy;

    switch (protocol)
    {
      case Proxy::HTTP_CONNECT:
        newProxy.reset(new Http(user));
        break;

      case Proxy::HTTP_POST:
        newProxy.reset(new HttpPost(user));
        break;

      case Proxy::WINGATE:
        newProxy.reset(new WinGate(user));
        break;

      case Proxy::SOCKS4:
        newProxy.reset(new Socks4(user));
        break;

      case Proxy::SOCKS5:
        newProxy.reset(new Socks5(user));
        break;

      default:
        std::cerr << "Unknown proxy type?" << std::endl;
        break;
    }

    if (newProxy)
    {
      result = true;
      this->connect(newProxy, port);
    }
  }
  catch (OOMon::errno_error & e)
  {
    std::cerr << "Connect to proxy failed (" << e.why() << ")" << std::endl;
  }

  return result;
}


void
ProxyList::enqueueScans(const UserEntryPtr user,
    const ProxyList::PortList & ports, const Proxy::Protocol protocol)
{
  for (ProxyList::PortList::const_iterator pos = ports.begin();
      pos != ports.end(); ++pos)
  {
    if (!this->skipCheck(user->getIP(), *pos, protocol))
    {
      ProxyList::QueueNode enqueue(user, *pos, protocol);

      this->queuedScans.push_back(enqueue);
    }
  }
}


// check(address, hostname, nick, userhost)
//
// Checks a client for proxies
//
// Returns true if no errors occurred
//
void
ProxyList::check(const UserEntryPtr user)
{
  if (ProxyList::enable)
  {
    this->enqueueScans(user, ProxyList::httpConnectPorts, Proxy::HTTP_CONNECT);
    this->enqueueScans(user, ProxyList::httpPostPorts, Proxy::HTTP_POST);
    this->enqueueScans(user, ProxyList::socks4Ports, Proxy::SOCKS4);
    this->enqueueScans(user, ProxyList::socks5Ports, Proxy::SOCKS5);
    this->enqueueScans(user, ProxyList::wingatePorts, Proxy::WINGATE);
  }
}


// SetAllFD(readset, writeset)
//
// Do this if you want to be able to receive data :P
//
void
ProxyList::setAllFD(fd_set & readset, fd_set & writeset)
{
  std::for_each(this->scanners.begin(), this->scanners.end(),
    BotSock::FDSetter(readset, writeset));
}


bool
ProxyList::ProxyProcessor::operator()(ProxyPtr proxy)
{
  bool remove;

  try
  {
    remove = !proxy->process(this->readset_, this->writeset_);
  }
  catch (OOMon::timeout_error)
  {
#ifdef PROXYLIST_DEBUG
    std::cout << "Proxy connection to " << proxy->address() <<
      " timed out." << std::endl;
#endif
    remove = true;
  }

  return remove;
}


// ProcessAll(readset, writeset)
//
// Processes any received data at each proxy connection.
//
void
ProxyList::processAll(const fd_set & readset, const fd_set & writeset)
{
  // Expire any old proxy cache entries
  std::time_t now = std::time(NULL);
  ProxyList::CacheEntry empty;
  std::replace_if(this->safeHosts.begin(), this->safeHosts.end(),
    boost::bind(&ProxyList::CacheEntry::isExpired, _1, now), empty);

  // Process queue
  while (!this->queuedScans.empty() &&
      (static_cast<int>(this->scanners.size()) < ProxyList::maxCount))
  {
    ProxyList::QueueNode front(this->queuedScans.front());

    if (this->initiateCheck(front.user, front.port, front.protocol))
    {
      this->queuedScans.pop_front();
    }
    else
    {
      break;
    }
  }

  // Process active scans
  ProxyList::ProxyProcessor p(readset, writeset);
  this->scanners.remove_if(p);

  // Remove any expired scans
  this->scanners.remove_if(boost::bind(&Proxy::expired, _1));
}


bool
ProxyList::ProxyIsChecking::operator()(ProxyPtr proxy)
{
  return ((proxy->address() == this->address_) &&
    (proxy->port() == this->port_) && (proxy->type() == this->type_));
}


bool
ProxyList::ProxyIsQueued::operator()(const QueueNode & node) const
{
  return ((node.user->getIP() == this->address_) &&
    (node.port == this->port_) && (node.protocol == this->protocol_));
}


bool
ProxyList::isChecking(const BotSock::Address & address,
    const BotSock::Port port, const Proxy::Protocol protocol) const
{
  return ((this->scanners.end() != std::find_if(this->scanners.begin(),
    this->scanners.end(), ProxyList::ProxyIsChecking(address, port,
      protocol))) || (this->queuedScans.end() !=
    std::find_if(this->queuedScans.begin(), this->queuedScans.end(),
      ProxyList::ProxyIsQueued(address, port, protocol))));
}


void
ProxyList::addToCache(const BotSock::Address & address,
    const BotSock::Port port, const Proxy::Protocol type)
{
  if (ProxyList::cacheEnable)
  {
    ProxyList::CacheEntry newEntry(address, port, type);
    ProxyList::CacheEntry empty;

    ProxyList::Cache::iterator findEmpty = std::find(this->safeHosts.begin(),
      this->safeHosts.end(), empty);
    if (findEmpty != this->safeHosts.end())
    {
      // Found an empty slot!  Now use it!
      *findEmpty = newEntry;
    }
    else if (!this->safeHosts.empty())
    {
      // No empty slot found!  Find the oldest entry and replace it.
      std::sort(this->safeHosts.begin(), this->safeHosts.end());

      this->safeHosts[0] = newEntry;
    }
  }
}


bool
ProxyList::skipCheck(const BotSock::Address & address,
    const BotSock::Port & port, const Proxy::Protocol & type)
{
  return (this->isChecking(address, port, type) ||
      this->isVerifiedClean(address, port, type));
}


bool
ProxyList::isVerifiedClean(const BotSock::Address & address,
    const BotSock::Port port, const Proxy::Protocol type)
{
  bool result(false);

  if (ProxyList::cacheEnable)
  {
    const ProxyList::CacheEntry tmp(address, port, type);

    ProxyList::Cache::iterator pos = std::find(this->safeHosts.begin(),
      this->safeHosts.end(), tmp);

    if (this->safeHosts.end() == pos)
    {
      result = false;
      ++this->cacheMisses;
    }
    else
    {
      result = true;
      ++this->cacheHits;
      pos->update();

#ifdef PROXYLIST_DEBUG
      if (result)
      {
        std::cout << "Verified clean: " << address << ":" << port << " (" <<
          type << ")" << std::endl;
      }
#endif
    }
  }

  return result;
}


void
ProxyList::status(BotClient * client) const
{
  ProxyList::SockList::size_type size(this->scanners.size());
  ProxyList::Queue::size_type queueSize(this->queuedScans.size());

  if ((size > 0) || ProxyList::enable)
  {
    std::string scanCount("Proxy scanners: ");
    scanCount += boost::lexical_cast<std::string>(size);
    scanCount += " (";
    scanCount += boost::lexical_cast<std::string>(queueSize);
    scanCount += " queued)";
    client->send(scanCount);

    int cacheCount = 0;
    for (ProxyList::Cache::const_iterator pos = this->safeHosts.begin();
      pos < this->safeHosts.end(); ++pos)
    {
      if (!pos->isEmpty())
      {
        ++cacheCount;
      }
    }
    if (ProxyList::cacheEnable)
    {
      std::string cache("Proxy cache: ");
      cache += boost::lexical_cast<std::string>(cacheCount);
      cache += '/';
      cache += boost::lexical_cast<std::string>(this->safeHosts.size());
      cache += " (";
      cache += boost::lexical_cast<std::string>(this->cacheHits);
      cache += " hits, ";
      cache += boost::lexical_cast<std::string>(this->cacheMisses);
      cache += " misses)";
      client->send(cache);
    }
  }
}


std::string
ProxyList::getPorts(const ProxyList::PortList * ports)
{
  std::string result;

  for (ProxyList::PortList::const_iterator pos = ports->begin();
      pos != ports->end(); ++pos)
  {
    if (!result.empty())
    {
      result += ", ";
    }

    result += boost::lexical_cast<std::string>(*pos);
  }

  return result;
}


std::string
ProxyList::setPorts(ProxyList::PortList * ports, const std::string & newValue)
{
  std::string result;
  ProxyList::PortList newPorts;

  StrVector values;
  StrSplit(values, newValue, " ,", true);
  for (StrVector::const_iterator pos = values.begin(); pos != values.end();
      ++pos)
  {
    try
    {
      newPorts.push_back(boost::lexical_cast<BotSock::Port>(*pos));
    }
    catch (boost::bad_lexical_cast)
    {
      result = "*** Bad port number: ";
      result += *pos;
      break;
    }
  }

  if (result.empty())
  {
    ports->swap(newPorts);
  }

  return result;
}


std::string
ProxyList::getCacheSize(void)
{
  return boost::lexical_cast<std::string>(proxies.safeHosts.size());
}


std::string
ProxyList::setCacheSize(const std::string & newValue)
{
  std::string result;

  try
  {
    Cache::size_type newSize = boost::lexical_cast<Cache::size_type>(newValue);

    if (newSize > 0)
    {
      ProxyList::cacheSize = newSize;
      proxies.safeHosts.resize(ProxyList::cacheSize);
    }
    else
    {
      result = "*** Cache size must be 1 or higher!";
    }
  }
  catch (boost::bad_lexical_cast)
  {
    result = "*** Numeric value expected!";
  }

  return result;
}


void
ProxyList::init(void)
{
  Proxy::init();

  ProxyList::setPorts(&ProxyList::httpConnectPorts,
      DEFAULT_SCAN_HTTP_CONNECT_PORTS);
  ProxyList::setPorts(&ProxyList::httpPostPorts,
      DEFAULT_SCAN_HTTP_POST_PORTS);
  ProxyList::setPorts(&ProxyList::socks4Ports,
      DEFAULT_SCAN_SOCKS4_PORTS);
  ProxyList::setPorts(&ProxyList::socks5Ports,
      DEFAULT_SCAN_SOCKS5_PORTS);
  ProxyList::setPorts(&ProxyList::wingatePorts,
      DEFAULT_SCAN_WINGATE_PORTS);

  vars.insert("SCAN_CACHE", Setting::BooleanSetting(ProxyList::cacheEnable));
  vars.insert("SCAN_CACHE_EXPIRE",
      Setting::IntegerSetting(ProxyList::cacheExpire, 0));
  vars.insert("SCAN_CACHE_SIZE", Setting(boost::bind(ProxyList::getCacheSize),
        boost::bind(ProxyList::setCacheSize, _1)));
  vars.insert("SCAN_FOR_PROXIES", Setting::BooleanSetting(ProxyList::enable));
  vars.insert("SCAN_HTTP_CONNECT_PORTS",
      Setting(boost::bind(ProxyList::getPorts, &ProxyList::httpConnectPorts),
        boost::bind(ProxyList::setPorts, &ProxyList::httpConnectPorts, _1)));
  vars.insert("SCAN_HTTP_POST_PORTS",
      Setting(boost::bind(ProxyList::getPorts, &ProxyList::httpPostPorts),
        boost::bind(ProxyList::setPorts, &ProxyList::httpPostPorts, _1)));
  vars.insert("SCAN_MAX_COUNT", Setting::IntegerSetting(ProxyList::maxCount,
        1));
  vars.insert("SCAN_SOCKS4_PORTS",
      Setting(boost::bind(ProxyList::getPorts, &ProxyList::socks4Ports),
        boost::bind(ProxyList::setPorts, &ProxyList::socks4Ports, _1)));
  vars.insert("SCAN_SOCKS5_PORTS",
      Setting(boost::bind(ProxyList::getPorts, &ProxyList::socks5Ports),
        boost::bind(ProxyList::setPorts, &ProxyList::socks5Ports, _1)));
  vars.insert("SCAN_WINGATE_PORTS",
      Setting(boost::bind(ProxyList::getPorts, &ProxyList::wingatePorts),
        boost::bind(ProxyList::setPorts, &ProxyList::wingatePorts, _1)));
}

