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

// Std C Headers
#include <errno.h>
#include <time.h>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "irc.h"
#include "config.h"
#include "log.h"
#include "main.h"
#include "util.h"
#include "engine.h"
#include "services.h"
#include "dcclist.h"
#include "dcc.h"
#include "vars.h"
#include "jupe.h"
#include "userhash.h"
#include "pattern.h"
#include "botclient.h"


#ifdef DEBUG
# define IRC_DEBUG
#endif


IRC server;


IRC::IRC(): BotSock(false, true), supportETrace(false), supportKnock(false),
  caseMapping(CASEMAP_RFC1459), klines('K'), dlines('D')
{
  this->amIAnOper = false;
  this->serverName = "";
  this->gettingKlines = false;
  this->gettingTempKlines = false;
  this->gettingDlines = false;
  this->gettingTrace = false;
  this->myNick = "";
  this->lastUserDeltaCheck = 0;
  this->lastCtcpVersionTimeoutCheck = 0;

  registerOnConnectHandler(boost::bind(&IRC::onConnect, this));
  registerOnReadHandler(boost::bind(&IRC::onRead, this, _1));

  addServerNoticeParser("Client connecting: *", ::onClientConnect);
  addServerNoticeParser("Client exiting: *", ::onClientExit);
  addServerNoticeParser("Nick change: *", ::onNickChange);
  addServerNoticeParser("* is now an operator*", ::onOperNotice);
  addServerNoticeParser("Failed OPER attempt - *", ::onOperFailNotice);
  addServerNoticeParser("Received KILL message for *", ::onKillNotice);
  addServerNoticeParser("Invalid username: *", ::onInvalidUsername);
  addServerNoticeParser("LINKS *", ::onLinksNotice);
  addServerNoticeParser("TRACE *", ::onTraceNotice);
  addServerNoticeParser("MOTD *", ::onMotdNotice);
  addServerNoticeParser("INFO *", ::onInfoNotice);
  addServerNoticeParser("STATS *", ::onStatsNotice);
  addServerNoticeParser("Possible flooder*", ::onFlooderNotice);
  addServerNoticeParser("*is a possible spambot*", ::onSpambotNotice);
  addServerNoticeParser("Too many local connections for*",
    ::onTooManyConnNotice);
  addServerNoticeParser("* is attempting to join locally juped channel*",
    ::onJupeJoinNotice);
  addServerNoticeParser("* added K-Line for *",
    boost::bind(&KlineList::parseAndAdd, &this->klines, _1));
  addServerNoticeParser("* added temporary * K-Line for *",
    boost::bind(&KlineList::parseAndAdd, &this->klines, _1));
  addServerNoticeParser("* has removed the K-Line for: *",
    boost::bind(&KlineList::parseAndRemove, &this->klines, _1));
  addServerNoticeParser("* has removed the temporary K-Line for: *",
    boost::bind(&KlineList::parseAndRemove, &this->klines, _1));
  addServerNoticeParser("Temporary K-line for * expired",
    boost::bind(&KlineList::onExpireNotice, &this->klines, _1));
  addServerNoticeParser("* added D-Line for *",
    boost::bind(&KlineList::parseAndAdd, &this->dlines, _1));
  addServerNoticeParser("* added temporary * D-Line for *",
    boost::bind(&KlineList::parseAndAdd, &this->dlines, _1));
  addServerNoticeParser("* has removed the D-Line for: *",
    boost::bind(&KlineList::parseAndRemove, &this->dlines, _1));
  addServerNoticeParser("* has removed the temporary D-Line for: *",
    boost::bind(&KlineList::parseAndRemove, &this->dlines, _1));
  addServerNoticeParser("Temporary D-line for * expired",
    boost::bind(&KlineList::onExpireNotice, &this->dlines, _1));
  addServerNoticeParser("?LINE active for *", ::onLineActive);
  addServerNoticeParser("?LINE over-ruled for *", ::onLineActive);
  addServerNoticeParser("* is requesting gline for *", ::onGlineRequest);
  addServerNoticeParser("* has triggered gline for *", ::onGlineRequest);
  addServerNoticeParser("Rejecting clonebot: *", ::onCsClones);
  addServerNoticeParser("Clonebot killed: *", ::onCsClones);
  addServerNoticeParser("Nick flooding detected by: *", ::onCsNickFlood);
  addServerNoticeParser("Rejecting *", ::onBotReject);
  addServerNoticeParser("* is clearing temp klines", ::onClearTempKlines);
  addServerNoticeParser("* is clearing temp dlines", ::onClearTempDlines);
}


