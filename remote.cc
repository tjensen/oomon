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

// Std C++ headers
#include <iostream>
#include <string>

// OOMon Headers
#include "strtype"
#include "remote.h"
#include "remotelist.h"
#include "dcclist.h"
#include "links.h"
#include "util.h"
#include "config.h"
#include "log.h"


#ifdef DEBUG
# define REMOTE_DEBUG
#endif


const std::string Remote::PROTOCOL_NAME("OOMon");
const int Remote::PROTOCOL_VERSION_MAJOR(2);
const int Remote::PROTOCOL_VERSION_MINOR(1);


Remote::Remote(void)
  : BotSock(false, true), _stage(Remote::STAGE_INIT), _client(true)
{
}


Remote::Remote(const std::string & handle)
  : BotSock(false, true), _handle(handle), _stage(Remote::STAGE_INIT),
  _client(true)
{
}


Remote::Remote(BotSock *listener)
  : BotSock(listener, false, true), _stage(Remote::STAGE_INIT), _client(false)
{
}


Remote::~Remote(void)
{
}


bool
Remote::isAuthorized(void) const
{
  bool result = false;

  std::string address = BotSock::inet_ntoa(this->getRemoteAddress());
  std::string hostname = this->getHostname();

  if (!hostname.empty())
  {
    // If hostname is valid, check it and the IP address
#ifdef REMOTE_DEBUG
    std::cout << "Remote host: " << hostname << " [" << address << "]" <<
      std::endl;
#endif
    result = Config::IsLinkable(hostname) || Config::IsLinkable(address);
  }
  else
  {
#ifdef REMOTE_DEBUG
    std::cout << "Remote host: " << address << std::endl;
#endif
    // If hostname is invalid, only check the IP address
    result = Config::IsLinkable(address);
  }

  return result;
}


bool
Remote::onConnect(void)
{
  bool result = false;

  BotSock::Address address = this->getRemoteAddress();
  std::string addressText = BotSock::inet_ntoa(address);
  std::string hostname = BotSock::nsLookup(address);

  if (!hostname.empty() && BotSock::nsLookup(hostname) == address)
  {
    this->_hostname = hostname;
  }

  if (this->isAuthorized())
  {
    result = true;
    if (this->isClient())
    {
      this->sendVersion();
    }
    this->setTimeout(60);
  }
  else
  {
    Log::Write("Connection attempt from unauthorized host " +
      BotSock::inet_ntoa(this->getRemoteAddress()) + ":" +
      IntToStr(ntohs(this->getRemotePort())));
  }

  return result;
}


bool
Remote::isCompatibleProtocolVersion(std::string text)
{
  bool result = false;

  std::string botType = FirstWord(text);

  if (Remote::PROTOCOL_NAME == botType)
  {
    std::string majorVersion = FirstWord(text);

    if (Remote::PROTOCOL_VERSION_MAJOR == atoi(majorVersion.c_str()))
    {
      std::string minorVersion = FirstWord(text);

      if (Remote::PROTOCOL_VERSION_MINOR == atoi(minorVersion.c_str()))
      {
	result = true;
      }
    }
  }

  return result;
}


bool
Remote::onRead(std::string text)
{
  bool result = false;

  if (text.empty())
  {
    result = true;
  }
  else
  {
#ifdef REMOTE_DEBUG
    std::cout << "Remote >> " << text << std::endl;
#endif

    switch (this->_stage)
    {
      case Remote::STAGE_INIT:
        result = Remote::isCompatibleProtocolVersion(text);
        if (result)
        {
#ifdef REMOTE_DEBUG
          std::cout << "Compatible OOMon protocol detected!" << std::endl;
#endif
          if (this->isClient())
	  {
	    this->sendAuth();
	  }
	  else
	  {
	    this->sendVersion();
	  }
	  this->_stage = Remote::STAGE_GOODVERSION;
        }
        else
        {
          this->sendError("Incompatible protocol detected");
        }
        break;
      case Remote::STAGE_GOODVERSION:
	result = authenticate(text);
	if (result)
	{
	  if (this->isServer())
	  {
	    this->sendAuth();
	  }
	  else
	  {
	    this->sendMyBotNet();
	  }
	  this->_stage = Remote::STAGE_AUTHED;
	}
	break;
      case Remote::STAGE_AUTHED:
      case Remote::STAGE_READY:
	result = parse(text);
	break;
      default:
        std::cerr << "Invalid Remote stage reached!" << std::endl;
        result = false;
        this->sendError("Invalid stage reached");
        break;
    }
  }

  return result;
}


bool
Remote::authenticate(std::string text)
{
  bool result = false;

  std::string command = UpCase(FirstWord(text));

  if (0 == command.compare("AUTH"))
  {
    std::string address = BotSock::inet_ntoa(this->getRemoteAddress());
    std::string hostname = this->getHostname();
    std::string handle = FirstWord(text);

    if (Config::AuthBot(handle, address, text) ||
      (!hostname.empty() && Config::AuthBot(handle, hostname, text)))
    {
#ifdef REMOTE_DEBUG
      std::cout << "Authentication successful!" << std::endl;
#endif
      this->_handle = handle;
      this->_children.setName(handle);
      result = true;
    }
    else
    {
#ifdef REMOTE_DEBUG
      std::cout << "Authentication failed!" << std::endl;
#endif
      this->sendError("Authentication failed");
    }
  }
  else
  {
    this->sendUnknownCommand(command);
  }

  return result;
}


