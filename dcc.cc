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

// Std C++ Headers
#include <iostream>
#include <fstream>
#include <string>
#include <list>

// Std C Headers

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "dcc.h"
#include "dcclist.h"
#include "botexcept.h"
#include "config.h"
#include "log.h"
#include "irc.h"
#include "util.h"
#include "main.h"
#include "engine.h"
#include "help.h"
#include "trap.h"
#include "userdb.h"
#include "proxy.h"
#include "seedrand.h"
#include "vars.h"
#include "userhash.h"
#include "remotelist.h"
#include "arglist.h"


#ifdef DEBUG
# define DCC_DEBUG
#endif


DCC::DCC(const std::string & nick, const std::string & userhost)
  : BotSock()
{
  this->setBuffering(true);

  this->echoMyChatter = false;
  this->UserHost = userhost;
  this->Nick = nick;
  this->Flags = UF_NONE;

  this->watches = WatchSet::defaults();
}


DCC::DCC(DCC *listener, const std::string & nick, const std::string & userhost)
  : BotSock(listener)
{
  this->setBuffering(true);

  this->echoMyChatter = listener->echoMyChatter;
  this->UserHost = listener->UserHost;
  this->Nick = listener->Nick;
  this->Flags = listener->Flags;
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
DCC::onConnect()
{
#ifdef DCC_DEBUG
  std::cout << "DCC::onConnect()" << std::endl;
#endif
  this->send("OOMon-" + std::string(OOMON_VERSION) + " by Timothy L. Jensen");
  clients.sendAll(this->getUserhost() + " has connected", UF_AUTHED,
    WatchSet(), this);
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
  std::string Command, To;

  if (text == "")
    return true;

  if (text[0] == '.')				// A command?
  {
    // Command
    text.erase((std::string::size_type) 0, 1);
    Command = FirstWord(text);

    // Is this a remote command?
    std::string::size_type AtSign = Command.find('@');
    if (AtSign != std::string::npos)
    {
      if ((Flags & UF_REMOTE) == 0)  		// Yes! Do I have permission?
      {
        this->send("*** You can't use remote commands!");
	return true;
      }
      To = Command.substr(AtSign + 1);
      Command = Command.substr(0, AtSign);
      #ifdef DCC_DEBUG
	std::cout << "Remote command for " << To << ": " << Command <<
	  std::endl;
      #endif
    }
    else
    {
      To = "";
#ifdef DCC_DEBUG
      std::cout << "Local command: " << Command << std::endl;
#endif
    }
    if (To != "")
    {
      if (Same(To, Config::GetNick())) 	// No need to route to myself!
      {
        To = "";
      }
      else
      {
	if (!remotes.isConnected(To))
	{
	  this->send("*** Bot not linked: " + To);
	  return true;
        }
      }
    }

    // This is a bit messy...
    switch(DCC::getCommand(UpCase(Command))) {
    case DCC_AUTH:
      if (To == "")
      {
	std::string Parm1 = FirstWord(text);
	if (Config::Auth(Parm1, UserHost, text, Flags, Handle))
	{
	  this->setTimeout(0);
	  this->send("*** Authorization granted!");
	  ::SendAll(Handle + " (" + UserHost + ") has been authorized",
	    UF_AUTHED, WatchSet(), this);
	  Log::Write(Handle + " (" + UserHost + ") has been authorized");

          this->loadConfig();
	}
	else
	{
	  this->send("*** Authorization denied!");
	  ::SendAll(UserHost + " has failed authorization!", UF_AUTHED,
	    WatchSet(), this);
	  Log::Write(UserHost + " has failed authorization!");
	}
      }
      break;
    case DCC_QUIT:
      if (To == "")
        return false;
      break;
    case DCC_CHAT:
      if (To == "")
      {
	this->chat(text);
      }
      break;
    case DCC_WHO:
      if (this->isAuthed())
      {
        if (To == "")
        {
          StrList Output;
	  clients.who(Output);
	  this->send(Output);
        }
      }
      else
      {
	this->notAuthed();
      }
      break;
    case DCC_LINKS:
      if (this->isAuthed())
      {
        if (To == "")
        {
          StrList Output;
	  remotes.getLinks(Output);
	  this->send(Output);
        }
      }
      else
      {
	this->notAuthed();
      }
      break;
    case DCC_HELP:
      if (To == "")
        this->help(text);
      break;
    case DCC_INFO:
      if (To == "")
      {
	std::string topic = FirstWord(text);

	if (topic.length() > 0)
	{
          this->help("info " + topic);
	}
	else
	{
          this->help("info");
	}
      }
      break;
    case DCC_WATCH:
      this->watch(To, text);
      break;
    case DCC_ECHO:
      {
	std::string Parm = UpCase(FirstWord(text));
	this->doEcho(Parm);
      }
      break;
    case DCC_OP:
      if (Flags & UF_CHANOP) {
	std::string Parm1 = FirstWord(text);
	std::string Parm2 = FirstWord(text);
	if (Parm1 != "")
	  if (Parm2 != "")
	    server.op(Parm1, Parm2);
	  else
	    server.op(Parm1, Nick);
	else
	  this->send("*** Please include a channel");
      } else
	this->noAccess();
      break;
    case DCC_JOIN:
      if (Flags & UF_CHANOP) {
	std::string Parm1 = FirstWord(text);
	std::string Parm2 = FirstWord(text);
	if (Parm1 != "")
	  server.join(Parm1, Parm2);
	else
	  this->send("*** Please include a channel");
      } else
	this->noAccess();
      break;
    case DCC_PART:
      if (Flags & UF_CHANOP) {
	std::string Parm1 = FirstWord(text);
	if (Parm1 != "")
	  server.part(Parm1);
	else
	  this->send("*** Please include a channel");
      } else
	this->noAccess();
      break;
    case DCC_KLINE:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (text != "")
	  {
	    if (To == "")
	    {
	      this->kline(Minutes, Parm1, text);
            }
	  }
	  else
	  {
	    this->send("*** Please include a kline reason");
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_UNKLINE:
      if (Flags & UF_KLINE)
      {
	std::string Parm1 = FirstWord(text);
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->unkline(Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a user@host mask to unkline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KCLONE:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kClone(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KFLOOD:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kFlood(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KBOT:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kBot(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KLINK:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kLink(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KSPAM:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kSpam(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KTRACE:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kTrace(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KMOTD:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kMotd(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KINFO:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kInfo(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KPROXY:
      if (Flags & UF_KLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->kProxy(Minutes, Parm1);
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KPERM:
      if (Flags & UF_KLINE)
      {
	std::string Parm1 = FirstWord(text);
	if (Parm1 != "")
	{
	  if (text != "")
	  {
	    if (To == "")
	    {
	      this->kPerm(Parm1, text);
	    }
	  }
	  else
	  {
	    this->send("*** Please include a kline reason");
	  }
	}
	else
	{
	  this->send("*** Please include a nick or user@host mask to kline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_DLINE:
      if (Flags & UF_DLINE)
      {
	unsigned int Minutes = vars[VAR_DEFAULT_DLINE_TIMEOUT]->getInt();
	std::string Parm1 = FirstWord(text);
	if (isNumeric(Parm1))
	{
	  Minutes = atoi(Parm1.c_str());
	  Parm1 = FirstWord(text);
	}
	if (Parm1 != "")
	{
	  if (text != "")
	  {
	    if (To == "")
	    {
	      this->dline(Minutes, Parm1, text);
	    }
	  }
	  else
	  {
	    this->send("*** Please include a dline reason");
	  }
	}
	else
	{
	  this->send("*** Please include a nick or IP mask to dline");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_UNDLINE:
      if (Flags & UF_DLINE)
      {
	std::string Parm1 = FirstWord(text);
	if (Parm1 != "")
	{
	  if (To == "")
	  {
	    this->undline(Parm1);
	  }
	}
	else
	  this->send("*** Please include a user@host mask to undline");
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_GLINE:
      if (Flags & UF_GLINE)
      {
        // No longer implemented.
	this->send("*** I must be confused!");
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_UNGLINE:
      if (Flags & UF_GLINE)
      {
        // No longer implemented.
	this->send("*** I must be confused!");
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_KILL:
      if (Flags & UF_OPER) {
	if (To == "") {
	  std::string Parm1 = FirstWord(text);
	  if (Parm1 != "") {
	    if (text != "") {
	      ::SendAll("KILL " + Parm1 + " ((" + this->getHandle() + ") " +
		text + ")", UF_OPER, WATCH_KILLS, this);
	      server.kill(this->getHandle(), Parm1, text);
	    } else
	      this->send("*** Please include a kill reason.");
	  } else
	    this->send("*** Please include a nick to kill.");
	}
      } else
	this->noAccess();
      break;
    case DCC_KILLLIST:
      this->killList(To, text);
      break;
    case DCC_KILLNFIND:
      this->killNFind(To, text);
      break;
    case DCC_MULTI:
      if (Flags & UF_OPER) {
	if (To == "") {
	  std::string Parm1 = FirstWord(text);
	  int min = atoi(Parm1.c_str());
	  StrList Output;
	  users.reportMulti(Output, min);
	  this->send(Output);
	}
      } else
	this->noAccess();
      break;
    case DCC_HMULTI:
      if (Flags & UF_OPER) {
	if (To == "") {
	  std::string Parm1 = FirstWord(text);
	  int min = atoi(Parm1.c_str());
	  StrList Output;
	  users.reportHMulti(Output, min);
	  this->send(Output);
	}
      } else
	this->noAccess();
      break;
    case DCC_UMULTI:
      if (Flags & UF_OPER) {
	if (To == "") {
	  std::string Parm1 = FirstWord(text);
	  int min = atoi(Parm1.c_str());
	  StrList Output;
	  users.reportUMulti(Output, min);
	  this->send(Output);
	}
      } else
	this->noAccess();
      break;
    case DCC_VMULTI:
      if (Flags & UF_OPER) {
	if (To == "") {
	  std::string Parm1 = FirstWord(text);
	  int min = atoi(Parm1.c_str());
	  StrList Output;
	  users.reportVMulti(Output, min);
	  this->send(Output);
	}
      } else
	this->noAccess();
      break;
    case DCC_NFIND:
      this->nFind(To, text);
      break;
    case DCC_TRACE:
      if (Flags & UF_OPER) {
	if (To == "") {
	  server.retrace(this->getHandle());
	}
      } else
	this->noAccess();
      break;
    case DCC_CLONES:
      if (Flags & UF_OPER) {
	if (To == "") {
	  StrList Output;
	  users.reportClones(Output);
	  this->send(Output);
	}
      } else
	this->noAccess();
      break;
    case DCC_LIST:
      this->list(To, text);
      break;
    case DCC_FINDK:
      this->findK(To, text);
      break;
    case DCC_FINDD:
      this->findD(To, text);
      break;
    case DCC_RELOAD:
      if (Flags & UF_OPER) {
	if (To == "") {
	  std::string Parm1 = UpCase(FirstWord(text));
	  if (Parm1 == "USERS") {
	    server.retrace(this->getHandle());
	  } else if (Parm1 == "KLINES") {
	    server.reloadKlines(this->getHandle());
	  } else if (Parm1 == "DLINES") {
	    server.reloadDlines(this->getHandle());
	  } else if (Parm1 == "CONFIG") {
	    ReloadConfig(this->getHandle());
	  } else if (Parm1 == "HELP") {
	    Help::flush();
	  } else
	    this->send("*** Valid options are USERS, KLINES, CONFIG, and HELP");
	}
      } else
	this->noAccess();
      break;
    case DCC_DOMAINS:
      if (Flags & UF_OPER) {
	if (To == "") {
          std::string Parm1 = FirstWord(text);
	  int min = 5;
	  if (Parm1 != "")
	    min = atoi(Parm1.c_str());
	  if (min >= 1)
	  {
	    StrList Output;
	    users.reportDomains(Output, min);
	    this->send(Output);
	  } else
	    this->send("*** Invalid minimum user size!");
	}
      } else
	this->noAccess();
      break;
    case DCC_NETS:
      if (Flags & UF_OPER) {
	if (To == "") {
          std::string Parm1 = FirstWord(text);
	  int min = 5;
	  if (Parm1 != "")
	    min = atoi(Parm1.c_str());
	  if (min >= 1)
	  {
	    StrList Output;
	    users.reportNets(Output, min);
	    this->send(Output);
	  } else
	    this->send("*** Invalid minimum user size!");
	}
      } else
	this->noAccess();
      break;
    case DCC_CLASS:
      if (Flags & UF_OPER) {
	if (To == "") {
          std::string className = FirstWord(text);
	  StrList Output;
	  users.reportClasses(Output, className);
	  this->send(Output);
	}
      } else
	this->noAccess();
      break;
    case DCC_MOTD:
      if (this->isAuthed())
      {
	if (To == "")
	{
	  this->motd();
	}
      }
      else
      {
	this->notAuthed();
      }
      break;
    case DCC_CONN:
      if (Flags & UF_MASTER)
      {
	std::string Parm1 = FirstWord(text);
	if (Parm1 != "")
	{
	  remotes.conn(Handle, To, Parm1);
	}
	else
	{
	  this->send("*** Please specify the bot to connect");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_DISCONN:
      if (Flags & UF_MASTER)
      {
	std::string Parm1 = FirstWord(text);
	if (Parm1 != "")
	{
	  remotes.disconn(Handle, To, Parm1);
	}
	else
	{
	  this->send("*** Please specify the bot to disconnect");
	}
      }
      else
      {
	this->noAccess();
      }
      break;
    case DCC_DIE:
      if (Flags & UF_MASTER) {
	if (To == "")
	{
	  this->send("As you wish...");
	  if (text.length() > 0)
	  {
	    ::SendAll(this->getHandle() + " says I have to die now... (" +
	      text + ")", UF_NONE, WatchSet(), this);
	    Log::Write("DIE requested by " + this->getHandle() + " (" +
	      text + ")");
            if (text.length() > 0)
            {
              server.quit(text);
            }
	  }
	  else
	  {
	    ::SendAll(this->getHandle() + " says I have to die now...", UF_NONE,
	      WatchSet(), this);
	    Log::Write("DIE requested by " + this->getHandle());
	  }
	  gracefuldie(SIGTERM); 
	}
      } else
	this->noAccess();
      break;
    case DCC_RAW:
      if (Flags & UF_MASTER) {
	if (To == "") {
	  Log::Write("RAW by " + this->getHandle() + ": " + text);
	  server.write(text + "\n");
	}
      } else
	this->noAccess();
      break;
    case DCC_TRAP:
      this->trap(To, text);
      break;
    case DCC_SEEDRAND:
      this->seedrand(To, text);
      break;
    case DCC_STATUS:
      this->status(To, text);
      break;
    case DCC_SAVE:
      this->save(To, text);
      break;
    case DCC_LOAD:
      this->load(To, text);
      break;
    case DCC_SET:
      this->set(To, text);
      break;
    case DCC_SPAMSUB:
      this->spamsub(To, text);
      break;
    case DCC_SPAMUNSUB:
      this->spamunsub(To, text);
      break;
    case DCC_GLIST:
      this->glist(To, text);
      break;
    case DCC_LOCOPS:
      this->locops(To, text);
      break;
    case DCC_TEST:
#ifdef DCC_DEBUG
      if (Flags & UF_MASTER)
      {
	Proxy::check(text, text, "fake", "fake@" + text);
      }
      else
      {
	this->noAccess();
      }
      break;
#endif
    case DCC_UNKNOWN:
      this->send("*** Invalid command: " + Command);
      break;
    }
  }
  else					// Just chatter :P
  {
    // Chat
    if (text == "Unknown command" &&
      vars[VAR_IGNORE_UNKNOWN_COMMAND]->getBool())
    {
      // Just ignore it.
      return true;
    }
    this->chat(text);
  }
  return true;
}


// chat(text)
//
// Handles any text which should be interpretted as chatter.
//
void
DCC::chat(const std::string & text)
{
  if (this->isAuthed() || vars[VAR_UNAUTHED_MAY_CHAT]->getBool())
  {
    clients.sendChat(this->getHandle(), text, echoMyChatter ? NULL : this);
    remotes.sendChat(this->getHandle(), text);
  }
  else
  {
    this->send("*** You must authenticate yourself with \".auth\" before you can chat!");
  }
}


// motd()
//
// Writes the Message Of The Day
//
void
DCC::motd()
{
  StrList Output;
  ::motd(Output);
  this->send(Output);
}


// noAccess()
//
// Tells the user that he doesn't have access to use the command he passed
//
void
DCC::noAccess()
{
  this->send("*** You don't have access for that command!");
}


// notRemote()
//
// Tells the user that the command he used isn't usable remotely.
//
void
DCC::notRemote()
{
  this->send("*** That command can't be used remotely!");
}


// notAuthed()
//
// Tells the user that the command he must be authenticated before he can
// use the command.
//
void
DCC::notAuthed()
{
  this->send("*** You must authenticate yourself with \".auth\" before you can use that command!");
}


std::string
DCC::getHandle(const bool atBot) const
{
  if (atBot)
  {
    return (this->Handle + "@" + Config::GetNick());
  }
  else
  {
    return this->Handle;
  }
}


void
DCC::list(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r -count", "-class");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      std::string className;
      args.haveBinary("-class", className);

      if (text != "")
      {
	Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(text);
	  }
	  else
	  {
	    pattern = smartPattern(text, false);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

        StrList Output;
        try
	{
          users.listUsers(Output, pattern, className,
	    args.haveUnary("-count") ? UserHash::LIST_COUNT :
	    UserHash::LIST_VIEW);
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}
        this->send(Output);

	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .list [-count] [-class <name>] [-r] <pattern>");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::glist(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r -count", "-class");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      std::string className;
      args.haveBinary("-class", className);

      if (text != "")
      {
	Pattern *pattern;

	try
	{
          if (args.haveUnary("-r"))
          {
	    pattern = new RegExPattern(text);
	  }
          else
          {
	    pattern = smartPattern(text, false);
          }
        }
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

        StrList Output;
        try
	{
          users.listGecos(Output, pattern, className, args.haveUnary("-count"));
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}
        this->send(Output);
	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .glist [-class <name>] [-count] [-r] <pattern>");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::locops(const std::string & to, std::string text)
{
  if (this->Flags & UF_WALLOPS)
  {
    if (to.empty())
    {
      if (!text.empty())
      {
        server.locops("(" + this->getHandle(false) + ") " + text);
      }
      else
      {
	this->send("*** Syntax: .locops <text>");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::killList(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r", "-class");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      std::string className;
      args.haveBinary("-class", className);

      std::string mask = FirstWord(text);

      if (mask != "")
      {
	Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(mask);
	  }
	  else
	  {
	    pattern = smartPattern(mask, false);
	  }
        }
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

	try
	{
          StrList Output;
          if (text == "")
          {
            ::SendAll("KILLLIST " + mask + " [" + this->getHandle() + "]",
              UF_OPER, WATCH_KILLS, this);
            users.listUsers(Output, pattern, className, UserHash::LIST_KILL,
	      this->getHandle(), vars[VAR_KILLLIST_REASON]->getString());
          }
          else
          {
            ::SendAll("KILLLIST " + mask + " (" + text + ") [" +
              this->getHandle() + "]", UF_OPER, WATCH_KILLS, this);
	    users.listUsers(Output, pattern, className, UserHash::LIST_KILL,
	      this->getHandle(), text);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}
	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .killlist [-class <name>] [-r] <pattern> [<reason>]");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::nFind(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r -count", "-class");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      std::string className;
      args.haveBinary("-class", className);

      if (text != "")
      {
	Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(text);
	  }
	  else
	  {
	    pattern = smartPattern(text, true);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

        StrList Output;
        try
	{
          users.listNicks(Output, pattern, className,
	    args.haveUnary("-count") ? UserHash::LIST_COUNT :
	    UserHash::LIST_VIEW);
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}
        this->send(Output);

	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .nfind [-count] [-class <name>] [-r] <pattern>");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::killNFind(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r", "-class");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      std::string className;
      args.haveBinary("-class", className);

      std::string mask = FirstWord(text);

      if (mask != "")
      {
	Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(mask);
	  }
	  else
	  {
	    pattern = smartPattern(mask, true);
	  }
        }
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

	try
	{
          StrList Output;
          if (text == "")
          {
            ::SendAll("KILLNFIND " + mask + " [" + this->getHandle() + "]",
              UF_OPER, WATCH_KILLS, this);
            users.listNicks(Output, pattern, className, UserHash::LIST_KILL,
	      this->getHandle(), vars[VAR_KILLNFIND_REASON]->getString());
          }
          else
          {
            ::SendAll("KILLNFIND " + mask + " (" + text + ") [" +
              this->getHandle() + "]", UF_OPER, WATCH_KILLS, this);
	    users.listNicks(Output, pattern, className, UserHash::LIST_KILL,
	      this->getHandle(), text);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}
	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .killnfind [-class <name>] [-r] <pattern> [<reason>]");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::findK(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r -count -remove -temp -perm -reason", "");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      bool searchTemps = args.haveUnary("-temp");
      bool searchPerms = args.haveUnary("-perm");
      bool searchReason = args.haveUnary("-reason");

      // If neither "-temp" nor "-perm" were specified, search ALL klines.
      if (!searchTemps && !searchPerms)
      {
	searchTemps = true;
	searchPerms = true;
      }

      if (args.haveUnary("-remove"))
      {
        if (0 == (Flags & UF_KLINE))
        {
          this->noAccess();
          return;
        }
      }

      if (text != "")
      {
	Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(text);
	  }
	  else
	  {
	    pattern = smartPattern(text, false);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

        try
	{
          if (args.haveUnary("-remove"))
          {
            int removed = server.findAndRemoveK(this->getHandle(), pattern,
	      searchPerms, searchTemps, searchReason);
            this->send("*** " + IntToStr(removed) + " K-lines removed.");
          }
          else
          {
            StrList Output;
            server.findK(Output, pattern, args.haveUnary("-count"), searchPerms,
	      searchTemps, searchReason);
            this->send(Output);
          }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}

	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .findk [-count|-remove] [-reason] [-temp] [-perm] [-r] <pattern>");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::findD(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r -count -remove -temp -perm -reason", "");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      bool searchTemps = args.haveUnary("-temp");
      bool searchPerms = args.haveUnary("-perm");
      bool searchReason = args.haveUnary("-reason");

      // If neither "-temp" nor "-perm" were specified, search ALL klines.
      if (!searchTemps && !searchPerms)
      {
	searchTemps = true;
	searchPerms = true;
      }

      if (args.haveUnary("-remove"))
      {
        if (0 == (Flags & UF_DLINE))
        {
          this->noAccess();
          return;
        }
      }

      if (text != "")
      {
	Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(text);
	  }
	  else
	  {
	    pattern = smartPattern(text, false);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

        try
	{
	  if (args.haveUnary("-remove"))
          {
            int removed = server.findAndRemoveD(this->getHandle(), pattern,
	      searchPerms, searchTemps, searchReason);
            this->send("*** " + IntToStr(removed) + " D-lines removed.");
          }
          else
          {
            StrList Output;
            server.findD(Output, pattern, args.haveUnary("-count"),
	      searchPerms, searchTemps, searchReason);
            this->send(Output);
          }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}

	delete pattern;
      }
      else
      {
        this->send("*** Syntax: .findd [-count|-remove] [-reason] [-temp] [-perm] [-r] <pattern>");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::seedrand(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      ArgList args("-r -count", "-min");

      if (-1 == args.parseCommand(text))
      {
	this->send("*** Invalid parameter: " + args.getInvalid());
	return;
      }

      int threshhold = vars[VAR_SEEDRAND_COMMAND_MIN]->getInt();
      std::string parm;
      if (args.haveBinary("-min", parm))
      {
	threshhold = atoi(parm.c_str());
      }

      StrList output;
      if (text != "")
      {
        Pattern *pattern;

	try
	{
	  if (args.haveUnary("-r"))
	  {
	    pattern = new RegExPattern(text);
	  }
	  else
	  {
	    pattern = smartPattern(text, true);
	  }
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	  return;
	}

        try
	{
	  users.reportSeedrand(output, pattern, threshhold,
	    args.haveUnary("-count"));
	}
	catch (OOMon::regex_error & e)
	{
	  this->send("*** RegEx error: " + e.what());
	}

	delete pattern;
      }
      else
      {
	NickClusterPattern pattern("*");
	users.reportSeedrand(output, &pattern, threshhold,
	  args.haveUnary("-count"));
      }
      this->send(output);
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::status(const std::string & to, std::string text)
{
  if (this->Flags & UF_AUTHED)
  {
    if (to == "")
    {
      StrList output;
      users.status(output);
      ::status(output);
      server.status(output);
      clients.status(output);
      Proxy::status(output);
      this->send(output);
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::save(const std::string & to, std::string text)
{
  if (this->Flags & UF_MASTER)
  {
    if (to == "")
    {
      if (Config::saveSettings())
      {
        std::string notice("*** Saved settings to file.");
        ::SendAll(notice, UF_OPER);
        Log::Write(notice);
      }
      else
      {
	this->send("*** Error saving settings to file!");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::load(const std::string & to, std::string text)
{
  if (this->Flags & UF_MASTER)
  {
    if (to == "")
    {
      if (Config::loadSettings())
      {
        std::string notice("*** Loaded settings from file.");
        ::SendAll(notice, UF_OPER);
        Log::Write(notice);
      }
      else
      {
	this->send("*** Error loading settings from file!");
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::set(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      std::string varName = FirstWord(text);
      bool clearVar = false;

      if ((varName.length() > 1) && (varName[0] == '-'))
      {
        clearVar = true;
	varName = varName.substr(1);
      }

      if ((text.length() == 0) && !clearVar)
      {
	StrList output;
	if (vars.get(output, varName) > 0)
	{
	  this->send(output);
	}
	else
	{
	  this->send("No such variable \"" + varName + "\"");
	}
      }
      else
      {
        if (this->Flags & UF_MASTER)
        {
	  std::string error = vars.set(varName, text, this->getHandle());
	  if (error.length() > 0)
	  {
	    this->send(error);
	  }
	}
	else
	{
          this->noAccess();
	}
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::spamsub(const std::string & to, std::string text)
{
  if (this->Flags & UF_MASTER)
  {
    if (to == "")
    {
      std::string notice("*** " + this->getHandle() +
	" subscribing to SpamTrap");

      ::SendAll(notice, UF_OPER);
      Log::Write(notice);

      server.subSpamTrap(true);
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::spamunsub(const std::string & to, std::string text)
{
  if (this->Flags & UF_MASTER)
  {
    if (to == "")
    {
      std::string notice("*** " + this->getHandle() +
	" unsubscribing from SpamTrap");

      ::SendAll(notice, UF_OPER);
      Log::Write(notice);

      server.subSpamTrap(false);
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


// kline(Minutes, Target, Reason)
//
// Writes a kline to the server
//
void
DCC::kline(const int Minutes, const std::string & Target,
  const std::string & Reason)
{
  if (Minutes)
    ::SendAll("KLINE " + IntToStr(Minutes) + " " + Target + " (" + Reason +
      " [" + this->getHandle() + "])", UF_OPER, WATCH_KLINES,
      this);
  else
    ::SendAll("KLINE " + Target + " (" + Reason + " [" + this->getHandle() +
      "])", UF_OPER, WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, Reason);
}


// kClone(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kClone(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KCLONE " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KCLONE " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_CLONES);
}


// kFlood(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kFlood(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KFLOOD " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KFLOOD " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_FLOODING);
}


// kBot(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kBot(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KBOT " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KBOT " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_BOTS);
}


// kSpam(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kSpam(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KSPAM " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KSPAM " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_SPAM);
}


// kLink(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kLink(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KLINK " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KLINK " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_LINKS);
}


// kTrace(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kTrace(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KTRACE " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KTRACE " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_TRACE);
}


// kMotd(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kMotd(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KMOTD " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KMOTD " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_MOTD);
}


// kInfo(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kInfo(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KINFO " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KINFO " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_INFO);
}


// kProxy(Minutes, Target)
//
// Writes a kline to the server
//
void
DCC::kProxy(const int Minutes, const std::string & Target)
{
  if (Minutes)
    ::SendAll("KPROXY " + IntToStr(Minutes) + " " + Target + " [" +
      this->getHandle() + "]", UF_OPER, WATCH_KLINES, this);
  else
    ::SendAll("KPROXY " + Target + " [" + this->getHandle() + "]", UF_OPER,
      WATCH_KLINES, this);
  server.kline(this->getHandle(), Minutes, Target, REASON_PROXY);
}


// kPerm(Target, Reason)
//
// Writes a kline to the server
//
void
DCC::kPerm(const std::string & Target, const std::string & Reason)
{
  ::SendAll("KPERM " + Target + " (" + Reason + " [" + this->getHandle() + "])",
    UF_OPER, WATCH_KLINES, this);
  server.kline(this->getHandle(), 0, Target, "PERM " + Reason);
}


// unkline(Target)
//
// Just the opposite of Kline
//
void
DCC::unkline(const std::string & Target)
{
  ::SendAll("UNKLINE " + Target + " [" + this->getHandle() + "]", UF_OPER,
    WATCH_KLINES, this);
  server.unkline(this->getHandle(), Target);
}


// dline(Target)
//
// Writes a dline to the server
//
void
DCC::dline(const int Minutes, const std::string & Target,
  const std::string & Reason)
{
  if (Minutes)
    ::SendAll("DLINE " + IntToStr(Minutes) + " " + Target + " (" + Reason +
      " [" + this->getHandle() + "])", UF_OPER, WATCH_DLINES,
      this);
  else
    ::SendAll("DLINE " + Target + " (" + Reason + " [" + this->getHandle() +
      "])", UF_OPER, WATCH_DLINES, this);
  server.dline(this->getHandle(), Minutes, Target, Reason);
}


// undline(Target)
//
// Just the opposite of dline
//
void
DCC::undline(const std::string & Target)
{
  ::SendAll("UNDLINE " + Target + " [" + this->getHandle() + "]", UF_OPER,
    WATCH_DLINES, this);
  server.undline(this->getHandle(), Target);
}


// help(Topic)
//
// Gets help
//
void
DCC::help(std::string Topic)
{
  StrList text = Help::getHelp(Topic);

  if (text.size() > 0)
  {
    this->send(text);
  }
  else
  {
    this->send("Help can be read at http://oomon.sourceforge.net/");
  }
}


// getFlags()
//
// Returns a fixed length string containing the user's flags
//
std::string
DCC::getFlags() const
{
  std::string temp = "";

  if (this->isAuthed())
  {
    if (this->Flags & UF_CHANOP) temp += "C"; else temp += " ";
    if (this->Flags & UF_DLINE) temp += "D"; else temp += " ";
    if (this->Flags & UF_GLINE) temp += "G"; else temp += " ";
    if (this->Flags & UF_KLINE) temp += "K"; else temp += " ";
    if (this->Flags & UF_MASTER) temp += "M"; else temp += " ";
    if (this->Flags & UF_NICK) temp += "N"; else temp += " ";
    if (this->Flags & UF_OPER) temp += "O"; else temp += " ";
    if (this->Flags & UF_REMOTE) temp += "R"; else temp += " ";
    if (this->Flags & UF_WALLOPS) temp += "W"; else temp += " ";
    if (this->Flags & UF_CONN) temp += "X"; else temp += " ";
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
    ((flags == UF_NONE) || (flags == (this->Flags & flags))))
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
DCC::trap(const std::string & to, std::string text)
{
  if (this->Flags & UF_OPER)
  {
    if (to == "")
    {
      StrList output;
      TrapList::cmd(output, text, this->Flags & UF_MASTER, this->getHandle());
      this->send(output);
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->noAccess();
  }
}


void
DCC::watch(const std::string & to, std::string text)
{
  if (this->isAuthed())
  {
    if (to == "")
    {
      if (text == "")
      {
        this->send("Watching: " + WatchSet::getWatchNames(this->watches,
	  false));
      }
      else
      {
	StrList output;

	this->watches.set(output, text);

	this->send(output);

	userConfig->setWatches(this->Handle,
	  WatchSet::getWatchNames(this->watches, true));
      }
    }
    else
    {
      this->notRemote();
    }
  }
  else
  {
    this->notAuthed();
  }
}


void
DCC::doEcho(const std::string & parm)
{
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
        userConfig->setEcho(this->Handle, true);
      }
    }
    else if (parm == "OFF")
    {
      this->echoMyChatter = false;
      this->send("*** ECHO is now OFF.");
      if (this->isAuthed())
      {
        userConfig->setEcho(this->Handle, false);
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
DCC::loadConfig()
{
  try
  {
    this->echoMyChatter = userConfig->getEcho(this->Handle);
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
    this->watches.set(output, userConfig->getWatches(this->Handle));
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


DCC::DccCommand
DCC::getCommand(const std::string & text)
{
  if (text == "AUTH") return DCC_AUTH;
  else if (text == "QUIT") return DCC_QUIT;
  else if (text == "CHAT") return DCC_CHAT;
  else if (text == "WHO") return DCC_WHO;
  else if (text == "LINKS") return DCC_LINKS;
  else if (text == "HELP") return DCC_HELP;
  else if (text == "WATCH") return DCC_WATCH;
  else if (text == "MOTD") return DCC_MOTD;
  else if (text == "KLINE") return DCC_KLINE;
  else if (text == "UNKLINE") return DCC_UNKLINE;
  else if (text == "KCLONE") return DCC_KCLONE;
  else if (text == "KFLOOD") return DCC_KFLOOD;
  else if (text == "KBOT") return DCC_KBOT;
  else if (text == "KLINK") return DCC_KLINK;
  else if (text == "KSPAM") return DCC_KSPAM;
  else if (text == "KPERM") return DCC_KPERM;
  else if (text == "GLINE") return DCC_GLINE;
  else if (text == "UNGLINE") return DCC_UNGLINE;
  else if (text == "DLINE") return DCC_DLINE;
  else if (text == "KILL") return DCC_KILL;
  else if (text == "KILLLIST") return DCC_KILLLIST;
  else if (text == "KL") return DCC_KILLLIST;
  else if (text == "KILLNFIND") return DCC_KILLNFIND;
  else if (text == "KN") return DCC_KILLNFIND;
  else if (text == "MULTI") return DCC_MULTI;
  else if (text == "HMULTI") return DCC_HMULTI;
  else if (text == "UMULTI") return DCC_UMULTI;
  else if (text == "VMULTI") return DCC_VMULTI;
  else if (text == "BOTS") return DCC_MULTI;
  else if (text == "VBOTS") return DCC_VMULTI;
  else if (text == "NFIND") return DCC_NFIND;
  else if (text == "TRACE") return DCC_TRACE;
  else if (text == "CLONES") return DCC_CLONES;
  else if (text == "LIST") return DCC_LIST;
  else if (text == "IPLIST") return DCC_LIST;
  else if (text == "FINDK") return DCC_FINDK;
  else if (text == "RELOAD") return DCC_RELOAD;
  else if (text == "DOMAINS") return DCC_DOMAINS;
  else if (text == "CLASS") return DCC_CLASS;
  else if (text == "DIE") return DCC_DIE;
  else if (text == "CONN") return DCC_CONN;
  else if (text == "DISCONN") return DCC_DISCONN;
  else if (text == "KTRACE") return DCC_KTRACE;
  else if (text == "KMOTD") return DCC_KMOTD;
  else if (text == "KINFO") return DCC_KINFO;
  else if (text == "KPROXY") return DCC_KPROXY;
  else if (text == "KWINGATE") return DCC_KPROXY;
  else if (text == "RAW") return DCC_RAW;
  else if (text == "OP") return DCC_OP;
  else if (text == "JOIN") return DCC_JOIN;
  else if (text == "PART") return DCC_PART;
  else if (text == "TRAP") return DCC_TRAP;
  else if (text == "UNDLINE") return DCC_UNDLINE;
  else if (text == "FINDD") return DCC_FINDD;
  else if (text == "ECHO") return DCC_ECHO;
  else if (text == "SEEDRAND") return DCC_SEEDRAND;
  else if (text == "STATUS") return DCC_STATUS;
  else if (text == "SAVE") return DCC_SAVE;
  else if (text == "LOAD") return DCC_LOAD;
  else if (text == "SET") return DCC_SET;
  else if (text == "SPAMSUB") return DCC_SPAMSUB;
  else if (text == "SPAMUNSUB") return DCC_SPAMUNSUB;
  else if (text == "GLIST") return DCC_GLIST;
  else if (text == "NETS") return DCC_NETS;
  else if (text == "INFO") return DCC_INFO;
  else if (text == "LOCOPS") return DCC_LOCOPS;
#ifdef DCC_DEBUG
  else if (text == "TEST") return DCC_TEST;
#endif
  else return DCC_UNKNOWN;
}

