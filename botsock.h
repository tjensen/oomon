#ifndef __BOTSOCK_H__
#define __BOTSOCK_H__
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
#include <ctime>

// Boost C++ headers
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

// Std C headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// OOMon C++ headers
#include "oomon.h"


#ifndef INADDR_NONE
# define INADDR_NONE	0xffffffff
#endif
#ifndef INADDR_ANY
# define INADDR_ANY	0
#endif


class BotSock : private boost::noncopyable
{
public:
  typedef boost::shared_ptr<BotSock> ptr;
  typedef boost::function<bool (void)> OnConnectHandler;
  typedef boost::function<bool (std::string)> OnReadHandler;
  typedef boost::function<bool (const char *, const int)> OnBinaryReadHandler;
  typedef in_addr_t Address;
  typedef in_port_t Port;

  BotSock(const bool blocking_ = false, const bool lineBuffered = false);
  BotSock(const BotSock *listener, const bool blocking = false,
    const bool lineBuffered = false);
  virtual ~BotSock(void);

  bool connect(const Address address, const Port port);
  bool connect(const std::string & address, const Port port);

  bool reset(void);

  bool listen(const Port port = 0, const int backlog = 1);

  int read(void *buffer, int size);
  int write(void *buffer, int size);
  int write(const std::string & text);

  bool process(const fd_set &, const fd_set &);
  void setFD(fd_set &, fd_set &) const;

  void bindTo(const std::string & address);

  Address getLocalAddress(void) const;
  Port getLocalPort(void) const;

  Address getRemoteAddress(void) const;
  Port getRemotePort(void) const;

  bool setBlocking(const bool value);
  bool isBlocking(void) const { return this->blocking; };

  void setBuffering(const bool value);
  bool isBuffering(void) const { return this->lineBuffered; };

  bool isConnected(void) const { return this->connected; };
  bool isConnecting(void) const { return this->connecting; };
  bool isListening(void) const { return this->listening; };

  void setBinary(const bool value) { this->binary = value; };
  bool isBinary(void) const { return this->binary; };

  void setTimeout(const std::time_t value) { this->timeout = value; };
  std::time_t getTimeout(void) const { return this->timeout; };
  std::time_t getIdle(void) const
    { return (std::time(NULL) - this->lastActivity); };
  void gotActivity(void) { this->lastActivity = std::time(NULL); };

  std::string getUptime(void) const;

  void registerOnConnectHandler(OnConnectHandler func);
  void registerOnReadHandler(OnReadHandler func);
  void registerOnBinaryReadHandler(OnBinaryReadHandler func);

  static Address nsLookup(const std::string & address);
  static std::string nsLookup(const BotSock::Address & address);
  static std::string inet_ntoa(const BotSock::Address & address);
  static Address inet_addr(const std::string & address)
    { return ::inet_addr(address.c_str()); };

  static bool sameClassC(const BotSock::Address & ip1,
    const BotSock::Address & ip2);

  static const BotSock::Address ClassCNetMask;

  class FDSetter
  {
  public:
    FDSetter(fd_set & readset, fd_set & writeset) : readset_(readset),
      writeset_(writeset) { }
    void operator()(BotSock::ptr s)
    {
      s->setFD(this->readset_, this->writeset_);
    }
  private:
    fd_set & readset_;
    fd_set & writeset_;
  };

private:
  bool onConnect(void);
  bool onRead(const std::string & text);
  bool onRead(const char *data, const int size);

  void setOptions(void);
  bool bind(const BotSock::Address & address = INADDR_ANY,
    const BotSock::Port & port = 0);

  OnConnectHandler onConnectHandler;
  OnReadHandler onReadHandler;
  OnBinaryReadHandler onBinaryReadHandler;
  std::string buffer;
  Address bindAddress;
  std::time_t timeout, lastActivity, connectTime;
  bool connected, connecting, listening, blocking, lineBuffered, binary;
  int plug, backlog;
};


template <typename t>
class FDSetter
{
public:
  FDSetter(fd_set & readset, fd_set & writeset) : readset_(readset),
    writeset_(writeset) { }
  void operator()(t node)
  {
    node->setFD(this->readset_, this->writeset_);
  }
private:
  fd_set & readset_;
  fd_set & writeset_;
};


#endif /* __BOTSOCK_H__ */