bool
IRC::process(const fd_set & readset, const fd_set & writeset)
{
  time_t now = time(0);

  // If we've been idle for half the timeout period, send a PING to make
  // sure the connection is still good!
  if (this->isConnected() && (this->getIdle() > (this->getTimeout() / 2)) &&
    (this->getWriteIdle() > (this->getTimeout() / 2)))
  {
    this->write("PING :" + this->myNick + '\n');
  }

  if ((now - this->lastCtcpVersionTimeoutCheck) > 10)
  {
    users.checkVersionTimeout();
    this->lastCtcpVersionTimeoutCheck = now;
  }

  return BotSock::process(readset, writeset);
}


bool
IRC::onConnect()
{
  Log::Write("Connected to IRC server");

  // Send password (if necessary)
  if (Config::GetServerPassword() != "")
  {
    this->write("PASS " + Config::GetServerPassword() + "\n");
  }

  // Register client
  this->write("USER " + Config::GetUserName() + " oomon " +
    std::string(OOMON_VERSION) + " :" + Config::GetIRCName() + "\n");

  // Select nickname
  this->myNick = Config::GetNick();
  this->write("NICK " + this->myNick + "\n");

  this->amIAnOper = false;

  this->setTimeout(vars[VAR_SERVER_TIMEOUT]->getInt());

  return true;
}


int
IRC::write(const std::string & text)
{
#ifdef IRC_DEBUG
  std::cout << "IRC << " << text;
#endif

  this->lastWrite = time(NULL);

  return BotSock::write(text);
}


void
IRC::quit(const std::string & message)
{
#ifdef IRC_DEBUG
  std::cout << "Disconnecting from server." << std::endl;
#endif

  this->write("QUIT :" + message + "\n");
  Log::Write("Disconnecting from server");
  this->reset();

  this->amIAnOper = 0;
}


