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
  this->_sock.setBuffering(true);
  this->configureCallbacks();
}


Remote::Remote(BotSock *listener)
  : _stage(Remote::STAGE_INIT), _client(false), _sock(listener, false, true),
  _targetEstablished(false)
{
  this->_sock.setBuffering(true);
  this->configureCallbacks();
}


void
Remote::configureCallbacks(void)
{
  // BotSock callbacks
  this->_sock.registerOnConnectHandler(boost::bind(&Remote::onConnect, this));
  this->_sock.registerOnReadHandler(boost::bind(&Remote::onRead, this, _1));

  // Remote callbacks
  this->registerCommand("ERROR", &Remote::onError);
  this->registerCommand("AUTH", &Remote::onAuth);
  this->registerCommand("BOTJOIN", &Remote::onBotJoin);
  this->registerCommand("BOTPART", &Remote::onBotPart);
  this->registerCommand("CHAT", &Remote::onChat);
  this->registerCommand("COMMAND", &Remote::onCommand);
  this->registerCommand("BROADCAST", &Remote::onBroadcast);
  this->registerCommand("NOTICE", &Remote::onNotice);
}


void
Remote::registerCommand(const std::string & command,
  bool (Remote::*callback)(const std::string & from,
  const std::string & command, const StrVector & parameters))
{
  this->commands.insert(std::make_pair(::UpCase(command),
    boost::bind(callback, this, _1, _2, _3)));
}


void
Remote::unregisterCommand(const std::string & command)
{
  this->commands.erase(::UpCase(command));
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

    Log::Write("Connection initiated with host " +
      BotSock::inet_ntoa(this->_sock.getRemoteAddress()) + ":" +
      IntToStr(ntohs(this->_sock.getRemotePort())));

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
Remote::onAuth(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  bool result = false;

  if (2 == parameters.size())
  {
    std::string address = BotSock::inet_ntoa(this->_sock.getRemoteAddress());
    std::string hostname = this->getHostname();
    std::string handle = parameters[0];

    if (Config::AuthBot(handle, address, parameters[1]) ||
      (!hostname.empty() && Config::AuthBot(handle, hostname, parameters[1])))
    {
#ifdef REMOTE_DEBUG
      std::cout << "Authentication successful!" << std::endl;
#endif
      result = true;
      this->_handle = handle;
      this->_children.setName(handle);

      this->_stage = Remote::STAGE_AUTHED;
      this->unregisterCommand("AUTH");

      if (this->isServer())
      {
        this->sendAuth();
      }
      else
      {
        this->sendMyBotNet();
      }
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
    this->sendSyntaxError(command);
  }

  return result;
}


bool
Remote::parse(std::string text)
{
  bool result = true;

  StrVector parameters;
  std::string from;
  std::string command;

  if ((text.size() > 1) && (text[0] == ':'))
  {
    text = text.substr(1);

    from = FirstWord(text);
    command = ::UpCase(FirstWord(text));
  }
  else
  {
    command = ::UpCase(FirstWord(text));
  }
  SplitIRC(parameters, text);

  Remote::CommandMap::iterator pos = this->commands.find(command);

  if (pos != this->commands.end())
  {
    result = pos->second(from, pos->first, parameters);
  }
  else
  {
    this->sendUnknownCommand(command);
    result = false;
  }

  return result;
}


bool
Remote::onError(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  std::string handle(this->getHandle());

  std::string notice("*** Bot ");

  notice += handle.empty() ? "<unknown>" : handle;

  std::string log(notice);
  log += '[';
  log += this->getHostname();
  log += ']';

  if (parameters.size() > 0)
  {
    notice += " reports error: ";
    log += " reports error: ";
    notice += parameters[0];
    log += parameters[0];
  }
  else
  {
    notice += " reported an error.";
    log += " reported an error.";
  }
  Log::Write(log);
  clients.sendAll(notice, UserFlags::OPER);

  return false;
}


bool
Remote::onBotJoin(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  // <from> BOTJOIN <node>
  bool result = false;

  if (0 == parameters.size())
  {
    result = true;

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
      this->write(this->_sendQ);
      this->_sendQ = "";
    }
  }
  else if (1 == parameters.size())
  {
    std::string node(parameters[0]);

    if (remotes.isConnected(node))
    {
      this->sendError(node + " is already connected to botnet!");
    }
    else
    {
      result = true;

      this->_children.Link(from, node);
      remotes.sendBotJoin(from, node, this);

      if (this->ready())
      {
        clients.sendAll("*** Bot " + node + " has connected to " + from,
          UserFlags::OPER);
      }
    }
  }
  else
  {
    this->sendSyntaxError(command);
  }

  return result;
}


bool
Remote::onBotPart(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  // <from> BOTPART <node>
  bool result = false;

  if (1 == parameters.size())
  {
    std::string node(parameters[0]);

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
      this->sendUnknownCommand(command);
    }
  }
  else
  {
    this->sendSyntaxError(command);
  }

  return result;
}


bool
Remote::onChat(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  // <handle@bot> CHAT :<text>
  bool result = false;

  if (1 == parameters.size())
  {
    std::string text(parameters[0]);

    clients.sendChat(from, text);
    remotes.sendChat(from, text, this);

    result = true;
  }
  else
  {
    this->sendSyntaxError(command);
  }

  return result;
}


