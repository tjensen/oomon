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
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <functional>

// Std C headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pwd.h>

// OOMon headers
#include "oomon.h"
#include "proxy.h"
#include "socks4.h"
#include "socks5.h"
#include "wingate.h"
#include "botexcept.h"
#include "http.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "irc.h"
#include "vars.h"


#ifdef DEBUG
# define PROXY_DEBUG
#endif

Proxy::ProxyList Proxy::items;

const time_t Proxy::CACHE_EXPIRE = (60 * 60);	/* 1 hour */
const Proxy::ProxyList::size_type Proxy::CACHE_SIZE = 4000;

Proxy::Cache Proxy::safeHosts(Proxy::CACHE_SIZE);


Proxy::Proxy(const std::string & hostname, const std::string & nick,
  const std::string & userhost)
  : BotSock(), _hostname(hostname), _nick(nick), _userhost(userhost),
  _detectedProxy(false)
{
}


Proxy::~Proxy(void)
{
#ifdef PROXY_DEBUG
  std::cout << "Proxy checker ending..." << std::endl;
#endif
}


void
Proxy::detectedProxy(void)
{
  // We DEFINITELY do not want to cache this one!
  _detectedProxy = true;

  std::string notice = std::string("Open ") + this->typeName() +
    " proxy detected for " + this->nick() + "!" + this->userhost() +
    " [" + this->address() + ":" + IntToStr(this->port()) + "]";
  ::SendAll(notice, UF_OPER);
  Log::Write(notice);

  doAction(this->nick(), this->userhost(),
    BotSock::inet_addr(this->address()),
    vars[VAR_SCAN_PROXY_ACTION]->getAction(),
    vars[VAR_SCAN_PROXY_ACTION]->getInt(),
    vars[VAR_SCAN_PROXY_REASON]->getString(), false);
}


bool
Proxy::connect(const std::string & address, const BotSock::Port port)
{
  this->_address = address;
  this->_port = port;

  return this->BotSock::connect(_address, _port);
}


// connect(address, port)
//
// Attempts to connect to a proxy
//
// Returns true if no errors occurred
//
bool
Proxy::connect(ProxyPtr newProxy, const std::string & address,
  const BotSock::Port port)
{
#ifdef PROXY_DEBUG
  std::cout << "Creating proxy connection to address " << address << ":" <<
    port << std::endl;
#endif

  newProxy->bindTo(Config::getProxyVhost());
  newProxy->setTimeout(PROXY_IDLE_MAX);

  if (!newProxy->connect(address, port))
  {
    std::cerr << "Connect to proxy failed (other)" << std::endl;
    return false;
  }

  Proxy::items.push_back(newProxy);

#ifdef PROXY_DEBUG
  std::cout << Proxy::items.size() << " proxies queued up for processing." <<
    std::endl;
#endif

  return true;
}