bool
IRC::onRead(std::string text)
{
  if (text == "")
    return true;

#ifdef IRC_DEBUG
  std::cout << "IRC >> " << text << std::endl;
#endif

  StrVector params;

  if (text[0] != ':')
  {
    SplitIRC(params, text);
    params.insert(params.begin(), serverName);
  }
  else
  {
    SplitIRC(params, text.substr(1));
  }

  std::string from, userhost;
  SplitFrom(params[0], from, userhost);

  std::string command = params[1];
  int numeric;

  if ((numeric = atoi(command.c_str())) > 0)
  {
    switch(numeric)
    {
      case 001:
        serverName = from;
        // Oper up
        this->write("OPER " + Config::GetOperNick() + " " +
	  Config::GetOperPass() + "\n");
        this->klines.Clear();
        this->dlines.Clear();
        this->gettingDlines = false;
        this->gettingKlines = false;
        this->gettingTempKlines = false;
        this->gettingTrace = false;
        if (Config::GetChannels() != "")
        {
          this->write("JOIN " + Config::GetChannels() + "\n");
        }
        Init_Nick_Change_Table();
        Init_Link_Look_Table();
        break;
      case 005:
        for (StrVector::size_type idx = 3; idx < params.size(); ++idx)
        {
          if (params[idx] == "ETRACE")
	  {
	    this->supportETrace = true;
	  }
	  else if (params[idx] == "KNOCK")
	  {
	    this->supportKnock = true;
	  }
	  else if (params[idx].substr(0, 12) == "CASEMAPPING=")
	  {
	    std::string map = params[idx].substr(12);

	    if (Same(map, "ascii"))
	    {
	      this->caseMapping = CASEMAP_ASCII;
	    }
	    else if (Same(map, "strict-rfc1459"))
	    {
	      this->caseMapping = CASEMAP_STRICT_RFC1459;
	    }
	    else
	    {
	      this->caseMapping = CASEMAP_RFC1459;
	    }
	  }
	}
	break;
      case 204:	// ontraceuser(body)
      case 205:	// ontraceuser(body)
        if (this->gettingTrace && (params.size() > 6))
        {
          if ((params[6][0] == '[') && (params.size() > 7))
	  {
            // [hybrid7-rc6] IRC >> :plasma.toast.pc 204 OOMon Oper opers OOMon [toast@Plasma.Toast.PC] (192.168.1.1) 0 0
            onTraceUser(params[3], params[4], params[5], params[6], params[7]);
	  }
	  else
	  {
            // [hybrid6] IRC >> :plasma.toast.pc 205 OOMon User 1 Toast[toast@Plasma.Toast.PC] (192.168.1.1) 000000005 000000005
            onTraceUser(params[3], params[4], params[5], params[6]);
	  }
        }
        break;
      case 209:
      case 262:
        if (this->gettingTrace)
        {
          this->gettingTrace = false;
	  users.resetUserCountDelta();
          ::SendAll("TRACE complete.", UserFlags::OPER);
        }
        break;
      case 216:
        // IRC >> :plasma.engr.arizona.edu 216 OOMon K *.monkeys.org * * foo llalalala (1998/03/03 11.18)
        // IRC >> :plasma.engr.arizona.edu 216 OOMon K *bork.com * *hork moo la la la (1998/03/03 11.18)
        if ((params.size() > 6) &&
	  ((params[3] == "K") && vars[VAR_TRACK_PERM_KLINES]->getBool()) ||
	  ((params[3] == "k") && vars[VAR_TRACK_TEMP_KLINES]->getBool()))
        {
	  std::string reason = (params.size() > 7) ? params[7] : "";
	  for (StrVector::size_type pos = 8; pos < params.size(); pos++)
	  {
	    reason += " " + params[pos];
	  }
	  this->klines.Add(params[6] + "@" + params[4], reason,
	    params[3] == "k");
        }
        break;
      case 219:
	if (params.size() >= 3)
	{
          if (params[3] == "K")
          {
            this->gettingKlines = false;
          }
          else if (params[3] == "D")
          {
            this->gettingDlines = false;
          }
          else if (params[3] == "k")
          {
            this->gettingTempKlines = false;
          }
	}
        break;
      case 225:
        // IRC >> :plasma.toast.pc 225 ToastFOO D 1.2.3.4 :foo (2003/1/9 17.27)
        if ((params.size() > 4) &&
	  ((params[3] == "D") && vars[VAR_TRACK_PERM_DLINES]->getBool()) ||
	  ((params[3] == "d") && vars[VAR_TRACK_TEMP_DLINES]->getBool()))
        {
	  std::string reason = (params.size() > 5) ? params[5] : "";
	  for (StrVector::size_type pos = 6; pos < params.size(); pos++)
	  {
	    reason += " " + params[pos];
	  }
	  this->dlines.Add(params[4], reason, params[3] == "d");
        }
        break;
      case 303:
        if (params.size() >= 4)
	{
	  services.onIson(params[3]);
        }
        break;
      case 311:	/* RPL_WHOISUSER */
        if (params.size() > 5)
        {
	  // :plasma.toast.pc 311 Toast Toast toast Plasma.Toast.PC * :i
	  std::string nick = params[3];

	  if (this->same(nick, vars[VAR_SPAMTRAP_NICK]->getString()))
	  {
	    std::string userhost = params[4] + '@' + params[5];

	    if (this->same(userhost, vars[VAR_SPAMTRAP_USERHOST]->getString()))
	    {
	      services.pollSpamtrap();
	    }
	  }
	}
        break;
      case 381:
        {
	  this->amIAnOper = true;
#ifdef USE_FLAGS
	  this->write("FLAGS +ALL\n");
#endif
          this->umode(vars[VAR_UMODE]->getString());
          this->trace();
	  this->reloadDlines();
	  this->reloadKlines();
        }
	break;
      case 433:
	if (params.size() > 3)
	{
	  std::string to = params[2];
	  std::string usedNick = params[3];

	  if (to == "*")
	  {
	    if (usedNick.length() < 9)
	    {
	      this->myNick = usedNick + "_";
	    }
	    else
	    {
	      this->myNick = usedNick.substr(1) + usedNick.substr(0, 1);
	    }
	    this->write("NICK " + this->myNick + "\n");
	  }
	}
	break;
      case 473:		/* ERR_INVITEONLYCHAN */
	if (params.size() > 3)
	{
	  std::string channel = params[3];

          // If the IRC server supports the KNOCK command and the invite-only
	  // channel is listed in the bot's config file, issue a KNOCK.
	  if (this->supportKnock && Config::haveChannel(channel))
	  {
	    this->knock(channel);
	  }
	}
	break;
      case 709:
        if (this->gettingTrace && (params.size() > 9))
        {
          // :plasma.toast.pc 709 toast Oper opers toast toast Plasma.Toast.PC 192.168.1.1 :gecos information goes here
          onETraceUser(params[3], params[4], params[5], params[6], params[7],
	    params[8], params[9]);
        }
	break;
      default:
	// Just ignore all other numerics
	break;
    }
  }
  else
  {
    switch (IRC::getCommand(command))
    {
      case IRC_PING:
	if (params.size() > 2)
	{
          this->write("PONG " + params[2] + "\n");
	}
	else
	{
          this->write("PONG\n");
	}
        break;
      case IRC_NICK:
	if (params.size() > 2)
	{
	  if (this->same(from, this->myNick))
	  {
	    this->myNick = params[2];
	  }
	}
	break;
      case IRC_JOIN:
	if ((params.size() >= 3) && this->same(from, this->myNick))
	{
	  Log::Write("Joined channel " + params[2]);
	}
	break;
      case IRC_PART:
	if ((params.size() >= 3) && this->same(from, this->myNick))
	{
	  Log::Write("Parted channel " + params[2]);
	}
	break;
      case IRC_KICK:
	if ((params.size() >= 5) && this->same(params[3], this->myNick))
	{
	  Log::Write("Kicked from channel " + params[2] + " by " + from + 
	    " (" + params[4] + ')');
	}
	break;
      case IRC_INVITE:
	if (params.size() >= 3)
	{
	  std::string to = params[2];
	  std::string channel = params[3];

          // Make sure the INVITE was directed at the bot
          if (this->same(to, this->myNick))
	  {
	    // Is the channel listed in the config file?
	    if (Config::haveChannel(channel))
	    {
	      // Yes, so accept the invite by joining the channel!
	      this->join(channel);
	    }

	    Log::Write("Invited to channel " + params[3] + " by " + from + '.');
	  }
	}
	break;
      case IRC_NOTICE:
	if (params.size() > 3)
        {
	  this->onNotice(from, userhost, params[2], params[3]);
        }
        break;
      case IRC_PRIVMSG:
        if (params.size() > 3)
	{
          this->onPrivmsg(from, userhost, params[2], params[3]);
        }
        break;
      case IRC_WALLOPS:
	if (params.size() > 2)
        {
	  std::string text = params[2];
	  std::string wallopsType = "WALLOPS";

	  if ((text.length() > 11) && (text.substr(0, 11) == "OPERWALL - "))
	  {
	    wallopsType = "OPERWALL";
	    text = text.substr(11, std::string::npos);
	  }
	  else if ((text.length() > 9) && (text.substr(0, 9) == "LOCOPS - "))
	  {
	    wallopsType = "LOCOPS";
	    text = text.substr(9, std::string::npos);
	  }
	  else if ((text.length() > 10) && (text.substr(0, 10) == "WALLOPS - "))
	  {
	    text = text.substr(10, std::string::npos);
	  }

          clients.sendAll("[" + from + ":" + wallopsType + "] " + text,
	    UserFlags::WALLOPS, WATCH_WALLOPS);
        }
        break;
      case IRC_ERROR:
	if (params.size() > 2)
	{
	  Log::Write(params[2]);
	}
	break;
      default:
	// Ignore all other IRC commands
	break;
    }
  }

  return true;
}

