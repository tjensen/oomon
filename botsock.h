#ifndef __BOTSOCK_H__
#define __BOTSOCK_H__
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

// Std C headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// OOMon C++ headers
#include "oomon.h"


#ifndef INADDR_NONE
# define INADDR_NONE	0xffffffff
#endif
#ifndef INADDR_ANY
# define INADDR_ANY	0
#endif


class BotSock
{
public:
  typedef in_addr_t Address;
  typedef in_port_t Port;

  BotSock(const bool _blocking = false, const bool lineBuffered = false);
  BotSock(const BotSock *listener, const bool blocking = false,
    const bool lineBuffered = false);
  virtual ~BotSock();

  bool connect(const Address address, const Port port);
  bool connect(const std::string & address, const Port port);

  bool reset();

  bool listen(const Port port = 0, const int backlog = 1);

  int read(void *buffer, int size);
  int write(void *buffer, int size);
  int write(const std::string & text);

  bool process(const fd_set &, const fd_set &);
  void setFD(fd_set &, fd_set &) const;

  void bindTo(const std::string & address);

  Address getLocalAddress() const;
  Port getLocalPort() const;

  Address getRemoteAddress() const;
  Port getRemotePort() const;

  bool setBlocking(const bool value);
  bool isBlocking() const { return this->blocking; };

  void setBuffering(const bool value);
  bool isBuffering() const { return this->lineBuffered; };

  bool isConnected() const { return this->connected; };
  bool isConnecting() const { return this->connecting; };
  bool isListening() const { return this->listening; };

  void setBinary(const bool value) { this->binary = value; };
  bool isBinary() const { return this->binary; };

  void setTimeout(const time_t value) { this->timeout = value; };
  time_t getTimeout() const { return this->timeout; };
  time_t getIdle() const { return (time(NULL) - this->lastActivity); };
  void gotActivity() { this->lastActivity = time(NULL); };

  std::string getUptime(void) const;

  static Address nsLookup(const std::string & address);
  static std::string nsLookup(const BotSock::Address & address);
  static std::string inet_ntoa(const BotSock::Address & address);
  static Address inet_addr(const std::string & address)
    { return ::inet_addr(address.c_str()); };

  static bool sameClassC(const BotSock::Address & ip1,
    const BotSock::Address & ip2);

  static const BotSock::Address vhost_netmask;
  static const BotSock::Address ClassCNetMask;

  virtual bool onConnect() { return true; };

protected:
  virtual bool onRead(std::string) { return true; };
  virtual bool onRead(const char *, const int) { return true; };

private:
  void setOptions();
  bool bind(const BotSock::Address & address = INADDR_ANY,
    const BotSock::Port & port = 0);

  std::string buffer;
  Address bindAddress;
  time_t timeout, lastActivity, connectTime;
  bool connected, connecting, listening, blocking, lineBuffered, binary;
  int plug, backlog;
};


#endif /* __BOTSOCK_H__ */

