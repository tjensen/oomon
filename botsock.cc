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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

// OOMon headers
#include "oomon.h"
#include "botsock.h"
#include "botexcept.h"
#include "util.h"


#ifdef DEBUG
# define BOTSOCK_DEBUG
#endif


const BotSock::Address BotSock::vhost_netmask =
  BotSock::inet_addr(VHOST_NETMASK);
const BotSock::Address BotSock::ClassCNetMask =
  BotSock::inet_addr("255.255.255.0");


BotSock::BotSock(const bool _blocking, const bool lineBuffered)
  : buffer(""), bindAddress(INADDR_ANY), timeout(0), connected(false),
  connecting(false), listening(false), blocking(_blocking), binary(false),
  backlog(1)
{
  if ((this->plug = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    throw OOMon::socket_error("socket() failed");
  } 

  this->setBuffering(lineBuffered);
  this->gotActivity();
}


BotSock::BotSock(const BotSock *listener, const bool blocking,
  const bool lineBuffered)
  : buffer(""), bindAddress(listener->bindAddress), timeout(listener->timeout),
  connected(true), connecting(false), listening(false),
  backlog(listener->backlog)
{
  struct sockaddr remotehost;
  memset(&remotehost, 0, sizeof(remotehost));
  socklen_t size = sizeof(remotehost);
  if (-1 == (this->plug = ::accept(listener->plug, &remotehost, &size)))
  {
    throw OOMon::accept_error("accept() failed");
  }

  this->setBlocking(listener->isBlocking());
  this->setBuffering(listener->isBuffering());
  this->setBinary(listener->isBinary());

  this->gotActivity();
}


BotSock::~BotSock()
{
  if (0 != ::close(this->plug))
  {
    throw OOMon::close_error("close() failed");
  }
}


bool
BotSock::connect(const BotSock::Address address, const BotSock::Port port)
{
  this->setOptions();

  if (!this->bind(this->bindAddress))
  {
    return false;
  }

  if (!this->setBlocking(blocking))
  {
    return false;
  }

  struct sockaddr_in socketname;
  memset(&socketname, 0, sizeof(socketname));
  socketname.sin_addr.s_addr = address;
  socketname.sin_family = AF_INET;
  socketname.sin_port = htons(port);

#ifdef BOTSOCK_DEBUG
  std::cout << "BotSock::connect(): ::connect()" << std::endl;
#endif

  if (::connect(this->plug, reinterpret_cast<struct sockaddr *>(&socketname),
    sizeof(socketname)) < 0)
  {
    if (errno == EINPROGRESS)
    {
      this->gotActivity();
      this->connecting = true;
      return true;
    }
    else
    { 
      std::cerr << "connect() error: " << errno << std::endl;
      this->connected = this->connecting = true;
      return false;
    }
  }

  this->connected = true;

  this->gotActivity();
  
  this->connectTime = ::time(NULL);

#ifdef BOTSOCK_DEBUG
  std::cout << "BotSock::connect(): this->onConnect()" << std::endl;
#endif

  return this->onConnect();
}


bool
BotSock::connect(const std::string & address, const BotSock::Port port)
{
  BotSock::Address addr;

  if (INADDR_NONE == (addr = BotSock::nsLookup(address)))
  {
    return false;
  }

  return this->connect(addr, port);
}


bool
BotSock::listen(const BotSock::Port port, const int backlog)
{
  this->setOptions();

  if (!this->bind(this->bindAddress, port))
  {
    return false;
  }

  if (!this->setBlocking(blocking))
  {
    return false;
  }

  if (::listen(this->plug, backlog) < 0)
  {
    std::cerr << "listen() error: " << errno << std::endl;
    this->connected = this->connecting = this->listening = false;
    return false;
  }

  listening = true;

  this->gotActivity();

  return true;
}


int
BotSock::read(void *buffer, int size)
{
  return (::read(this->plug, buffer, size));
}


int
BotSock::write(void *buffer, int size)
{
  return (::write(this->plug, buffer, size));
}


int
BotSock::write(const std::string & text)
{
  return (::write(this->plug, text.c_str(), text.length()));
}


bool
BotSock::process(const fd_set & readset, const fd_set & writeset)
{
  if (listening)
  {
    if (FD_ISSET(this->plug, &readset))
    {
#ifdef BOTSOCK_DEBUG
      std::cout << "BotSock::process(): listener is ready for accept()" <<
	std::endl;
#endif

      throw OOMon::ready_for_accept("Listening socket has connection waiting");
    }
  }
  else
  {
    if (FD_ISSET(this->plug, &readset))
    {
      static char buff[2048];

      memset(buff, 0, sizeof(buff));

#ifdef BOTSOCK_DEBUG
      std::cout << "BotSock::process(): this->read()" << std::endl;
#endif

      int n = this->read(buff, sizeof(buff) - 1);

      if (n > 0)
      {
	buff[n] = '\0';		// Make sure it is null terminated!

        this->gotActivity();

#ifdef BOTSOCK_DEBUG
        std::cout << "BotSock::process(): this->onRead()" << std::endl;
#endif
        if (this->isBinary() && (this->buffer.length() == 0))
	{
          return this->onRead(buff, n);
	}
	else if (this->isBuffering())
	{
	  std::string::size_type eol;

          this->buffer += buff;

	  while (std::string::npos !=
	    (eol = this->buffer.find_first_of("\r\n")))
	  {
	    std::string line = this->buffer.substr(0, eol);
	    this->buffer = this->buffer.substr(eol + 1);

	    if (!this->onRead(line))
	    {
	      return false;
	    }
	  }
	  return true;
	}
	else
	{
	  if (this->buffer.length() == 0)
	  {
            return this->onRead(buff);
	  }
	  else
	  {
	    bool result = this->onRead(buff + buffer);
	    buffer = "";
	    return result;
	  }
	}
      }
      else if (n == 0)
      {
        std::cerr << "BotSock::process(): EOF of socket" << std::endl;

        return false;
      }
      else
      {
        std::cerr << "BotSock::read() error " << errno << std::endl;

        return false;
      }
    }

    if (!this->isConnected())
    {
      if (FD_ISSET(this->plug, &writeset))
      {
	this->connecting = false;
        this->connected = true;

        this->connectTime = ::time(NULL);

#ifdef BOTSOCK_DEBUG
        std::cout << "BotSock::process(): this->onConnect()" << std::endl;
#endif

        if (!this->onConnect())
        {
	  return false;
        }
      }
    }
  }

  if ((this->getTimeout() > 0) && (this->getIdle() > this->getTimeout()))
  {
    throw OOMon::timeout_error("Connection timed out");
  }

  return true;
}


void
BotSock::setFD(fd_set & readset, fd_set & writeset) const
{
  FD_SET(this->plug, &readset);
  if (this->isConnecting())
  {
    FD_SET(this->plug, &writeset);
  }
}


void
BotSock::bindTo(const std::string & address)
{
  if (address == "")
  {
    this->bindAddress = INADDR_ANY;
  }
  else
  {
    this->bindAddress = BotSock::nsLookup(address);
  }
}


BotSock::Address
BotSock::getLocalAddress() const
{
  struct sockaddr_in localaddr;
  socklen_t size = sizeof(localaddr);

  memset(&localaddr, 0, sizeof(localaddr));

  if (-1 == ::getsockname(this->plug,
    reinterpret_cast<struct sockaddr *>(&localaddr), &size))
  {
    return INADDR_NONE;
  }

  return localaddr.sin_addr.s_addr;
}


BotSock::Port
BotSock::getLocalPort() const
{
  struct sockaddr_in localaddr;
  socklen_t size = sizeof(localaddr);

  memset(&localaddr, 0, sizeof(localaddr));

  if (-1 == ::getsockname(this->plug,
    reinterpret_cast<struct sockaddr *>(&localaddr), &size))
  {
    return 0;
  }

  return localaddr.sin_port;
}


BotSock::Address
BotSock::getRemoteAddress() const
{
  struct sockaddr_in remoteaddr;
  socklen_t size = sizeof(remoteaddr);

  memset(&remoteaddr, 0, sizeof(remoteaddr));

  if (-1 == ::getpeername(this->plug,
    reinterpret_cast<struct sockaddr *>(&remoteaddr), &size))
  {
    return INADDR_NONE;
  }

  return remoteaddr.sin_addr.s_addr;
}


BotSock::Port
BotSock::getRemotePort() const
{
  struct sockaddr_in remoteaddr;
  socklen_t size = sizeof(remoteaddr);

  memset(&remoteaddr, 0, sizeof(remoteaddr));

  if (-1 == ::getpeername(this->plug,
    reinterpret_cast<struct sockaddr *>(&remoteaddr), &size))
  {
    return 0;
  }

  return remoteaddr.sin_port;
}


bool
BotSock::setBlocking(const bool value)
{
  this->blocking = value;

  if (this->plug != -1)
  {
    int flags = ::fcntl(this->plug, F_GETFL);

    if (value)
    {
      flags &= ~O_NONBLOCK;
    }
    else
    {
      flags |= O_NONBLOCK;
    }

    if (-1 == ::fcntl(this->plug, F_SETFL, flags))
    {
      std::cerr << "fcntl() error: " << errno << std::endl;
      return false;
    }
  }
  return true;
}


void
BotSock::setBuffering(const bool value)
{
  this->lineBuffered = value;
}


BotSock::Address
BotSock::nsLookup(const std::string & address)
{
  BotSock::Address addr;

  if (INADDR_NONE == (addr = BotSock::inet_addr(address)))
  {
    struct hostent *host = gethostbyname(address.c_str());

    if (NULL == host)
    {
      std::cerr << "Error: gethostbyname() " << errno <<
#if defined(HAVE_STRERROR)
	" (" << ::strerror(errno) << ")" <<
#endif
	std::endl;
      return INADDR_NONE;
    }

    memcpy(&addr, host->h_addr, sizeof(addr));
  }

  return addr;
}


std::string
BotSock::nsLookup(const BotSock::Address & address)
{
  struct hostent *name;
  std::string result;

  name = gethostbyaddr(reinterpret_cast<const char *>(&address),
    sizeof(address), AF_INET);

  if (name)
  {
    result = name->h_name;
  }
  else
  {
    std::cerr << "Error: gethostbyaddr() " << errno <<
#if defined(HAVE_STRERROR)
      " (" << ::strerror(errno) << ")" <<
#endif
      std::endl;
  }

  return result;
}


std::string
BotSock::inet_ntoa(const BotSock::Address & address)
{
  struct in_addr ip;

  ip.s_addr = address;

  return (::inet_ntoa(ip));
}


void
BotSock::setOptions()
{
  int optval = 1;

  // Parameter 4 is cast to (char *) because of Slowaris dumbness
  setsockopt(this->plug, SOL_SOCKET, SO_REUSEADDR,
    reinterpret_cast<char *>(&optval), sizeof(optval));
  setsockopt(this->plug, SOL_SOCKET, SO_KEEPALIVE,
    reinterpret_cast<char *>(&optval), sizeof(optval));
}


bool
BotSock::bind(const BotSock::Address & address, const BotSock::Port & port)
{
  struct sockaddr_in localaddr;

  memset(&localaddr, 0, sizeof(localaddr));

  localaddr.sin_family = AF_INET;
  localaddr.sin_addr.s_addr = ((address == INADDR_NONE) ? INADDR_ANY : address);
  localaddr.sin_port = port;

  if (::bind(this->plug, reinterpret_cast<struct sockaddr *>(&localaddr),
    sizeof(localaddr)) < 0)
  {
    std::cerr << "bind() error: " << errno << std::endl;
    return false;
  }

  return true;
}


bool
BotSock::reset()
{
  ::close(this->plug);

  this->connected = this->connecting = this->listening = false;

  this->plug = ::socket(AF_INET, SOCK_STREAM, 0);

  return (-1 != this->plug);
}


std::string
BotSock::getUptime(void) const
{
  return timeDiff(::time(NULL) - this->connectTime);
}


//////////////////////////////////////////////////////////////////////
// sameClassC(ip1, ip2)
//
// Description:
//  Compares two IP addresses to be in the same class-C (/24) subnet.
//
// Parameters:
//  ip1 - The first IP address.
//  ip2 - The second IP address.
//
// Return Value:
//  The function returns true if both IP addresses are within the same
//  class-C (/24) subnet.
//////////////////////////////////////////////////////////////////////
bool
BotSock::sameClassC(const BotSock::Address & ip1, const BotSock::Address & ip2)
{
  return ((ip1 & BotSock::ClassCNetMask) == (ip2 & BotSock::ClassCNetMask));
}