void
IRC::onCtcp(const std::string & from, const std::string & userhost,
  const std::string & to, std::string text)
{
  std::string command = this->upCase(FirstWord(text));

  BotSock::Address ip = users.getIP(from, userhost);

  if ((command == "DCC") && (this->same(to, this->myNick)))
  {
    if (!vars[VAR_OPER_ONLY_DCC]->getBool() || Config::IsOper(userhost, ip))
    {
      std::string dccCommand = FirstWord(text);
      if (dccCommand == "CHAT")
      {
        FirstWord(text);	// Ignore "chat" parameter

        BotSock::Address address = atoul(FirstWord(text).c_str());;
        BotSock::Port port = atoi(FirstWord(text).c_str());

        if ((address == INADDR_ANY) || (address == INADDR_NONE))
        {
          this->notice(from,
	    "Invalid address specified for DCC CHAT. Not funny.");
          return;
        }
        if (port < 1024)
        {
          this->notice(from, "Invalid port specified for DCC CHAT. Not funny.");
          return;
        }
        Log::Write("DCC CHAT request from " + from + " (" + userhost + ")");
        if (!clients.connect(address, port, from, userhost))
        {
          Log::Write("DCC CHAT failed for " + from);
        }
      }
    }
  }
  else if (Config::IsOper(userhost, ip) && (command == "CHAT"))
  {
    Log::Write("CHAT request from " + from + " (" + userhost + ")");
    if (!clients.listen(from, userhost))
    {
      Log::Write("DCC CHAT listen failed for " + from);
    }
  }
  else if ((command == std::string("PING")) ||
    (command == std::string("ECHO")))
  {
    this->ctcpReply(from, command + " " + text);
  }
  else if (command == std::string("VERSION"))
  {
    this->ctcpReply(from, "VERSION OOMon-" OOMON_VERSION
      " - http://oomon.sourceforge.net/");
  }
}


void
IRC::onCtcpReply(const std::string & from, const std::string & userhost,
  const std::string & to, std::string text)
{
  if (this->same(to, this->myNick))
  {
    std::string command(FirstWord(text));

    if (this->same(command, "VERSION"))
    {
      users.onVersionReply(from, userhost, text);
    }
  }
}


void
IRC::onPrivmsg(const std::string & from, const std::string & userhost,
  const std::string & to, std::string text)
{
  if ((text.length() > 0) && (text[0] == '\001'))
  {
    std::string ctcp = text.substr(1);

    std::string::size_type end = ctcp.find('\001');

    if (end != std::string::npos)
    {
      this->onCtcp(from, userhost, to, ctcp.substr(0, end));
    }
  }
  else if (this->same(to, this->myNick))
  {
    if (vars[VAR_SPAMTRAP_ENABLE]->getBool() &&
      this->same(from, vars[VAR_SPAMTRAP_NICK]->getString()) &&
      this->same(userhost, vars[VAR_SPAMTRAP_USERHOST]->getString()))
    {
      services.onSpamtrapMessage(text);
    }
    else
    {
      std::string msg("*" + from + "* " + text + " <" + userhost + ">");

      Log::Write(msg);
      ::SendAll("(IRC) " + msg, UserFlags::OPER, WATCH_MSGS);
      if (vars[VAR_RELAY_MSGS_TO_LOCOPS]->getBool())
      {
	this->locops(msg);
      }
    }
  }
}


