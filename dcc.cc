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

// Std C++ Headers
#include <iostream>
#include <string>

// Boost C++ Headers
#include <boost/bind.hpp>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "dcc.h"
#include "dcclist.h"
#include "botexcept.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "userdb.h"
#include "vars.h"
#include "remotelist.h"
#include "arglist.h"
#include "cmdparser.h"
#include "help.h"


#ifdef DEBUG
# define DCC_DEBUG
#endif


DCC::DCC(const std::string & nick, const std::string & userhost)
  : _sock(), _flags(UserFlags::NONE())
{
  this->_sock.setBuffering(true);
  this->_sock.registerOnConnectHandler(boost::bind(&DCC::onConnect, this));
  this->_sock.registerOnReadHandler(boost::bind(&DCC::onRead, this, _1));

  this->_echoMyChatter = false;
  this->_userhost = userhost;
  this->_nick = nick;

  this->_watches = WatchSet::defaults();

  this->_id = ptrToStr(this);

  // anonymous commands
  this->addCommand("AUTH", &DCC::cmdAuth, UserFlags::NONE());
  this->addCommand("HELP", &DCC::cmdHelp, UserFlags::NONE());
  this->addCommand("INFO", &DCC::cmdHelp, UserFlags::NONE());
  this->addCommand("ECHO", &DCC::cmdEcho, UserFlags::NONE());
  this->addCommand("CHAT", &DCC::cmdChat, UserFlags::NONE());
  this->addCommand("QUIT", &DCC::cmdQuit, UserFlags::NONE(),
    CommandParser::EXACT_ONLY);

  // UserFlags::AUTHED commands
  this->addCommand("WATCH", &DCC::cmdWatch, UserFlags::AUTHED);

  // UserFlags::WALLOPS commands
  this->addCommand("LOCOPS", &DCC::cmdLocops, UserFlags::WALLOPS);
}


DCC::DCC(DCC *listener, const std::string & nick, const std::string & userhost)
  : _sock(listener), _parser(listener->_parser)
{
  this->_sock.setBuffering(true);
  this->_sock.registerOnConnectHandler(boost::bind(&DCC::onConnect, this));
  this->_sock.registerOnReadHandler(boost::bind(&DCC::onRead, this, _1));

  this->_echoMyChatter = listener->_echoMyChatter;
  this->_userhost = listener->_userhost;
  this->_nick = listener->_nick;
  this->_watches = listener->_watches;
  this->_id = ptrToStr(this);
}


void
DCC::addCommand(const std::string & command,
  void (DCC::*func)(BotClient *from, const std::string & command,
  std::string parameters), const UserFlags flags, const int options)
{
  this->_parser.addCommand(command, boost::bind(func, this, _1, _2, _3), flags,
    options);
}


bool
DCC::connect(const BotSock::Address address, const BotSock::Port port,
  const std::string nick, const std::string userhost)
{
  this->_nick = nick;
  this->_userhost = userhost;

  return this->_sock.connect(address, port);
}


bool
DCC::listen(const std::string nick, const std::string userhost,
  const BotSock::Port port, const int backlog)
{
  this->_nick = nick;
  this->_userhost = userhost;

  this->_sock.setTimeout(60);
  return this->_sock.listen(port, backlog);
}


// onConnect()
//
// What to do when we connect via DCC with someone
//
// Returns true if no errors ocurred
//
bool
DCC::onConnect(void)
{
#ifdef DCC_DEBUG
  std::cout << "DCC::onConnect()" << std::endl;
#endif
  this->send("OOMon-" + std::string(OOMON_VERSION) + " by Timothy L. Jensen");
  clients.sendAll(this->userhost() + " has connected", UserFlags::AUTHED,
    WatchSet(), this);
  this->motd();
  this->_sock.setTimeout(DCC_IDLE_MAX);

  return true;
}


// onRead()
//
// Process incoming data from a DCC connection
//
// Returns true if no errors ocurred
//
bool
DCC::onRead(std::string text)
{
#ifdef DCC_DEBUG
  std::cout << "DCC >> " << text << std::endl;
#endif

  return this->parse(text);
}


