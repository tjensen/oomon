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


Remote::Remote(const std::string & handle)
  : _handle(handle), _stage(Remote::STAGE_INIT), _client(true),
  _sock(false, true), _targetEstablished(false)
{
  this->_sock.registerOnConnectHandler(boost::bind(&Remote::onConnect, this));
  this->_sock.registerOnReadHandler(boost::bind(&Remote::onRead, this, _1));
  this->_sock.setBuffering(true);
}


Remote::Remote(BotSock *listener)
  : _stage(Remote::STAGE_INIT), _client(false), _sock(listener, false, true),
  _targetEstablished(false)
{
  this->_sock.registerOnConnectHandler(boost::bind(&Remote::onConnect, this));
  this->_sock.registerOnReadHandler(boost::bind(&Remote::onRead, this, _1));
  this->_sock.setBuffering(true);
}


Remote::~Remote(void)
{
}


bool
Remote::isAuthorized(void) const
{
  bool result = false;

  std::string address = BotSock::inet_ntoa(this->_sock.getRemoteAddress());
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

  BotSock::Address address = this->_sock.getRemoteAddress();
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
    this->_sock.setTimeout(60);
  }
  else
  {
    Log::Write("Connection attempt from unauthorized host " +
      BotSock::inet_ntoa(this->_sock.getRemoteAddress()) + ":" +
      IntToStr(ntohs(this->_sock.getRemotePort())));
  }

  return result;
}


UserFlags
Remote::flags(void) const
{
  UserFlags result;

  if (this->_targetEstablished)
  {
    return this->_clientFlags;
  }
  else
  {
    std::cerr << "No target established for Remote::flags()" << std::endl;
  }

  return result;
}


std::string
Remote::handle(void) const
{
  std::string result;

  if (this->_targetEstablished)
  {
    return this->_clientHandle;
  }
  else
  {
    std::cerr << "No target established for Remote::handle()" << std::endl;
  }

  return result;
}


std::string
Remote::bot(void) const
{
  std::string result;

  if (this->_targetEstablished)
  {
    return this->_clientBot;
  }
  else
  {
    std::cerr << "No target established for Remote::bot()" << std::endl;
  }

  return result;
}


std::string
Remote::id(void) const
{
  std::string result;

  if (this->_targetEstablished)
  {
    return this->_clientId;
  }
  else
  {
    std::cerr << "No target established for Remote::id()" << std::endl;
  }

  return result;
}