void
IRC::onNotice(const std::string & from, const std::string & userhost,
	const std::string & to, std::string text)
{
  if (!text.empty() && (text[0] == '\001'))
  {
    text.erase(text.begin());

    std::string::size_type end = text.find('\001');

    if (std::string::npos != end)
    {
      text.erase(end, std::string::npos);
    }

    this->onCtcpReply(from, userhost, to, text);
  }
  else if (this->same(from, serverName))
  {
    if ((text.length() > 14) &&
      (0 == text.substr(0, 14).compare("*** Notice -- ")))
    {
      // Server Notice
      this->onServerNotice(text.substr(14));
    }
    else if (0 ==
      this->upCase(text).compare("*** YOU NEED OPER AND N FLAG FOR +N"))
    {
      Log::Write("I don't have an N flag in my o: line! :(");
    }
  }
  else if (this->same(from, vars[VAR_XO_SERVICES_RESPONSE]->getString()))
  {
    services.onXoNotice(text);
  }
  else if (this->same(from, CA_SERVICES_RESPONSE))
  {
    services.onCaNotice(text);
  }
  else if (this->same(to, this->myNick))
  {
    if (vars[VAR_SPAMTRAP_ENABLE]->getBool() &&
      this->same(from, vars[VAR_SPAMTRAP_NICK]->getString()) &&
      this->same(userhost, vars[VAR_SPAMTRAP_USERHOST]->getString()))
    {
      services.onSpamtrapNotice(text);
    }
    else if (userhost.length() > 0)
    {
      // Log all non-server notices
      Log::Write("-" + from + "- " + text + " <" + userhost + ">");
    }
  }
}


void
IRC::notice(const std::string & to, const std::string & text)
{
  this->write("NOTICE " + to + " :" + text + "\n");
}


void
IRC::notice(const std::string & to, const StrList & text)
{
  for (StrList::const_iterator pos = text.begin(); pos != text.end(); ++pos)
  {
    this->notice(to, *pos);
  }
}


void
IRC::msg(const std::string & to, const std::string & text)
{
  this->write("PRIVMSG " + to + " :" + text + "\n");
}


void
IRC::ctcp(const std::string & to, const std::string & text)
{
  this->write("PRIVMSG " + to + " :\001" + text + "\001\n");
}


void
IRC::ctcpReply(const std::string & to, const std::string & text)
{
  this->write("NOTICE " + to + " :\001" + text + "\001\n");
}


void
IRC::isOn(const std::string & text)
{
  this->write("ISON " + text + "\n");
}


void
IRC::kline(const std::string & from, const int minutes,
  const std::string & target, const std::string & reason)
{
  std::string line;

  if (minutes > 0)
  {
    line = "KLINE " + IntToStr(minutes) + " " + target + " :" + reason;
  }
  else
  {
    line = "KLINE " + target + " :" + reason;
  }

  if (vars[VAR_OPER_NICK_IN_REASON]->getBool())
  {
    this->write(line + " [" + from + "]\n");
  }
  else
  {
    this->write(line + "\n");
  }
  Log::Write(line + " [" + from + "]");
}


void
IRC::unkline(const std::string & from, const std::string & target)
{
  this->write("UNKLINE " + target + "\n");
  Log::Write("UNKLINE " + target + " [" + from + "]");
}


void
IRC::dline(const std::string & from, const int minutes,
  const std::string & target, const std::string & reason)
{
  std::string line;

  if (minutes > 0)
  {
    line = "DLINE " + IntToStr(minutes) + " " + target + " :" + reason;
  }
  else
  {
    line = "DLINE " + target + " :" + reason;
  }

  if (vars[VAR_OPER_NICK_IN_REASON]->getBool())
  {
    this->write(line + " [" + from + "]\n");
  }
  else
  {
    this->write(line + "\n");
  }
  Log::Write(line + " [" + from + "]");
}


void
IRC::undline(const std::string & from, const std::string & target)
{
  this->write("UNDLINE " + target + "\n");
  Log::Write("UNDLINE " + target + " [" + from + "]");
}


void
IRC::kill(const std::string & from, const std::string & target,
  const std::string & reason)
{
  std::string line = "KILL " + target;

  if (vars[VAR_OPER_NICK_IN_REASON]->getBool())
  {
    this->write(line + " :(" + from + ") " + reason + "\n");
  }
  else
  {
    this->write(line + " :" + reason + "\n");
  }
  Log::Write(line + " ((" + from + ") " + reason + ")");
}


