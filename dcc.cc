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


#ifdef DEBUG
# define DCC_DEBUG
#endif


DCC::DCC(const std::string & nick, const std::string & userhost)
  : BotSock(), client(new DCC::Client(this))
{
  this->setBuffering(true);

  this->echoMyChatter = false;
  this->UserHost = userhost;
  this->Nick = nick;

  this->watches = WatchSet::defaults();

  CommandParser::CommandFunctorPtr
    authCmd(new DCC::CommandFunctor(this, &DCC::cmdAuth));
  parser.addCommand("AUTH", authCmd, UF_NONE);

  CommandParser::CommandFunctorPtr
    quitCmd(new DCC::CommandFunctor(this, &DCC::cmdQuit));
  parser.addCommand("QUIT", quitCmd, UF_NONE, CommandParser::EXACT_ONLY);

  CommandParser::CommandFunctorPtr
    echoCmd(new DCC::CommandFunctor(this, &DCC::cmdEcho));
  parser.addCommand("ECHO", echoCmd, UF_NONE);

  CommandParser::CommandFunctorPtr
    watchCmd(new DCC::CommandFunctor(this, &DCC::cmdWatch));
  parser.addCommand("WATCH", watchCmd, UF_AUTHED);

  CommandParser::CommandFunctorPtr
    chatCmd(new DCC::CommandFunctor(this, &DCC::cmdChat));
  parser.addCommand("CHAT", chatCmd, UF_NONE);
}


DCC::DCC(DCC *listener, const std::string & nick, const std::string & userhost)
  : BotSock(listener), parser(listener->parser), client(listener->client)
{
  this->setBuffering(true);

  this->echoMyChatter = listener->echoMyChatter;
  this->UserHost = listener->UserHost;
  this->Nick = listener->Nick;
  this->watches = listener->watches;
}


bool
DCC::connect(const BotSock::Address address, const BotSock::Port port,
  const std::string nick, const std::string userhost)
{
  this->Nick = nick;
  this->UserHost = userhost;

  return this->BotSock::connect(address, port);
}


bool
DCC::listen(const std::string nick, const std::string userhost,
  const BotSock::Port port, const int backlog)
{
  this->Nick = nick;
  this->UserHost = userhost;

  this->setTimeout(60);
  return this->BotSock::listen(port, backlog);
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
  clients.sendAll(this->getUserhost() + " has connected", UF_AUTHED,
    WatchSet(), this->client);
  this->motd();
  this->setTimeout(DCC_IDLE_MAX);

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
        if ((this->client->flags() & UF_REMOTE) == 0)
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
          parser.parse(this->client, command, text);
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
	remotes.sendCommand(this->client, to, command, text);
      }
    }
    else if (text != "Unknown command" ||
        !vars[VAR_IGNORE_UNKNOWN_COMMAND]->getBool())
    {
      // Chat
      this->cmdChat(this->client, "CHAT", text);
    }
  }

  return result;
}


void
DCC::cmdAuth(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string authHandle = FirstWord(parameters);
  std::string handle;
  int flags;

  if (Config::Auth(authHandle, this->UserHost, parameters, flags,
    handle))
  {
    this->client->handle(handle);
    this->client->flags(flags);
    this->setTimeout(0);
    this->send("*** Authorization granted!");

    std::string notice(this->client->handle() + " (" + this->UserHost +
      ") has been authorized");
    ::SendAll(notice, UF_AUTHED, WatchSet(), this->client);
    Log::Write(notice);

    this->loadConfig();
  }
  else
  {
    this->send("*** Authorization denied!");
    ::SendAll(this->UserHost + " has failed authorization!", UF_AUTHED,
      WatchSet(), this->client);
    Log::Write(this->UserHost + " has failed authorization!");
  }
}


void
DCC::cmdQuit(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  throw DCC::quit("*** Client issued QUIT command.");
}


// chat(text)
//
// Handles any text which should be interpretted as chatter.
//
void
DCC::cmdChat(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  if ((from->flags() & UF_AUTHED) || vars[VAR_UNAUTHED_MAY_CHAT]->getBool())
  {
    if (this->echoMyChatter)
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
  StrList output;
  ::motd(output);
  this->send(output);
}


// getFlags()
//
// Returns a fixed length string containing the user's flags
//
std::string
DCC::getFlags(void) const
{
  std::string temp = "";
  if (this->isAuthed())
  {
    int flags = this->client->flags();

    if (flags & UF_CHANOP) temp += "C"; else temp += " ";
    if (flags & UF_DLINE) temp += "D"; else temp += " ";
    if (flags & UF_GLINE) temp += "G"; else temp += " ";
    if (flags & UF_KLINE) temp += "K"; else temp += " ";
    if (flags & UF_MASTER) temp += "M"; else temp += " ";
    if (flags & UF_NICK) temp += "N"; else temp += " ";
    if (flags & UF_OPER) temp += "O"; else temp += " ";
    if (flags & UF_REMOTE) temp += "R"; else temp += " ";
    if (flags & UF_WALLOPS) temp += "W"; else temp += " ";
    if (flags & UF_CONN) temp += "X"; else temp += " ";
  }
  else
  {
    temp = "-         ";
  }
  return temp;
}


void
DCC::send(const std::string & message, const int flags,
  const WatchSet & watches)
{
  if (this->isConnected() &&
    ((flags == UF_NONE) || (flags == (this->client->flags() & flags))))
  {
    if (!this->watches.has(watches))
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
    this->write(message + "\n");
  }
  else
  {
#ifdef DCC_DEBUG
    std::cout << "DCC::send(): DCC not connected or lacks required flags" <<
      std::endl;
#endif
  }
}


// send(strings, flags, watches)
//
// Sends a list of strings to the DCC connection
//
void
DCC::send(StrList & messages, const int flags, const WatchSet & watches)
{
  for (StrList::iterator pos = messages.begin(); pos != messages.end(); ++pos)
  {
    this->send((*pos), flags, watches);
  }
}


void
DCC::cmdWatch(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  if (parameters == "")
  {
    this->send("Watching: " + WatchSet::getWatchNames(this->watches, false));
  }
  else
  {
    StrList output;

    this->watches.set(output, parameters);

    this->send(output);

    userConfig->setWatches(this->client->handle(),
      WatchSet::getWatchNames(this->watches, true));
  }
}


void
DCC::cmdEcho(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string parm = UpCase(FirstWord(parameters));

  try
  {
    if (parm == "")
    {
      this->send(std::string("*** ECHO is ") +
        (this->echoMyChatter ? "ON" : "OFF"));
    }
    else if (parm == "ON")
    {
      this->echoMyChatter = true;
      this->send("*** ECHO is now ON.");
      if (this->isAuthed())
      {
        userConfig->setEcho(this->client->handle(), true);
      }
    }
    else if (parm == "OFF")
    {
      this->echoMyChatter = false;
      this->send("*** ECHO is now OFF.");
      if (this->isAuthed())
      {
        userConfig->setEcho(this->client->handle(), false);
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
    this->echoMyChatter = userConfig->getEcho(this->client->handle());
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
    StrList output;
    this->watches.set(output, userConfig->getWatches(this->client->handle()));
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