bool
Remote::parse(std::string text)
{
  bool result = true;

  if ((text.size() > 1) && (text[0] == ':'))
  {
    text = text.substr(1);

    std::string from = FirstWord(text);

    std::string command = UpCase(FirstWord(text));

    if (0 == command.compare("BOTJOIN"))
    {
      result = this->onBotJoin(from, FirstWord(text));
    }
    else if (0 == command.compare("BOTPART"))
    {
      result = this->onBotPart(from, FirstWord(text));
    }
    else if (0 == command.compare("CHAT"))
    {
      result = this->onChat(from, text);
    }
    else
    {
      this->sendUnknownCommand(command);
      result = false;
    }
  }
  else
  {
    std::string command = UpCase(FirstWord(text));

    if (0 == command.compare("ERROR"))
    {
      clients.sendAll("*** Bot " + this->getHandle() + " reports error: " +
	text);
      result = false;
    }
    else
    {
      this->sendUnknownCommand(command);
    }
    result = false;
  }

  return result;
}


bool
Remote::onBotJoin(const std::string & from, const std::string & node)
{
  if (!node.empty())
  {
    this->_children.Link(from, node);
    remotes.sendBotJoin(from, node, this);
  }

  if (this->ready())
  {
    clients.sendAll("*** Bot " + node + " has connected to " + from,
      UserFlags::OPER);
    remotes.sendBotJoin(from, node, this);
  }
  else if (node.empty())
  {
    this->_stage = Remote::STAGE_READY;
    this->setTimeout(0);

    remotes.sendBotJoin(Config::GetNick(), this->getHandle(), this);
    clients.sendAll("*** Bot " + this->getHandle() + " has connected",
      UserFlags::OPER);

    if (this->isServer())
    {
      this->sendMyBotNet();
    }

    if (!this->_sendQ.empty())
    {
      this->write(this->_sendQ);
      this->_sendQ = "";
    }
  }

  return true;
}


bool
Remote::onBotPart(const std::string & from, const std::string & node)
{
  bool result = false;

  if (this->ready())
  {
    this->_children.Unlink(node);
    remotes.sendBotPart(from, node, this);

    clients.sendAll("*** Bot " + node + " has disconnected from " + from,
      UserFlags::OPER);

    result = true;
  }
  else
  {
    this->sendUnknownCommand("BOTPART");
  }

  return result;
}


bool
Remote::onChat(const std::string & from, const std::string & text)
{
  clients.sendChat(from, text);
  remotes.sendChat(from, text, this);

  return true;
}


int
Remote::sendError(const std::string & text)
{
  return this->sendCommand("", "ERROR", text);
}


int
Remote::sendUnknownCommand(const std::string & command)
{
  std::cerr << "Unknown remote command: " << command << std::endl;
  return this->sendError("Unknown command: " + command);
}


int
Remote::sendCommand(const std::string & from, const std::string & command,
  const std::string & parameters)
{
  std::string text;

  if (!from.empty())
  {
    text = ":" + from + " " + command;
  }
  else
  {
    text = command;
  }

  if (!parameters.empty())
  {
    text += " " + parameters;
  }

  return this->write(text + '\n');
}


int
Remote::sendChat(const std::string & from, const std::string & text)
{
  int result = 0;

  std::string cmd(":" + from + " CHAT " + text + '\n');

  if (this->ready())
  {
    result = this->write(cmd);
  }
  else
  {
    this->_sendQ += cmd;
  }

  return result;
}


int
Remote::sendBotJoin(const std::string & oldnode, const std::string & newnode)
{
  int result = 0;

  std::string cmd(":" + oldnode + " BOTJOIN " + newnode + '\n');

  if (this->ready())
  {
    result = this->write(cmd);
  }
  else
  {
    this->_sendQ += cmd;
  }

  return result;
}


int
Remote::sendBotPart(const std::string & from, const std::string & node)
{
  int result = 0;

  std::string cmd(":" + from + " BOTPART " + node + '\n');

  if (this->ready())
  {
    result = this->write(cmd);
  }
  else
  {
    this->_sendQ += cmd;
  }

  return result;
}


int
Remote::sendMyBotNet(void)
{
  int result = 0;

  BotLinkList net;
  remotes.getBotNet(net, this);

  for (BotLinkList::iterator pos = net.begin(); pos != net.end(); 
    pos = net.erase(pos))
  {
    result += this->sendCommand(pos->nodeA, "BOTJOIN", pos->nodeB);
  }
  this->sendCommand(Config::GetNick(), "BOTJOIN");

  return result;
}


int
Remote::sendVersion(void)
{
  return this->write(Remote::PROTOCOL_NAME + " " +
    IntToStr(Remote::PROTOCOL_VERSION_MAJOR) + " " +
    IntToStr(Remote::PROTOCOL_VERSION_MINOR) + '\n');
}


int
Remote::sendAuth(void)
{
  std::string handle = this->getHandle();
  std::string hostname;
  BotSock::Port port;
  std::string password;

  Config::GetConn(handle, hostname, port, password);

  return this->sendCommand("", "AUTH", Config::GetNick() + " " + password);
}


void
Remote::getLinks(StrList & Output, const std::string & prefix) const
{
  this->_children.getLinks(Output, prefix);
}


void
Remote::getBotNetBranch(BotLinkList & list) const
{
  this->_children.getBotList(list);
}


bool
Remote::isConnectedTo(const std::string & Name) const
{
  return this->_children.exists(Name);
}