// parse(text)
//
// This is where we actually parse the data received. Fun stuff.
//
// Returns true if the DCC connection should be left open.
//
bool
DCC::parse(std::string text)
{
  bool result = true;

  if (!text.empty())
  {
    std::string command;
    std::string to;

    if (text[0] == '.')				// A command?
    {
      // Command
      text.erase((std::string::size_type) 0, 1);
      command = FirstWord(text);

      // Is this a remote command?
      std::string::size_type at = command.find('@');
      if (at != std::string::npos)
      {
	// Yes! Do I have permission?
        if (!this->flags().has(UserFlags::REMOTE))
        {
          this->send("*** You can't use remote commands!");
	  return true;
        }

        to = command.substr(at + 1);
        command = command.substr(0, at);
      }

      if (to.empty() || Same(to, Config::GetNick()))
      {
        try
        {
          this->_parser.parse(this, command, text);
        }
        catch (CommandParser::exception & e)
        {
          this->send(e.what());
        }
        catch (DCC::quit & q)
        {
          result = false;
        }
      }
      else
      {
	// ISSUE REMOTE COMMAND
	remotes.sendCommand(this, to, command, text);
      }
    }
    else if (text != "Unknown command" ||
        !vars[VAR_IGNORE_UNKNOWN_COMMAND]->getBool())
    {
      // Chat
      this->cmdChat(this, "CHAT", text);
    }
  }

  return result;
}


void
DCC::cmdAuth(BotClient *from, const std::string & command,
  std::string parameters)
{
  std::string authHandle = FirstWord(parameters);
  std::string handle;
  UserFlags flags;

  if (Config::Auth(authHandle, this->_userhost, parameters, flags,
    handle))
  {
    this->handle(handle);
    this->flags(flags);
    this->_sock.setTimeout(0);
    this->send("*** Authorization granted!");

    std::string notice(this->handle() + " (" + this->userhost() +
      ") has been authorized");
    ::SendAll(notice, UserFlags::AUTHED, WatchSet(), this);
    Log::Write(notice);

    this->loadConfig();
  }
  else
  {
    this->send("*** Authorization denied!");
    ::SendAll(this->userhost() + " has failed authorization!",
      UserFlags::AUTHED, WatchSet(), this);
    Log::Write(this->userhost() + " has failed authorization!");
  }
}


void
DCC::cmdQuit(BotClient *from, const std::string & command,
  std::string parameters)
{
  throw DCC::quit("*** Client issued QUIT command.");
}


// chat(text)
//
// Handles any text which should be interpretted as chatter.
//
void
DCC::cmdChat(BotClient *from, const std::string & command,
  std::string parameters)
{
  if (from->flags().has(UserFlags::AUTHED) ||
    vars[VAR_UNAUTHED_MAY_CHAT]->getBool())
  {
    if (this->_echoMyChatter)
    {
      clients.sendChat(from->handleAndBot(), parameters);
    }
    else
    {
      clients.sendChat(from->handleAndBot(), parameters, from);
    }
    remotes.sendChat(from->handleAndBot(), parameters);
  }
  else
  {
    from->send("*** You must authenticate yourself with \".auth\" before you can chat!");
  }
}


// motd()
//
// Writes the Message Of The Day
//
void
DCC::motd(void)
{
  ::motd(this);
}


// humanReadableFlags()
//
// Returns a fixed length string containing the user's flags
//
std::string
DCC::humanReadableFlags(void) const
{
  std::string temp = "";
  if (this->isAuthed())
  {
    UserFlags flags = this->flags();

    if (flags.has(UserFlags::CHANOP)) temp += "C"; else temp += " ";
    if (flags.has(UserFlags::DLINE)) temp += "D"; else temp += " ";
    if (flags.has(UserFlags::GLINE)) temp += "G"; else temp += " ";
    if (flags.has(UserFlags::KLINE)) temp += "K"; else temp += " ";
    if (flags.has(UserFlags::MASTER)) temp += "M"; else temp += " ";
    if (flags.has(UserFlags::NICK)) temp += "N"; else temp += " ";
    if (flags.has(UserFlags::OPER)) temp += "O"; else temp += " ";
    if (flags.has(UserFlags::REMOTE)) temp += "R"; else temp += " ";
    if (flags.has(UserFlags::WALLOPS)) temp += "W"; else temp += " ";
    if (flags.has(UserFlags::CONN)) temp += "X"; else temp += " ";
  }
  else
  {
    temp = "-         ";
  }
  return temp;
}


