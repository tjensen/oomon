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
#include "botclient.h"
#include "botexcept.h"
#include "log.h"


//#ifdef DEBUG
# define REMOTELIST_DEBUG
//#endif /* DEBUG */


RemoteList remotes;


void
RemoteList::shutdown(void)
{
  this->_listeners.clear();
  this->_connections.clear();
}


void
RemoteList::setFD(fd_set & readset, fd_set & writeset) const
{
  BotSock::FDSetter s(readset, writeset);

  std::for_each(this->_listeners.begin(), this->_listeners.end(), s);
  std::for_each(this->_connections.begin(), this->_connections.end(), s);
}


bool
RemoteList::ListenProcess::operator()(BotSock::ptr s)
{
  bool remove;

  try
  {
    remove = !s->process(this->_readset, this->_writeset);
  }
  catch (OOMon::ready_for_accept)
  {
#ifdef REMOTELIST_DEBUG
    std::cout << "Remote listener threw exception: ready_for_accept" <<
      std::endl;
#endif

    remove = false;

    RemotePtr temp;
    bool addRemote = true;

    try
    {
      temp = RemotePtr(new Remote(s.get()));
    }
    catch (OOMon::errno_error & e)
    {
#ifdef REMOTELIST_DEBUG
      std::cout << "Remote::accept() threw exception: " << e.why() << std::endl;
#endif
      if (EWOULDBLOCK == e.getErrno())
      {
        addRemote = (EWOULDBLOCK == e.getErrno());
      }
      else
      {
        Log::Write("Remote bot accept error: " + e.why());
      }
    }

    if (addRemote && temp->onConnect())
    {
      this->_connections.push_back(temp);
    }
  }

  return remove;
}


bool
RemoteList::RemoteProcess::operator()(RemotePtr r)
{
  bool remove;

  try
  {
    remove = !r->process(this->_readset, this->_writeset);

    if (remove)
    {
      std::string handle = r->getHandle();
      if (handle.empty())
      {
        handle = "unknown";
      }
#ifdef REMOTELIST_DEBUG
      std::cout << "Remote bot (" + handle + ") closed connection" << std::endl;
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
    remove = true;

    if (r->isConnected())
    {
      r->sendError("Timeout");
      std::string handle = r->getHandle();
      if (handle.empty())
      {
        handle = "unknown";
      }
      Log::Write("Remote bot connection to " + handle + " timed out");
    }
  }
  catch (OOMon::errno_error & e)
  {
    remove = true;
#ifdef REMOTELIST_DEBUG
    std::cout << "Remote::process() threw exception: errno_error: " <<
      e.why() << std::endl;
#endif
    r->sendError(e.why());
    Log::Write("Remote bot connection error: " + e.why());
  }

  if (remove && r->ready())
  {
    remotes.sendBotPart(Config::GetNick(), r->getHandle());
    clients.sendAll("*** Bot " + r->getHandle() + " has disconnected", UF_OPER);
  }

  return remove;
}


void
RemoteList::processAll(const fd_set & readset, const fd_set & writeset)
{
  ListenProcess lp(readset, writeset, this->_connections);
  this->_listeners.remove_if(lp);

  RemoteProcess rp(readset, writeset);
  this->_connections.remove_if(rp);
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
  std::for_each(this->_connections.begin(), this->_connections.end(),
    RemoteList::SendChat(from, text, exception));
}


void
RemoteList::sendBotJoin(const std::string & oldnode,
  const std::string & newnode, const Remote *exception)
{
  std::for_each(this->_connections.begin(), this->_connections.end(),
    SendBotJoinPart(true, oldnode, newnode, exception));
}


void
RemoteList::sendBotPart(const std::string & from, const std::string & node,
  const Remote *exception)
{
  std::for_each(this->_connections.begin(), this->_connections.end(),
    SendBotJoinPart(false, from, node, exception));
}


void
RemoteList::sendCommand(const BotClient::ptr from, const std::string & to,
  const std::string & command, const std::string & parameters)
{
  from->send("*** Remote commands aren't implemented yet!");
}


void
RemoteList::conn(const std::string & from, const std::string & target)
{
  this->connect(target);
}


void
RemoteList::disconn(const std::string & from, const std::string & target)
{
}


void
RemoteList::getLinks(StrList & Output)
{
  Output.push_back(Config::GetNick());

  for (ConnectionList::iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    RemotePtr copy(*pos);

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
    RemotePtr copy(*pos);

    if ((copy.get() != exception) && copy->ready())
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

  BotSock::ptr temp(new BotSock);

  if (!address.empty())
  {
    temp->bindTo(address);
  }

  if (temp->listen(port, 5))
  {
    this->_listeners.push_back(temp);

    result = true;
  }

  return result;
}


void
RemoteList::listen(void)
{
  this->_listeners.clear();

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
    RemotePtr temp(new Remote(_handle));

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
    RemotePtr copy(*pos);

    if (Same(Name, copy->getHandle()))
    {
      return true;
    }
  }
  return false;
}   


RemoteList::RemotePtr
RemoteList::findBot(const std::string & to) const
{
  RemotePtr result;

  for (ConnectionList::const_iterator pos = this->_connections.begin();
    pos != this->_connections.end(); ++pos)
  {
    RemotePtr copy(*pos);

    if (copy->isConnectedTo(to))
    {
      result = copy;
      break;
    }
  }
  return result;
}


bool
RemoteList::isConnected(const std::string & handle) const
{
  return (0 != this->findBot(handle));
}