void
IRC::trace(const std::string & target)
{
  if (0 == target.length())
  {
    users.clear();
    this->gettingTrace = true;
    if (this->supportETrace)
    {
      this->write("ETRACE\n");
    }
    else
    {
      this->write("TRACE\n");
    }
  }
  else
  {
    this->write("TRACE " + target + '\n');
  }
}


void
IRC::statsL(const std::string & nick)
{
  if (nick != "")
  {
    this->write("STATS L " + nick + '\n');
  }
}


void
IRC::retrace(const std::string & from)
{
  ::SendAll("Reload USERS requested by " + from, UserFlags::OPER);
  this->trace();
}


void
IRC::reloadKlines(void)
{
  this->klines.Clear();
  if (vars[VAR_TRACK_PERM_KLINES]->getBool())
  {
    this->gettingKlines = true;
    this->write("STATS K\n");
  }
  if (vars[VAR_TRACK_TEMP_KLINES]->getBool())
  {
    this->gettingTempKlines = true;
    this->write("STATS k\n");
  }
}


void
IRC::reloadKlines(const std::string & from)
{
    ::SendAll("Reload KLINES requested by " + from, UserFlags::OPER);
    this->reloadKlines();
}


void
IRC::reloadDlines(void)
{
  this->dlines.Clear();
  if (vars[VAR_TRACK_PERM_DLINES]->getBool())
  {
    this->gettingDlines = true;
    this->write("STATS D\n");
  }
  if (vars[VAR_TRACK_TEMP_DLINES]->getBool())
  {
    this->gettingTempKlines = true;
    this->write("STATS d\n");
  }
}


void
IRC::reloadDlines(const std::string & from)
{
  ::SendAll("Reload DLINES requested by " + from, UserFlags::OPER);
  this->reloadDlines();
}


void
IRC::findK(BotClient * client, const Pattern *userhost, const bool count,
  const bool searchPerms, const bool searchTemps, const bool searchReason) const
{
  this->klines.find(client, userhost, count, searchPerms, searchTemps,
    searchReason);
}


int
IRC::findAndRemoveK(const std::string & from, const Pattern *userhost,
  const bool searchPerms, const bool searchTemps, const bool searchReason)
{
  return this->klines.findAndRemove(from, userhost, searchPerms, searchTemps,
    searchReason);
}


void
IRC::findD(BotClient * client, const Pattern *userhost, const bool count,
  const bool searchPerms, const bool searchTemps, const bool searchReason) const
{
  this->dlines.find(client, userhost, count, searchPerms, searchTemps,
    searchReason);
}


int
IRC::findAndRemoveD(const std::string & from, const Pattern *userhost,
  const bool searchPerms, const bool searchTemps, const bool searchReason)
{
  return this->dlines.findAndRemove(from, userhost, searchPerms, searchTemps,
    searchReason);
}


void
IRC::op(const std::string & channel, const std::string & nick)
{
  this->write("MODE " + channel + " +o " + nick + "\n");
}

//////////////////////////////////////////////////////////////////////
// join(channel)
//
// Description:
//  Sends a JOIN command to the IRC server.
//
// Parameters:
//  channel - The channel to attempt to join.
//
// Return Value:
//  None.
//////////////////////////////////////////////////////////////////////
void
IRC::join(const std::string & channel)
{
  this->write("JOIN " + channel + "\n");
}

//////////////////////////////////////////////////////////////////////
// join(channel, key)
//
// Description:
//  Sends a JOIN command with a key to the IRC server.
//
// Parameters:
//  channel - The channel to attempt to join.
//  key     - The channel key to use when joining.
//
// Return Value:
//  None.
//////////////////////////////////////////////////////////////////////
void
IRC::join(const std::string & channel, const std::string & key)
{
  this->write("JOIN " + channel + " " + key + "\n");
}

//////////////////////////////////////////////////////////////////////
// part(channel)
//
// Description:
//  Sends a PART command to the IRC server.
//
// Parameters:
//  channel - The channel to attempt to leave.
//
// Return Value:
//  None.
//////////////////////////////////////////////////////////////////////
void
IRC::part(const std::string & channel)
{
  this->write("PART " + channel + "\n");
}

//////////////////////////////////////////////////////////////////////
// knock(channel)
//
// Description:
//  Sends a KNOCK command to the IRC server.
//
// Parameters:
//  channel - The channel for which the knock is intended.
//
// Return Value:
//  None.
//////////////////////////////////////////////////////////////////////
void
IRC::knock(const std::string & channel)
{
  this->write("KNOCK " + channel + "\n");
}


