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

#include "oomon.h"

#include <string>
#include <list>
#include <algorithm>
#include <functional>

#include "strtype"
#include "remotelist.h"
#include "remote.h"
#include "dcclist.h"
#include "util.h"
#include "config.h"
#include "botexcept.h"
#include "log.h"


//#ifdef DEBUG
# define REMOTELIST_DEBUG
//#endif /* DEBUG */


RemoteList remotes;


void
RemoteList::shutdown(void)
{
  this->shutdownListeners();

  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); )
  {
    Remote *copy = *pos;

    pos = this->_connections.erase(pos);
    delete copy;
  }
}


void
RemoteList::shutdownListeners(void)
{
  for (ListenerList::iterator pos = this->_listeners.begin();
    pos != this->_listeners.end(); ++pos)
  {
    BotSock *copy = *pos;

    pos = this->_listeners.erase(pos);
    delete copy;
  }
}


void
RemoteList::setFD(fd_set & readset, fd_set & writeset) const
{
  for (ListenerList::const_iterator pos = this->_listeners.begin();
    pos != this->_listeners.end(); ++pos)
  {
    BotSock *copy = *pos;
    copy->setFD(readset, writeset);
  }

  for (ConnectionList::const_iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;
    copy->setFD(readset, writeset);
  }
}


void
RemoteList::processAll(const fd_set & readset, const fd_set & writeset)
{
  for (ListenerList::iterator pos = this->_listeners.begin();
    pos != this->_listeners.end(); )
  {
    bool keepListener = true;
    BotSock *copy = *pos;

    try
    {
      keepListener = copy->process(readset, writeset);
    }
    catch (OOMon::ready_for_accept)
    {
#ifdef REMOTELIST_DEBUG
      std::cout << "Remote listener threw exception: ready_for_accept" <<
	std::endl;
#endif
      try
      {
        Remote *temp = new Remote(copy);

        if (temp->onConnect())
        {
          this->_connections.push_back(temp);
        }
        else
        {
          delete temp;
        }
      }
      catch (OOMon::errno_error & e)
      {
#ifdef REMOTELIST_DEBUG
        std::cout << "Remote::accept() threw exception: " << e.why() <<
	  std::endl;
#endif
        keepListener = (EWOULDBLOCK == e.getErrno());
	if (EWOULDBLOCK != e.getErrno())
	{
          Log::Write("Remote bot accept error: " + e.why());
        }
      }
    }

    if (keepListener)
    {
      ++pos;
    }
    else
    {
      pos = this->_listeners.erase(pos);
      delete copy;
    }
  }

  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); )
  {
    bool keepConnection = true;
    Remote *copy = *pos;

    try
    {
      keepConnection = copy->process(readset, writeset);

      if (!keepConnection)
      {
	std::string handle = copy->getHandle();
	if (handle.empty())
	{
	  handle = "unknown";
	}
#ifdef REMOTELIST_DEBUG
        std::cout << "Remote bot (" + handle + ") closed connection" <<
	  std::endl;
#endif
        Log::Write("Remote bot (" + handle + ") closed connection");
      }
    }
    catch (OOMon::timeout_error)
    {
#ifdef REMOTELIST_DEBUG
      std::cout << "Remote::process() threw exception: timeout_error" <<
	std::endl;
#endif
      keepConnection = false;

      if (copy->isConnected())
      {
        copy->sendError("Timeout");
	std::string handle = copy->getHandle();
	if (handle.empty())
	{
	  handle = "unknown";
	}
        Log::Write("Remote bot connection to " + handle + " timed out");
      }
    }
    catch (OOMon::errno_error & e)
    {
      keepConnection = false;
#ifdef REMOTELIST_DEBUG
      std::cout << "Remote::process() threw exception: errno_error: " <<
	e.why() << std::endl;
#endif
      copy->sendError(e.why());
      Log::Write("Remote bot connection error: " + e.why());
    }

    if (keepConnection)
    {
      ++pos;
    }
    else
    {
      pos = this->_connections.erase(pos);

      if (copy->ready())
      {
	this->sendBotPart(Config::GetNick(), copy->getHandle());
	clients.sendAll("*** Bot " + copy->getHandle() + " has disconnected",
	  UF_OPER);
      }

      delete copy;
    }
  }
}


void
RemoteList::send(const std::string & From, const std::string & Command,
  const std::string & To, const std::string & Params,
  const class Remote *exception)
{
}


void
RemoteList::sendChat(const std::string & from, const std::string & text,
  const Remote *exception)
{
  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if (copy != exception)
    {
      copy->sendChat(from, text);
    }
  }
}


void
RemoteList::sendBotJoin(const std::string & oldnode,
  const std::string & newnode, const Remote *exception)
{
  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if (copy != exception)
    {
      copy->sendBotJoin(oldnode, newnode);
    }
  }
}


void
RemoteList::sendBotPart(const std::string & from, const std::string & node,
  const Remote *exception)
{
  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if (copy != exception)
    {
      copy->sendBotPart(from, node);
    }
  }
}


void
RemoteList::conn(const std::string & from, const std::string & to,
  const std::string & target)
{
  if (to.empty() || Same(to, Config::GetNick()))
  {
    this->connect(target);
  }
}


void
RemoteList::disconn(const std::string & From, const std::string & To,
  const std::string & Target)
{
}


void
RemoteList::getLinks(StrList & Output)
{
  Output.push_back(Config::GetNick());

  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if (copy != this->_connections.back())
    {
      Output.push_back("|\\");
      copy->getLinks(Output, "| ");
    }
    else
    {
      Output.push_back(" \\");
      copy->getLinks(Output, "  ");
    }
  }
}


void
RemoteList::getBotNet(BotLinkList & list, const Remote *exception)
{
  list.clear();

  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if ((copy != exception) && copy->ready())
    {
      list.push_back(BotLink(Config::GetNick(), copy->getHandle()));
      copy->getBotNetBranch(list);
    }
  }
}


bool
RemoteList::listen(const std::string & address, const BotSock::Port & port)
{
  bool result = false;

  Remote *temp = new Remote;

  if (!address.empty())
  {
    temp->bindTo(address);
  }

  if (temp->listen(port, 5))
  {
    this->_listeners.push_back(temp);

    result = true;
  }
  else
  {
    delete temp;
  }

  return result;
}


void
RemoteList::listen(void)
{
  this->shutdownListeners();

  this->listen("", htons(Config::GetPort()));
}


bool
RemoteList::connect(const std::string & handle)
{
  std::string _handle(handle);
  std::string host;
  BotSock::Port port;
  std::string password;
  int flags;
  bool result = false;

  if (Config::GetConn(_handle, host, port, password, flags))
  {
    Remote *temp = new Remote(_handle);

    temp->connect(host, port);

    this->_connections.push_back(temp);

    result = true;
  }

  return result;
} 


bool
RemoteList::isLinkedDirectlyToMe(const std::string & Name) const
{
  for (ConnectionList::const_iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if (Same(Name, copy->getHandle()))
    {
      return true;
    }
  }
  return false;
}   


Remote *
RemoteList::findBot(const std::string & To) const
{
  for (ConnectionList::const_iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    Remote *copy = *pos;

    if (copy->isConnectedTo(To))
    {
      return copy;
    }
  }
  return NULL;
}


bool
RemoteList::isConnected(const std::string & handle) const
{
  return (0 != this->findBot(handle));
}