bool
Remote::onCommand(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  // <handle@bot> COMMAND <to> <id> :<command> <parameters>
  bool result = false;

  if (3 == parameters.size())
  {
    std::string::size_type at(from.find('@'));
    if (std::string::npos == at)
    {
      this->sendError("Only users can send remote commands!");
    }
    else
    {
      std::string handle(from.substr(0, at));
      std::string bot(from.substr(at + 1));

      std::string to(parameters[0]);
      std::string id(parameters[1]);
      std::string cmdline(parameters[2]);

      std::string cmd(FirstWord(cmdline));
      std::string args(cmdline);

      try
      {
        this->_clientHandle = handle;
        this->_clientBot = bot;
        this->_clientId = id;
        this->_clientFlags = Config::getRemoteFlags(from);

        this->_targetEstablished = true;
        if (Same(to, Config::GetNick()))
        {
          try
          {
            this->_parser.parse(this, cmd, args);
          }
          catch (CommandParser::exception & e)
          {
            this->send(e.what());
          }
        }
        else
        {
	  remotes.sendRemoteCommand(this, to, cmd, args);
        }
        this->_targetEstablished = false;

	result = true;
      }
      catch (...)
      {
        _targetEstablished = false;
        throw;
      }
    }
  }
  else
  {
    this->sendSyntaxError(command);
  }

  return result;
}


bool
Remote::onBroadcast(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  // <from> BROADCAST <skip> <flags> <watches> :<text>
  bool result = false;

  if (4 == parameters.size())
  {
    std::string skip(parameters[0]);
    std::string flags(parameters[1]);
    std::string watches(parameters[2]);
    std::string text(parameters[3]);

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

      result = true;
    }
    else if (0 == skip.compare("*"))
    {
      clients.sendAll(from, text, flags, watches);
      remotes.sendBroadcastPtr(from, this, text, flags, watches);

      result = true;
    }
    else
    {
      this->sendError("Malformed skip ID in broadcast!");
    }
  }
  else
  {
    this->sendSyntaxError(command);
  }

  return result;
}


bool
Remote::onNotice(const std::string & from, const std::string & command,
  const StrVector & parameters)
{
  // <from> NOTICE <clientId> <clientBot> :<text>
  bool result = false;

  if (3 == parameters.size())
  {
    std::string clientId(parameters[0]);
    std::string clientBot(parameters[1]);
    std::string text(parameters[2]);

    remotes.sendNotice(from, clientId, clientBot, text);

    result = true;
  }
  else
  {
    this->sendSyntaxError(command);
  }

  return result;
}


int
Remote::sendBroadcast(const std::string & from, const std::string & text,
  const std::string & flags, const std::string & watches)
{
  std::string msg("* ");
  msg += flags;
  msg += ' ';
  msg += watches;
  msg += " :";
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
  msg += " :";
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
  msg += " :";
  msg += text;
  msg += '\n';

  return this->sendCommand(from, "NOTICE", msg);
}


int
Remote::sendError(const std::string & text)
{
  int result = this->sendCommand("", "ERROR", ":" + text, false);

  std::string handle(this->getHandle());

  std::string notice("*** Error on link with ");
  notice += (handle.empty()) ? "<unknown>" : handle;
  std::string log(notice);
  notice += ": ";
  notice += text;

  log += '[';
  log += this->getHostname();
  log += "]: ";
  log += text;

  clients.sendAll(notice, UserFlags::OPER);
  Log::Write(log);

  return result;
}


int
Remote::sendUnknownCommand(const std::string & command)
{
  std::cerr << "Unknown remote command: " << command << std::endl;
  return this->sendError("Unknown command: " + command);
}


int
Remote::sendSyntaxError(const std::string & command)
{
  std::cerr << "Syntax error: " << command << std::endl;
  return this->sendError("Syntax error: " + command);
}


int
Remote::sendCommand(const std::string & from, const std::string & command,
  const std::string & parameters, const bool queue)
{
  int result = 0;

  std::string text;
  if (!from.empty())
  {
    text += ':';
    text += from;
    text += ' ';
  }
  text += command;
  if (!parameters.empty())
  {
    text += ' ';
    text += parameters;
  }
  text += '\n';

  if (!queue || this->ready())
  {
    result = this->write(text);
  }
  else
  {
    this->_sendQ += text;
  }

  return result;
}


int
Remote::sendChat(const std::string & from, const std::string & text)
{
  return this->sendCommand(from, "CHAT", ":" + text);
}


int
Remote::sendBotJoin(const std::string & oldnode, const std::string & newnode)
{
  return this->sendCommand(oldnode, "BOTJOIN", newnode);
}


int
Remote::sendBotPart(const std::string & from, const std::string & node)
{
  return this->sendCommand(from, "BOTPART", node);
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
    result += this->sendCommand(pos->nodeA, "BOTJOIN", pos->nodeB, false);
  }
  this->sendCommand(Config::GetNick(), "BOTJOIN", "", false);

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

  return this->sendCommand("", "AUTH", Config::GetNick() + " :" + password,
    false);
}


int
Remote::write(const std::string & text)
{
#ifdef REMOTE_DEBUG
  std::cout << "Remote << " << text;
#endif
  return this->_sock.write(text);
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
Remote::sendRemoteCommand(const std::string & from, const std::string & to,
  const std::string & clientId, const std::string & command,
  const std::string & parameters)
{
  std::string msg(to);
  msg += ' ';
  msg += clientId;
  msg += " :";
  msg += command;
  msg += ' ';
  msg += parameters;

  return this->sendCommand(from, "COMMAND", msg);
}