// check(address, hostname, nick, userhost)
//
// Checks a client for proxies
//
// Returns true if no errors occurred
//
void
Proxy::check(const std::string & address, const std::string & hostname,
  const std::string & nick, const std::string & userhost)
{
  try
  {
    if (!Proxy::skipCheck(address, 23, WINGATE))
    {
      ProxyPtr newProxy(new WinGate(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 23);
    }

    if (!Proxy::skipCheck(address, 1080, SOCKS4))
    {
      ProxyPtr newProxy(new Socks4(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 1080);
    }

    if (!Proxy::skipCheck(address, 1080, SOCKS5))
    {
      ProxyPtr newProxy(new Socks5(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 1080);
    }

    if (!Proxy::skipCheck(address, 80, HTTP))
    {
      ProxyPtr newProxy(new Http(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 80);
    }

    if (!Proxy::skipCheck(address, 1080, HTTP))
    {
      ProxyPtr newProxy(new Http(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 1080);
    }

    if (!Proxy::skipCheck(address, 3128, HTTP))
    {
      ProxyPtr newProxy(new Http(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 3128);
    }

    if (!Proxy::skipCheck(address, 8080, HTTP))
    {
      ProxyPtr newProxy(new Http(hostname, nick, userhost));
      Proxy::connect(newProxy, address, 8080);
    }
  }
  catch (OOMon::errno_error & e)
  {
    std::cerr << "Connect to proxy failed (" << e.why() << ")" << std::endl;
  }
}


// SetFD(readset, writeset)
//
// Do this if you want to be able to receive data :P
//
void
Proxy::setAllFD(fd_set & readset, fd_set & writeset)
{
  for (ProxyList::iterator pos = Proxy::items.begin();
    pos != Proxy::items.end(); ++pos)
  {
    (*pos)->setFD(readset, writeset);
  }
}


bool
Proxy::ProxyProcessor::operator()(ProxyPtr proxy)
{
  bool remove;

  try
  {
    remove = !proxy->process(this->_readset, this->_writeset);
  }
  catch (OOMon::timeout_error)
  {
#ifdef PROXY_DEBUG
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
Proxy::processAll(const fd_set & readset, const fd_set & writeset)
{
  // Expire any old proxy cache entries
  time_t now = time(NULL);
  Proxy::CacheEntry empty;
  std::replace_if(Proxy::safeHosts.begin(), Proxy::safeHosts.end(),
    std::bind2nd(std::mem_fun_ref(&Proxy::CacheEntry::isExpired), now), empty);

  ProxyProcessor p(readset, writeset);

  Proxy::items.remove_if(p);
}


bool
Proxy::ProxyIsChecking::operator()(ProxyPtr proxy)
{
  return ((proxy->address() == this->_address) &&
    (proxy->port() == this->_port) && (proxy->type() == this->_type));
}


bool
Proxy::isChecking(const std::string & address, const BotSock::Port port,
  const ProxyType type)
{
  return (Proxy::items.end() != std::find_if(Proxy::items.begin(),
    Proxy::items.end(), ProxyIsChecking(address, port, type)));
}


void
Proxy::addToCache(const std::string & address, const BotSock::Port port,
  const ProxyType type)
{
  Proxy::CacheEntry newEntry(BotSock::inet_addr(address), port, type);
  Proxy::CacheEntry empty;

  Proxy::Cache::iterator findEmpty = std::find(Proxy::safeHosts.begin(),
    Proxy::safeHosts.end(), empty);
  if (findEmpty != Proxy::safeHosts.end())
  {
    // Found an empty slot!  Now use it!
    *findEmpty = newEntry;
  }
  else if (!Proxy::safeHosts.empty())
  {
    // No empty slot found!  Find the oldest entry and replace it.
    std::sort(Proxy::safeHosts.begin(), Proxy::safeHosts.end());

    Proxy::safeHosts[0] = newEntry;
  }
}


bool
Proxy::isVerifiedClean(const std::string & address, const BotSock::Port port,
  const ProxyType type)
{
  const Proxy::CacheEntry tmp(BotSock::inet_addr(address), port, type);

  Proxy::Cache::const_iterator pos = std::find(Proxy::safeHosts.begin(),
    Proxy::safeHosts.end(), tmp);

  bool result = (pos != Proxy::safeHosts.end());

#ifdef PROXY_DEBUG
  if (result)
  {
    std::cout << "Verified clean: " << address << ":" << port << " (" <<
      type << ")" << std::endl;
  }
#endif

  return result;
}


void
Proxy::status(StrList & output)
{
  output.push_back("Proxy scanners: " + IntToStr(Proxy::items.size()));

  int cacheCount = std::count_if(Proxy::safeHosts.begin(),
    Proxy::safeHosts.end(), std::not1(std::mem_fun_ref(&Proxy::CacheEntry::isEmpty)));
  for (Proxy::Cache::iterator pos = Proxy::safeHosts.begin();
    pos < Proxy::safeHosts.end(); ++pos)
  {
    if (!pos->isEmpty())
    {
      ++cacheCount;
    }
  }
  output.push_back("Proxy cache: " + IntToStr(cacheCount) + "/" +
    IntToStr(Proxy::safeHosts.size()));
}