void
Remote::send(const std::string & text)
{
  if (this->_targetEstablished)
  {
    this->sendNotice(Config::GetNick(), this->_clientId, this->_clientBot,
      text);
  }
  else
  {
    std::cerr << "No target established for Remote::send()" << std::endl;
  }
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
    std::string address = BotSock::inet_ntoa(this->_sock.getRemoteAddress());
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
    else if (0 == command.compare("COMMAND"))
    {
      result = this->onCommand(from, text);
    }
    else if (0 == command.compare("BROADCAST"))
    {
      result = this->onBroadcast(from, text);
    }
    else if (0 == command.compare("NOTICE"))
    {
      result = this->onNotice(from, text);
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
    this->_sock.setTimeout(0);

    remotes.sendBotJoin(Config::GetNick(), this->getHandle(), this);
    clients.sendAll("*** Bot " + this->getHandle() + " has connected",
      UserFlags::OPER);

    if (this->isServer())
    {
      this->sendMyBotNet();
    }

    if (!this->_sendQ.empty())
    {
      this->_sock.write(this->_sendQ);
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


bool
Remote::onCommand(const std::string & from, std::string text)
{
  bool result = true;
  try
  {
    std::string::size_type at(from.find('@'));
    if (std::string::npos != at)
    {
      std::string handle(from.substr(0, at));
      std::string bot(from.substr(at + 1));

      std::string to(FirstWord(text));
      std::string id(FirstWord(text));
      std::string command(FirstWord(text));
      std::string parameters(text);

      this->_clientHandle = handle;
      this->_clientBot = bot;
      this->_clientId = id;
      this->_clientFlags = Config::getRemoteFlags(from);

#ifdef REMOTE_DEBUG
      std::cout << handle << "@" << bot << " flags: " <<
	UserFlags::getNames(this->_clientFlags, ' ') << std::endl;
#endif /* REMOTE_DEBUG */

      this->_targetEstablished = true;
      if (Same(to, Config::GetNick()))
      {
        try
        {
          this->_parser.parse(this, command, parameters);
        }
        catch (CommandParser::exception & e)
        {
          this->send(e.what());
        }
      }
      else
      {
	remotes.sendCommand(this, to, command, parameters);
      }
      this->_targetEstablished = false;
    }
    else
    {
      this->sendError("Only users can send remote commands!");
      result = false;
    }
  }
  catch (...)
  {
    _targetEstablished = false;
    throw;
  }
  return result;
}


bool
Remote::onBroadcast(const std::string & from, std::string text)
{
  std::string skip(FirstWord(text));
  std::string flags(FirstWord(text));
  std::string watches(FirstWord(text));

  std::string::size_type at(skip.find('@'));
  if (std::string::npos != at)
  {
    std::string skipId = skip.substr(0, at);
    std::string skipBot = skip.substr(at + 1);

    if (Same(skipBot, Config::GetNick()))
    {
      clients.sendAll(from, skipId, text, flags, watches);
      remotes.sendBroadcastPtr(from, this, text, flags, watches);
    }
    else
    {
      clients.sendAll(from, text, flags, watches);
      remotes.sendBroadcastId(from, skipId, skipBot, text, flags, watches);
    }
  }
  else if (0 == skip.compare("*"))
  {
    clients.sendAll(from, text, flags, watches);
    remotes.sendBroadcastPtr(from, this, text, flags, watches);
  }
  else
  {
    this->sendError("Malformed skip ID in broadcast!");
  }

  return true;
}


bool
Remote::onNotice(const std::string & from, std::string text)
{
  std::string clientId(FirstWord(text));
  std::string clientBot(FirstWord(text));

  remotes.sendNotice(from, clientId, clientBot, text);

  return true;
}


int
Remote::sendBroadcast(const std::string & from, const std::string & text,
  const std::string & flags, const std::string & watches)
{
  std::string msg("* ");
  msg += flags;
  msg += ' ';
  msg += watches;
  msg += ' ';
  msg += text;
  msg += '\n';

  return this->sendCommand(from, "BROADCAST", msg);
}


int
Remote::sendBroadcastPtr(const std::string & from, const Remote *skip,
  const std::string & text, const std::string & flags,
  const std::string & watches)
{
  int result = 0;

  if (this != skip)
  {
    result = this->sendBroadcast(from, text, flags, watches);
  }

  return result;
}


int
Remote::sendBroadcastId(const std::string & from, const std::string & skipId,
  const std::string & skipBot, const std::string & text,
  const std::string & flags, const std::string & watches)
{
  std::string msg(skipId);
  msg += '@';
  msg += skipBot;
  msg += ' ';
  msg += flags;
  msg += ' ';
  msg += watches;
  msg += ' ';
  msg += text;
  msg += '\n';

  return this->sendCommand(from, "BROADCAST", msg);
}


int
Remote::sendNotice(const std::string & from, const std::string & clientId,
  const std::string & clientBot, const std::string & text)
{
  std::string msg(clientId);
  msg += ' ';
  msg += clientBot;
  msg += ' ';
  msg += text;
  msg += '\n';

  return this->sendCommand(from, "NOTICE", msg);
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

  return this->_sock.write(text + '\n');
}


int
Remote::sendChat(const std::string & from, const std::string & text)
{
  int result = 0;

  std::string cmd(":" + from + " CHAT " + text + '\n');

  if (this->ready())
  {
    result = this->_sock.write(cmd);
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
    result = this->_sock.write(cmd);
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
    result = this->_sock.write(cmd);
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
  return this->_sock.write(Remote::PROTOCOL_NAME + " " +
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
Remote::getLinks(BotClient * client, const std::string & prefix) const
{
  this->_children.getLinks(client, prefix);
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

int
Remote::sendCommand(const std::string & from, const std::string & to,
  const std::string & clientId, const std::string & command,
  const std::string & parameters)
{
  std::string msg(to);
  msg += ' ';
  msg += clientId;
  msg += ' ';
  msg += command;
  msg += ' ';
  msg += parameters;

  return this->sendCommand(from, "COMMAND", msg);
}

