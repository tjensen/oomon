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
#include <cerrno>
#include <ctime>

// Std C headers
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

// OOMon headers
#include "oomon.h"
#include "botsock.h"
#include "botexcept.h"
#include "util.h"


#ifdef DEBUG
# define BOTSOCK_DEBUG
#endif


const BotSock::Address BotSock::ClassCNetMask =
  BotSock::inet_addr("255.255.255.0");


BotSock::BotSock(const bool blocking_, const bool lineBuffered)
#ifdef OLD_BOTSOCK_LINEBUFFER
  : buffer(""),
#else
  : bufferPos(0),
#endif
  bindAddress(INADDR_ANY), timeout(0), connected(false), connecting(false),
  listening(false), blocking(blocking_), binary(false), backlog(1)
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
#ifdef OLD_BOTSOCK_LINEBUFFER
  : buffer(""),
#else
  : bufferPos(0),
#endif
  bindAddress(listener->bindAddress), timeout(listener->timeout),
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


BotSock::~BotSock(void)
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
  
  this->connectTime = std::time(NULL);

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
#ifdef OLD_BOTSOCK_LINEBUFFER
      static char buff[2048];

      memset(buff, 0, sizeof(buff));
#endif

#ifdef BOTSOCK_DEBUG
      std::cout << "BotSock::process(): this->read()" << std::endl;
#endif

#ifdef OLD_BOTSOCK_LINEBUFFER
      int n = this->read(buff, sizeof(buff) - 1);
#else
      int n = this->read(this->buffer + this->bufferPos,
        sizeof(this->buffer) - this->bufferPos);
#endif

      if (n > 0)
      {
#ifdef OLD_BOTSOCK_LINEBUFFER
	buff[n] = '\0';		// Make sure it is null terminated!

        this->gotActivity();

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
#else
        this->bufferPos += n;

        this->gotActivity();

        if (this->isBinary())
	{
          return this->onRead(this->buffer, this->bufferPos);
	}
	else if (this->isBuffering())
	{
          char * cr;
          char * nl;
          do {
            int eol = -1;

            // find first end-of-line character
            cr = reinterpret_cast<char *>(memchr(this->buffer, '\r',
              this->bufferPos));
            nl = reinterpret_cast<char *>(memchr(this->buffer, '\n',
              this->bufferPos));
            if (cr && nl)
            {
              if (cr < nl)
              {
                eol = cr - this->buffer;
              }
              else
              {
                eol = nl - this->buffer;
              }
            }
            else if (cr)
            {
              eol = cr - this->buffer;
            }
            else if (nl)
            {
              eol = nl - this->buffer;
            }

            if (eol >= 0)
            {
              std::string line(this->buffer, eol);
              if (this->bufferPos > (eol + 1))
              {
                memmove(this->buffer, this->buffer + eol + 1,
                  this->bufferPos - (eol + 1));
                this->bufferPos -= (eol + 1);
              }
              else
              {
                this->bufferPos = 0;
              }

              if (!this->onRead(line))
              {
                return false;
              }
            }
            else if (this->bufferPos == sizeof(this->buffer))
            {
              std::string line(this->buffer, sizeof(this->buffer));
              this->bufferPos = 0;

              if (!this->onRead(line))
              {
                return false;
              }
            }
          } while (cr || nl);

	  return true;
	}
        else
        {
          std::string line(this->buffer, this->bufferPos);
          this->bufferPos = 0;

          if (!this->onRead(line))
          {
            return false;
          }
        }
#endif
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

        this->connectTime = std::time(NULL);

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
BotSock::getLocalAddress(void) const
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
BotSock::getLocalPort(void) const
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
BotSock::getRemoteAddress(void) const
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
BotSock::getRemotePort(void) const
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
BotSock::setOptions(void)
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
BotSock::reset(void)
{
  ::close(this->plug);

  this->connected = this->connecting = this->listening = false;

  this->plug = ::socket(AF_INET, SOCK_STREAM, 0);

  return (-1 != this->plug);
}


std::string
BotSock::getUptime(void) const
{
  return timeDiff(std::time(NULL) - this->connectTime);
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


void
BotSock::registerOnConnectHandler(OnConnectHandler func)
{
  this->onConnectHandler = func;
}


void
BotSock::registerOnReadHandler(OnReadHandler func)
{
  this->onReadHandler = func;
}


void
BotSock::registerOnBinaryReadHandler(OnBinaryReadHandler func)
{
  this->onBinaryReadHandler = func;
}


bool
BotSock::onConnect(void)
{
  bool result = true;

  if (!this->onConnectHandler.empty())
  {
#ifdef BOTSOCK_DEBUG
    std::cout << "BotSock::onConnect()" << std::endl;
#endif

    if (!this->onConnectHandler())
    {
      result = false;
    }
  }

  return result;
}


bool
BotSock::onRead(const std::string & text)
{
  bool result = true;

  if (!this->onReadHandler.empty())
  {
#ifdef BOTSOCK_DEBUG
    std::cout << "BotSock::onRead()" << std::endl;
#endif

    if (!this->onReadHandler(text))
    {
      result = false;
    }
  }

  return result;
}


bool
BotSock::onRead(const char *data, const int size)
{
  bool result = true;

  if (!this->onBinaryReadHandler.empty())
  {
#ifdef BOTSOCK_DEBUG
    std::cout << "BotSock::onBinaryRead()" << std::endl;
#endif

    if (!this->onBinaryReadHandler(data, size))
    {
      result = false;
    }
  }

  return result;
}