void
IRC::status(BotClient * client) const
{
  client->send("Connect Time: " + this->getUptime());

  KlineList::size_type klineCount = this->klines.size();
  if (vars[VAR_TRACK_TEMP_KLINES]->getBool() && (klineCount > 0))
  {
    client->send("K: lines: " + IntToStr(klineCount) + " (" +
      IntToStr(this->klines.permSize()) + " permanent)");
  }
  else if (vars[VAR_TRACK_PERM_KLINES]->getBool())
  {
    client->send("K: lines: " + IntToStr(klineCount));
  }

  KlineList::size_type dlineCount = this->dlines.size();
  if (vars[VAR_TRACK_TEMP_DLINES]->getBool() && (dlineCount > 0))
  {
    client->send("D: lines: " + IntToStr(dlineCount) + " (" +
      IntToStr(this->dlines.permSize()) + " permanent)");
  }
  else if (vars[VAR_TRACK_PERM_DLINES]->getBool())
  {
    client->send("D: lines: " + IntToStr(dlineCount));
  }
}


void
IRC::subSpamTrap(const bool sub)
{
  if (sub)
  {
    this->msg(vars[VAR_SPAMTRAP_NICK]->getString(), ".nicksub " +
      IntToStr(vars[VAR_SPAMTRAP_MIN_SCORE]->getInt()) + " " +
      this->getServerName() + " raw");
  }
  else
  {
    this->msg(vars[VAR_SPAMTRAP_NICK]->getString(), ".nickunsub");
  }
}


void
IRC::whois(const std::string & nick)
{
  this->write("WHOIS " + nick + '\n');
}


void
IRC::umode(const std::string & mode)
{
  this->write("MODE " + this->myNick + " :" + mode + '\n');
}


void
IRC::locops(const std::string & text)
{
  this->write("LOCOPS :" + text + '\n');
}


void
IRC::checkUserDelta(void)
{
  time_t now = ::time(NULL);
  time_t lapse = now - this->lastUserDeltaCheck;

  if (lapse >= 10)
  {
    int delta = users.getUserCountDelta();

    if ((this->lastUserDeltaCheck > 0) &&
      (delta > vars[VAR_USER_COUNT_DELTA_MAX]->getInt()))
    {
      std::string msg("*** User count increased by " + IntToStr(delta) +
	" in " + IntToStr(lapse) + " seconds.");
      clients.sendAll(msg, UserFlags::OPER);
      Log::Write(msg);
    }
    this->lastUserDeltaCheck = now;
  }
}


IRCCommand
IRC::getCommand(const std::string & text)
{
  if (text == "PING")
    return IRC_PING;
  else if (text == "NICK")
    return IRC_NICK;
  else if (text == "JOIN")
    return IRC_JOIN;
  else if (text == "PART")
    return IRC_PART;
  else if (text == "KICK")
    return IRC_KICK;
  else if (text == "INVITE")
    return IRC_INVITE;
  else if (text == "NOTICE")
    return IRC_NOTICE;
  else if (text == "PRIVMSG")
    return IRC_PRIVMSG;
  else if (text == "WALLOPS")
    return IRC_WALLOPS;
  else if (text == "ERROR")
    return IRC_ERROR;
  else
    return IRC_UNKNOWN;
}


//////////////////////////////////////////////////////////////////////
// IRC::upCase(c)
//
// Description:
//  Converts a character to upper-case.
//
// Parameters:
//  c - An upper or lower-case character.
//
// Return Value:
//  The function returns the upper-case representation of the
//  character.
//////////////////////////////////////////////////////////////////////
char
IRC::upCase(const char c) const
{
  char result;

  if (this->caseMapping == CASEMAP_ASCII)
  {
    if ((c >= 97) && (c <= 122))
    {
      result = static_cast<char>((c - 32));
    }
    else
    {
      result = c;
    }
  }
  else if (this->caseMapping == CASEMAP_STRICT_RFC1459)
  {
    if ((c >= 97) && (c <= 125))
    {
      result = static_cast<char>((c - 32));
    }
    else
    {
      result = c;
    }
  }
  else /* this->caseMapping == CASEMAP_RFC1459 */
  {
    if ((c >= 97) && (c <= 126))
    {
      result = static_cast<char>((c - 32));
    }
    else
    {
      result = c;
    }
  }

  return result;
}


//////////////////////////////////////////////////////////////////////
// IRC::upCase(text)
//
// Description:
//  Converts a string to all upper-case characters.
//
// Parameters:
//  text - A string containing upper and/or lower-case characters.
//
// Return Value:
//  The function returns the upper-case representation of the string.
//////////////////////////////////////////////////////////////////////
std::string
IRC::upCase(const std::string & text) const
{
  std::string result;

  if (this->caseMapping == CASEMAP_ASCII)
  {
    for (std::string::const_iterator pos = text.begin(); pos != text.end();
      ++pos)
    {
      if ((*pos >= 97) && (*pos <= 122))
      {
        result += static_cast<char>((*pos - 32));
      }
      else
      {
        result += static_cast<char>(*pos);
      }
    }
  }
  else if (this->caseMapping == CASEMAP_STRICT_RFC1459)
  {
    for (std::string::const_iterator pos = text.begin(); pos != text.end();
      ++pos)
    {
      if ((*pos >= 97) && (*pos <= 125))
      {
        result += static_cast<char>((*pos - 32));
      }
      else
      {
        result += static_cast<char>(*pos);
      }
    }
  }
  else /* this->caseMapping == CASEMAP_RFC1459 */
  {
    for (std::string::const_iterator pos = text.begin(); pos != text.end();
      ++pos)
    {
      if ((*pos >= 97) && (*pos <= 126))
      {
        result += static_cast<char>((*pos - 32));
      }
      else
      {
        result += static_cast<char>(*pos);
      }
    }
  }

  return result;
}