void
DCC::send(const std::string & message)
{
  if (this->isConnected())
  {
    this->_sock.write(message + '\n');
  }
}


void
DCC::send(const std::string & message, const UserFlags flags,
  const WatchSet & watches)
{
  if ((flags == UserFlags::NONE()) || (flags == (this->flags() & flags)))
  {
    if (!this->_watches.has(watches))
    {
#ifdef DCC_DEBUG
      std::cout << "DCC client not watching this type of message" << std::endl;
#endif
      // Missing watch...don't display status message
      return;
    }
#ifdef DCC_DEBUG
    std::cout << "DCC << " << message << std::endl;
#endif
    this->send(message);
  }
  else
  {
#ifdef DCC_DEBUG
    std::cout << "DCC::send(): DCC not connected or lacks required flags" <<
      std::endl;
#endif
  }
}


void
DCC::who(BotClient * client) const
{
  std::string text(this->humanReadableFlags());

  text += ' ';
  text += this->handle();
  text += " (";
  text += this->userhost();

  if (this->isConnected())
  {
      text += ") ";
      text += IntToStr(this->idleTime());
  }
  else
  {
      text += ") -connecting-";
  }

  client->send(text);
}


bool
DCC::statsP(StrList & output) const
{
  bool countMe = false;

  if (this->isConnected() && this->isOper())
  {
    std::string notice(this->handle());

    if (vars[VAR_STATSP_SHOW_USERHOST]->getBool())
    {
      notice += " (";
      notice += this->userhost();
      notice += ')';
    }
    if (vars[VAR_STATSP_SHOW_IDLE]->getBool())
    {
      notice += " Idle: ";
      notice += IntToStr(this->idleTime());
    }

    output.push_back(notice);
    countMe = true;
  }

  return countMe;
}


void
DCC::cmdWatch(BotClient *from, const std::string & command,
  std::string parameters)
{
  if (parameters == "")
  {
    from->send("Watching: " + WatchSet::getWatchNames(this->_watches, false));
  }
  else
  {
    this->_watches.set(from, parameters);

    userConfig->setWatches(this->handle(),
      WatchSet::getWatchNames(this->_watches, true));
  }
}


void
DCC::cmdEcho(BotClient *from, const std::string & command,
  std::string parameters)
{
  std::string parm = UpCase(FirstWord(parameters));

  try
  {
    if (parm == "")
    {
      this->send(std::string("*** ECHO is ") +
        (this->_echoMyChatter ? "ON" : "OFF"));
    }
    else if (parm == "ON")
    {
      this->_echoMyChatter = true;
      this->send("*** ECHO is now ON.");
      if (this->isAuthed())
      {
        userConfig->setEcho(this->handle(), true);
      }
    }
    else if (parm == "OFF")
    {
      this->_echoMyChatter = false;
      this->send("*** ECHO is now OFF.");
      if (this->isAuthed())
      {
        userConfig->setEcho(this->handle(), false);
      }
    }
    else
    {
      this->send("*** Huh?  Turn echo ON or OFF?");
    }
  }
  catch (OOMon::botdb_error & e)
  {
    Log::Write("Error in DCC::doEcho(): " + e.why());
  }
}


void
DCC::loadConfig(void)
{
  try
  {
    this->_echoMyChatter = userConfig->getEcho(this->handle());
  }
  catch (OOMon::norecord_error & e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch (OOMon::botdb_error & e)
  {
    std::cerr << e.what() << std::endl;
  }

  try
  {
    this->_watches.set(this, userConfig->getWatches(this->handle()), false);
  }
  catch (OOMon::norecord_error & e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch (OOMon::botdb_error & e)
  {
    std::cerr << e.what() << std::endl;
  }
}


void
DCC::cmdHelp(BotClient *from, const std::string & command,
  std::string parameters)
{
  std::string topic;

  if (command == "info")
  {
    topic = parameters.empty() ? "info" : ("info " + parameters);
  }
  else
  {
    topic = parameters;
  }


  Help::getHelp(from, topic);
}


void
DCC::cmdLocops(BotClient *from, const std::string & command,
  std::string parameters)
{
  if (!parameters.empty())
  {
    server.locops("(" + from->handle() + ") " + parameters);
  }
  else
  {
    CommandParser::syntax(command, "<text>");
  }
}