//////////////////////////////////////////////////////////////////////
// IRC::downCase(c)
//
// Description:
//  Converts a character to lower-case.
//
// Parameters:
//  c - An upper or lower-case character.
//
// Return Value:
//  The function returns the lower-case representation of the
//  character.
//////////////////////////////////////////////////////////////////////
char
IRC::downCase(const char c) const
{
  char result;

  if (this->caseMapping == CASEMAP_ASCII)
  {
    if ((c >= 65) && (c <= 90))
    {
      result = static_cast<char>((c + 32));
    }
    else
    {
      result = c;
    }
  }
  else if (this->caseMapping == CASEMAP_STRICT_RFC1459)
  {
    if ((c >= 65) && (c <= 93))
    {
      result = static_cast<char>((c + 32));
    }
    else
    {
      result = c;
    }
  }
  else /* this->caseMapping == CASEMAP_RFC1459 */
  {
    if ((c >= 65) && (c <= 94))
    {
      result = static_cast<char>((c + 32));
    }
    else
    {
      result = c;
    }
  }

  return result;
}


//////////////////////////////////////////////////////////////////////
// IRC::downCase(text)
//
// Description:
//  Converts a string to all lower-case characters.
//
// Parameters:
//  text - A string containing upper and/or lower-case characters.
//
// Return Value:
//  The function returns the lower-case representation of the string.
//////////////////////////////////////////////////////////////////////
std::string
IRC::downCase(const std::string & text) const
{
  std::string result;

  if (this->caseMapping == CASEMAP_ASCII)
  {
    for (std::string::const_iterator pos = text.begin(); pos != text.end();
      ++pos)
    {
      if ((*pos >= 65) && (*pos <= 90))
      {
        result += static_cast<char>((*pos + 32));
      }
      else
      {
        result += static_cast<char>(*pos);
      }
    }
  }
  else if (this->caseMapping == CASEMAP_STRICT_RFC1459)
  {
    for (std::string::const_iterator pos = text.begin(); pos != text.end();
      ++pos)
    {
      if ((*pos >= 65) && (*pos <= 93))
      {
        result += static_cast<char>((*pos + 32));
      }
      else
      {
        result += static_cast<char>(*pos);
      }
    }
  }
  else /* this->caseMapping == CASEMAP_RFC1459 */
  {
    for (std::string::const_iterator pos = text.begin(); pos != text.end();
      ++pos)
    {
      if ((*pos >= 65) && (*pos <= 94))
      {
        result += static_cast<char>((*pos + 32));
      }
      else
      {
        result += static_cast<char>(*pos);
      }
    }
  }

  return result;
}


//////////////////////////////////////////////////////////////////////
// IRC::same(text1, text2)
//
// Description:
//  Case-insensitively compares two strings.
//
// Parameters:
//  text1  - The first string.
//  text2  - The second string.
//
// Return Value:
//  The function returns true if both strings match.
//////////////////////////////////////////////////////////////////////
bool
IRC::same(const std::string & text1, const std::string & text2) const
{
  return (0 == this->upCase(text1).compare(this->upCase(text2)));
}


//////////////////////////////////////////////////////////////////////
// IRC::Parser::match(text)
//
// Description:
//  Compares the text against the parser's pattern.
//
// Parameters:
//  text - The text to compare against the pattern.
//
// Return Value:
//  The function returns true if the text matches the pattern and
//  the caller should stop parsing the text.
//////////////////////////////////////////////////////////////////////
bool
IRC::Parser::match(std::string text) const
{
  bool result = false;

  if (this->_pattern.match(text))
  {
    result = this->_func(text);
  }

  return result;
}


//////////////////////////////////////////////////////////////////////
// IRC::addServerNotice(pattern, func)
//
// Description:
//  Registers a new server notice handler function.
//
// Parameters:
//  pattern - A string containing a cluster-style pattern.
//  func    - The function to call when text matching the pattern is
//            detected.
//
// Return Value:
//  None
//////////////////////////////////////////////////////////////////////
void
IRC::addServerNoticeParser(const std::string & pattern,
  const ParserFunction func)
{
  serverNotices.push_back(Parser(pattern, func));
}


void
IRC::onServerNotice(const std::string & text)
{
  std::find_if(this->serverNotices.begin(), this->serverNotices.end(),
    boost::bind(&Parser::match, _1, text));
}

