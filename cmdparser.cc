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
#include <algorithm>
#include <functional>
#include <memory>

#include "oomon.h"
#include "cmdparser.h"
#include "dcclist.h"
#include "remotelist.h"
#include "proxylist.h"
#include "help.h"
#include "main.h"
#include "log.h"
#include "engine.h"
#include "userhash.h"
#include "arglist.h"
#include "pattern.h"
#include "trap.h"
#include "vars.h"
#include "config.h"


#ifdef DEBUG
# define CMDPARSER_DEBUG
#endif


CommandParser::CommandParser(void)
{
  // anonymous commands
  addCommand("HELP", &CommandParser::cmdHelp, UF_NONE, NO_REMOTE);
  addCommand("INFO", &CommandParser::cmdHelp, UF_NONE, NO_REMOTE);

  // UF_AUTHED commands
  addCommand("WHO", &CommandParser::cmdWho, UF_AUTHED);
  addCommand("LINKS", &CommandParser::cmdLinks, UF_AUTHED);
  addCommand("MOTD", &CommandParser::cmdMotd, UF_AUTHED);
  addCommand("STATUS", &CommandParser::cmdStatus, UF_AUTHED);

  // UF_CHANOP commands
  addCommand("JOIN", &CommandParser::cmdJoin, UF_CHANOP);
  addCommand("OP", &CommandParser::cmdOp, UF_CHANOP);
  addCommand("PART", &CommandParser::cmdPart, UF_CHANOP);

  // UF_WALLOPS commands
  addCommand("LOCOPS", &CommandParser::cmdLocops, UF_WALLOPS, NO_REMOTE);

  // UF_KLINE commands
  addCommand("KLINE", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KBOT", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KCLONE", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KFLOOD", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KINFO", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KLINK", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KMOTD", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KPERM", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KPROXY", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KWINGATE", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KSPAM", &CommandParser::cmdKline, UF_KLINE);
  addCommand("KTRACE", &CommandParser::cmdKline, UF_KLINE);
  addCommand("UNKLINE", &CommandParser::cmdUnkline, UF_KLINE);

  // UF_GLINE commands
  //addCommand("GLINE", &CommandParser::cmdGline, UF_GLINE, NO_REMOTE);
  //addCommand("UNGLINE", &CommandParser::cmdGline, UF_GLINE, NO_REMOTE);

  // UF_DLINED commands
  addCommand("DLINE", &CommandParser::cmdDline, UF_DLINE);
  addCommand("UNDLINE", &CommandParser::cmdUndline, UF_DLINE);

  // UF_OPER commands
  addCommand("KILL", &CommandParser::cmdKill, UF_OPER);
  addCommand("KILLLIST", &CommandParser::cmdKilllist, UF_OPER, EXACT_ONLY);
  addCommand("KL", &CommandParser::cmdKilllist, UF_OPER, EXACT_ONLY);
  addCommand("KILLNFIND", &CommandParser::cmdKillnfind, UF_OPER, EXACT_ONLY);
  addCommand("KN", &CommandParser::cmdKillnfind, UF_OPER, EXACT_ONLY);
  addCommand("RELOAD", &CommandParser::cmdReload, UF_OPER);
  addCommand("TRACE", &CommandParser::cmdReload, UF_OPER);
  addCommand("TRAP", &CommandParser::cmdTrap, UF_OPER);
  addCommand("SET", &CommandParser::cmdSet, UF_OPER);
  addCommand("NFIND", &CommandParser::cmdNfind, UF_OPER);
  addCommand("LIST", &CommandParser::cmdList, UF_OPER);
  addCommand("IPLIST", &CommandParser::cmdList, UF_OPER);
  addCommand("GLIST", &CommandParser::cmdGlist, UF_OPER);
  addCommand("FINDK", &CommandParser::cmdFindk, UF_OPER);
  addCommand("FINDD", &CommandParser::cmdFindd, UF_OPER);
  addCommand("CLASS", &CommandParser::cmdClass, UF_OPER);
  addCommand("DOMAINS", &CommandParser::cmdDomains, UF_OPER);
  addCommand("NETS", &CommandParser::cmdDomains, UF_OPER);
  addCommand("MULTI", &CommandParser::cmdMulti, UF_OPER);
  addCommand("BOTS", &CommandParser::cmdMulti, UF_OPER);
  addCommand("HMULTI", &CommandParser::cmdMulti, UF_OPER);
  addCommand("UMULTI", &CommandParser::cmdMulti, UF_OPER);
  addCommand("VMULTI", &CommandParser::cmdMulti, UF_OPER);
  addCommand("VBOTS", &CommandParser::cmdMulti, UF_OPER);
  addCommand("CLONES", &CommandParser::cmdClones, UF_OPER);
  addCommand("SEEDRAND", &CommandParser::cmdSeedrand, UF_OPER);
  addCommand("DRONES", &CommandParser::cmdSeedrand, UF_OPER);

  // UF_MASTER commands
  addCommand("DIE", &CommandParser::cmdDie, UF_MASTER, EXACT_ONLY);
  addCommand("CONN", &CommandParser::cmdConn, UF_MASTER);
  addCommand("DISCONN", &CommandParser::cmdDisconn, UF_MASTER);
  addCommand("RAW", &CommandParser::cmdRaw, UF_MASTER, EXACT_ONLY);
  addCommand("SAVE", &CommandParser::cmdSave, UF_MASTER);
  addCommand("LOAD", &CommandParser::cmdLoad, UF_MASTER);
  addCommand("SPAMSUB", &CommandParser::cmdSpamsub, UF_MASTER);
  addCommand("SPAMUNSUB", &CommandParser::cmdSpamunsub, UF_MASTER);
#ifdef CMDPARSER_DEBUG
  addCommand("TEST", &CommandParser::cmdTest, UF_MASTER, EXACT_ONLY);
#endif /* CMDPARSER_DEBUG */
}


void
CommandParser::parse(const BotClient::ptr from, const std::string & command,
  const std::string & parameters)
{
  CommandVector::iterator cmd = std::find_if(this->commands.begin(),
    this->commands.end(), CommandParser::exact_match(command));

  if (cmd == this->commands.end())
  {
    int count = std::count_if(this->commands.begin(), this->commands.end(),
      CommandParser::partial_match(command));

#ifdef CMDPARSER_DEBUG
    std::cout << count << " command(s) found matching \"" << command << "\"" <<
      std::endl;
#endif /* CMDPARSER_DEBUG */

    if (1 == count)
    {
      cmd = std::find_if(this->commands.begin(),
        this->commands.end(), CommandParser::partial_match(command));
    }
  }

  if (cmd == this->commands.end())
  {
    throw CommandParser::exception("*** Ambigious command: " + command);
  }
  else
  {
    if (cmd->flags == (from->flags() & cmd->flags))
    {
      if (!cmd->noRemote || !from->remote())
      {
        (*(cmd->func))(from, cmd->name, parameters);
      }
      else
      {
        throw CommandParser::exception("*** Not a remote command: " +
	  cmd->name);
      }
    }
    else
    {
      CommandParser::insufficient_privileges();
    }
  }
}


void
CommandParser::cmdHelp(BotClient::ptr from, const std::string & command,
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

  StrList output = Help::getHelp(topic);

  if (output.empty())
  {
    from->send("Help can be read at http://oomon.sourceforge.net/");
  }
  else
  {
    from->send(output);
  }
}


void
CommandParser::cmdWho(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  StrList output;

  clients.who(output);
  from->send(output);
}


void
CommandParser::cmdLinks(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  StrList output;

  remotes.getLinks(output);
  from->send(output);
}


void
CommandParser::cmdMotd(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  StrList output;

  ::motd(output);
  from->send(output);
}


void
CommandParser::cmdStatus(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  StrList output;

  users.status(output);
  ::status(output);
  server.status(output);
  clients.status(output);
  proxies.status(output);
  from->send(output);
}


void
CommandParser::cmdKline(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  unsigned int minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();

  std::string parm = FirstWord(parameters);
  if (isNumeric(parm))
  {
    minutes = atoi(parm.c_str());
    parm = FirstWord(parameters);
  }

  if (!parm.empty())
  {
    std::string reason;

    if (command == "kbot")
    {
      reason = "Bots are prohibited";
    }
    else if (command == "kclone")
    {
      reason = "Clones are prohibited";
    }
    else if (command == "kflood")
    {
      reason = "Flooding is prohibited";
    }
    else if (command == "kinfo")
    {
      reason = "INFO flooders are prohibited";
    }
    else if (command == "klink")
    {
      reason = "LINKS lookers are prohibited";
    }
    else if (command == "kmotd")
    {
      reason = "MOTD flooders are prohibited";
    }
    else if (command == "kperm")
    {
      if (!parameters.empty())
      {
	reason = "PERM " + parameters;
      }
      else
      {
	throw CommandParser::exception("*** Please include a k-line reason");
      }
    }
    else if ((command == "kproxy") || (command == "kwingate"))
    {
      reason = "Open proxy";
    }
    else if (command == "kspam")
    {
      reason = "Spamming is prohibited";
    }
    else if (command == "ktrace")
    {
      reason = "TRACE flooders are prohibited";
    }
    else
    {
      reason = parameters;
    }

    if (!reason.empty())
    {
      if (minutes)
      {
        ::SendAll("KLINE " + IntToStr(minutes) + " " + parm + " (" + reason +
          " [" + from->handleAndBot() + "])", UF_OPER, WATCH_KLINES, from);
      }
      else
      {
        ::SendAll("KLINE " + parm + " (" + reason + " [" +
	  from->handleAndBot() + "])", UF_OPER, WATCH_KLINES, from);
      }
      server.kline(from->handleAndBot(), minutes, parm, reason);
    }
    else
    {
      throw CommandParser::exception("*** Please include a k-line reason.");
    }
  }
  else
  {
    throw CommandParser::exception("*** Please include a nick or userhost mask to k-line.");
  }
}


void
CommandParser::cmdUnkline(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string target = FirstWord(parameters);

  if (!target.empty())
  {
    ::SendAll("UNKLINE " + target + " [" + from->handleAndBot() + "]", UF_OPER,
      WATCH_KLINES, from);
    server.unkline(from->handleAndBot(), target);
  }
  else
  {
    throw CommandParser::exception("*** Please include a userhost mask to unkline.");
  }
}


void
CommandParser::cmdDline(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  unsigned int minutes = vars[VAR_DEFAULT_DLINE_TIMEOUT]->getInt();

  std::string parm = FirstWord(parameters);
  if (isNumeric(parm))
  {
    minutes = atoi(parm.c_str());
    parm = FirstWord(parameters);
  }

  if (!parm.empty())
  {
    if (!parameters.empty())
    {
      if (minutes)
      {
        ::SendAll("DLINE " + IntToStr(minutes) + " " + parm + " (" +
	  parameters + " [" + from->handleAndBot() + "])", UF_OPER,
	  WATCH_DLINES, from);
      }
      else
      {
        ::SendAll("DLINE " + parm + " (" + parameters + " [" +
	  from->handleAndBot() + "])", UF_OPER, WATCH_DLINES, from);
      }
      server.dline(from->handleAndBot(), minutes, parm, parameters);
    }
    else
    {
      throw CommandParser::exception("*** Please include a d-line reason.");
    }
  }
  else
  {
    throw CommandParser::exception("*** Please include a nick or ip mask to dline.");
  }
}


void
CommandParser::cmdUndline(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string target = FirstWord(parameters);

  if (!target.empty())
  {
    ::SendAll("UNDLINE " + target + " [" + from->handleAndBot() + "]", UF_OPER,
      WATCH_KLINES, from);
    server.undline(from->handleAndBot(), target);
  }
  else
  {
    throw CommandParser::exception("*** Please include an ip mask to undline.");
  }
}


void
CommandParser::cmdJoin(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string channel = FirstWord(parameters);
  std::string key = FirstWord(parameters);

  if (!channel.empty())
  {
    server.join(channel, key);
  }
  else
  {
    CommandParser::syntax(command, "<channel> [<key>]");
  }
}


void
CommandParser::cmdOp(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string channel = FirstWord(parameters);
  std::string nick = FirstWord(parameters);

  if (!channel.empty() && !nick.empty())
  {
    server.op(channel, nick);
  }
  else
  {
    CommandParser::syntax(command, "<channel> <nick>");
  }
}


void
CommandParser::cmdPart(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string channel = FirstWord(parameters);

  if (!channel.empty())
  {
    server.part(channel);
  }
  else
  {
    CommandParser::syntax(command, "<channel>");
  }
}


void
CommandParser::cmdLocops(BotClient::ptr from, const std::string & command,
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


void
CommandParser::cmdKill(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string nick = FirstWord(parameters);
  if (nick.empty() || parameters.empty())
  {
    CommandParser::syntax(command, "<nick> <reason>");
  }
  else
  {
    ::SendAll("KILL " + nick + " ((" + from->handleAndBot() + ") " +
      parameters + ")", UF_OPER, WATCH_KILLS, from);
    server.kill(from->handleAndBot(), nick, parameters);
  }
}


void
CommandParser::cmdKilllist(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r", "-class");

  if (-1 == args.parseCommand(parameters))
  {
    throw CommandParser::exception("*** Invalid parameter: " +
      args.getInvalid());
  }

  std::string className;
  args.haveBinary("-class", className);

  std::string mask = FirstWord(parameters);

  if (mask.empty())
  {
    CommandParser::syntax(command, "[-class <name>] [-r] <pattern> [<reason>]");
  }
  else
  {
    std::auto_ptr<Pattern> pattern;

    try
    {
      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(mask));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(mask, false));
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }

    try
    {
      StrList output;
      if (parameters.empty())
      {
        ::SendAll("KILLLIST " + mask + " [" + from->handleAndBot() + "]",
          UF_OPER, WATCH_KILLS, from);
        users.listUsers(output, pattern.get(), className, UserHash::LIST_KILL,
          from->handleAndBot(), vars[VAR_KILLLIST_REASON]->getString());
      }
      else
      {
        ::SendAll("KILLLIST " + mask + " (" + parameters + ") [" +
          from->handleAndBot() + "]", UF_OPER, WATCH_KILLS, from);
        users.listUsers(output, pattern.get(), className, UserHash::LIST_KILL,
	  from->handleAndBot(), parameters);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdKillnfind(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r", "-class");

  if (-1 == args.parseCommand(parameters))
  {
    throw CommandParser::exception("*** Invalid parameter: " +
      args.getInvalid());
  }

  std::string className;
  args.haveBinary("-class", className);

  std::string mask = FirstWord(parameters);

  if (mask.empty())
  {
    CommandParser::syntax(command, "[-class <name>] [-r] <pattern> [<reason>]");
  }
  else
  {
    std::auto_ptr<Pattern> pattern;

    try
    {
      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(mask));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(mask, true));
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }

    try
    {
      StrList output;
      if (parameters.empty())
      {
        ::SendAll("KILLNFIND " + mask + " [" + from->handleAndBot() + "]",
          UF_OPER, WATCH_KILLS, from);
        users.listNicks(output, pattern.get(), className, UserHash::LIST_KILL,
          from->handleAndBot(), vars[VAR_KILLNFIND_REASON]->getString());
      }
      else
      {
        ::SendAll("KILLNFIND " + mask + " (" + parameters + ") [" +
          from->handleAndBot() + "]", UF_OPER, WATCH_KILLS, from);
        users.listNicks(output, pattern.get(), className, UserHash::LIST_KILL,
	  from->handleAndBot(), parameters);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdReload(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  if (command == "trace")
  {
    server.retrace(from->handleAndBot());
  }
  else
  {
    std::string parm(UpCase(FirstWord(parameters)));

    if (parm == "USERS")
    {
      server.retrace(from->handleAndBot());
    }
    else if (parm == "KLINES")
    {
      server.reloadKlines(from->handleAndBot());
    }
    else if (parm == "DLINES")
    {
      server.reloadDlines(from->handleAndBot());
    }
    else if (parm == "CONFIG")
    {
      ReloadConfig(from->handleAndBot());
    }
    else if (parm == "HELP")
    {
      Help::flush();
    }
    else
    {
      CommandParser::syntax(command, "users|klines|dlines|config|help");
    }
  }
}


void
CommandParser::cmdTrap(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  StrList output;
  TrapList::cmd(output, parameters, from->flags() & UF_MASTER,
    from->handleAndBot());
  from->send(output);
}


void
CommandParser::cmdSet(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string varName = FirstWord(parameters);
  bool clearVar = false;

  if ((varName.length() > 1) && (varName[0] == '-'))
  {
    clearVar = true;
    varName = varName.substr(1);
  }

  if (parameters.empty() && !clearVar)
  {
    StrList output;
    if (vars.get(output, varName) > 0)
    {
      from->send(output);
    }
    else
    {
      from->send("No such variable \"" + varName + "\"");
    }
  }
  else
  {
    if (UF_MASTER == (from->flags() & UF_MASTER))
    {
      std::string error = vars.set(varName, parameters, from->handleAndBot());
      if (error.length() > 0)
      {
        from->send(error);
      }
    }
    else
    {
      CommandParser::insufficient_privileges();
    }
  }
}


void
CommandParser::cmdNfind(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r -count", "-class");

  if (-1 == args.parseCommand(parameters))
  {
    throw CommandParser::exception("*** Invalid parameter: " +
      args.getInvalid());
  }

  std::string className;
  args.haveBinary("-class", className);

  if (parameters.empty())
  {
    CommandParser::syntax(command, "[-count] [-class <name>] [-r] <pattern>");
  }
  else
  {
    try
    {
      std::auto_ptr<Pattern> pattern;

      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(parameters));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(parameters, true));
      }

      StrList output;
      users.listNicks(output, pattern.get(), className,
        args.haveUnary("-count") ? UserHash::LIST_COUNT : UserHash::LIST_VIEW);
      from->send(output);
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdList(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r -count", "-class");

  if (-1 == args.parseCommand(parameters))
  {
    throw CommandParser::exception("*** Invalid parameter: " +
      args.getInvalid());
  }

  std::string className;
  args.haveBinary("-class", className);

  if (parameters.empty())
  {
    CommandParser::syntax(command, "[-count] [-class <name>] [-r] <pattern>");
  }
  else
  {
    try
    {
      std::auto_ptr<Pattern> pattern;

      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(parameters));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(parameters, false));
      }

      StrList output;
      users.listUsers(output, pattern.get(), className,
        args.haveUnary("-count") ? UserHash::LIST_COUNT : UserHash::LIST_VIEW);
      from->send(output);
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdGlist(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r -count", "-class");

  if (-1 == args.parseCommand(parameters))
  {
    throw CommandParser::exception("*** Invalid parameter: " +
      args.getInvalid());
  }

  std::string className;
  args.haveBinary("-class", className);

  if (parameters.empty())
  {
    CommandParser::syntax(command, "[-class <name>] [-count] [-r] <pattern>");
  }
  else
  {
    try
    {
      std::auto_ptr<Pattern> pattern;

      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(parameters));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(parameters, false));
      }

      StrList output;
      users.listGecos(output, pattern.get(), className,
	args.haveUnary("-count"));
      from->send(output);
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdFindk(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r -count -remove -temp -perm -reason", "");

  if (-1 == args.parseCommand(parameters))
  {
   throw CommandParser::exception("*** Invalid parameter: " +
     args.getInvalid());
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
    if (0 == (from->flags() & UF_KLINE))
    {
      CommandParser::insufficient_privileges();
    }
  }

  if (parameters.empty())
  {
    CommandParser::syntax(command,
      "[-count|-remove] [-reason] [-temp] [-perm] [-r] <pattern>");
  }
  else
  {
    try
    {
      std::auto_ptr<Pattern> pattern;

      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(parameters));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(parameters, false));
      }

      if (args.haveUnary("-remove"))
      {
        int removed = server.findAndRemoveK(from->handleAndBot(), pattern.get(),
          searchPerms, searchTemps, searchReason);
        from->send("*** " + IntToStr(removed) + " K-lines removed.");
      }
      else
      {
        StrList output;
        server.findK(output, pattern.get(), args.haveUnary("-count"),
	  searchPerms, searchTemps, searchReason);
        from->send(output);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdFindd(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r -count -remove -temp -perm -reason", "");

  if (-1 == args.parseCommand(parameters))
  {
   throw CommandParser::exception("*** Invalid parameter: " +
     args.getInvalid());
  }

  bool searchTemps = args.haveUnary("-temp");
  bool searchPerms = args.haveUnary("-perm");
  bool searchReason = args.haveUnary("-reason");

  // If neither "-temp" nor "-perm" were specified, search ALL dlines.
  if (!searchTemps && !searchPerms)
  {
    searchTemps = true;
    searchPerms = true;
  }

  if (args.haveUnary("-remove"))
  {
    if (0 == (from->flags() & UF_DLINE))
    {
      CommandParser::insufficient_privileges();
    }
  }

  if (parameters.empty())
  {
    CommandParser::syntax(command,
      "[-count|-remove] [-reason] [-temp] [-perm] [-r] <pattern>");
  }
  else
  {
    try
    {
      std::auto_ptr<Pattern> pattern;

      if (args.haveUnary("-r"))
      {
        pattern = std::auto_ptr<Pattern>(new RegExPattern(parameters));
      }
      else
      {
        pattern = std::auto_ptr<Pattern>(smartPattern(parameters, false));
      }

      if (args.haveUnary("-remove"))
      {
        int removed = server.findAndRemoveD(from->handleAndBot(), pattern.get(),
          searchPerms, searchTemps, searchReason);
        from->send("*** " + IntToStr(removed) + " D-lines removed.");
      }
      else
      {
        StrList output;
        server.findD(output, pattern.get(), args.haveUnary("-count"),
	  searchPerms, searchTemps, searchReason);
        from->send(output);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdClass(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string className = FirstWord(parameters);

  StrList output;
  users.reportClasses(output, className);
  from->send(output);
}


void
CommandParser::cmdDomains(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string min_str = FirstWord(parameters);
  int min = 5;

  if (!min_str.empty())
  {
    min = atoi(min_str.c_str());
  }

  if (min >= 1)
  {
    StrList output;
    if (command == "domains")
    {
      users.reportDomains(output, min);
    }
    else if (command == "nets")
    {
      users.reportNets(output, min);
    }
    from->send(output);
  }
  else
  {
    throw CommandParser::exception("*** Invalid minimum user size!");
  }
}


void
CommandParser::cmdMulti(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string min_str = FirstWord(parameters);
  int min = atoi(min_str.c_str());

  StrList output;
  if ((command == "multi") || (command == "bots"))
  {
    users.reportMulti(output, min);
  }
  else if (command == "hmulti")
  {
    users.reportHMulti(output, min);
  }
  else if (command == "umulti")
  {
    users.reportUMulti(output, min);
  }
  else if ((command == "vmulti") || (command == "vbots"))
  {
    users.reportVMulti(output, min);
  }
  from->send(output);
}


void
CommandParser::cmdClones(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  StrList output;
  users.reportClones(output);
  from->send(output);
}


void
CommandParser::cmdSeedrand(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  ArgList args("-r -count", "-min");

  if (-1 == args.parseCommand(parameters))
  {
    throw CommandParser::exception("*** Invalid parameter: " +
      args.getInvalid());
  }

  int threshhold = vars[VAR_SEEDRAND_COMMAND_MIN]->getInt();
  std::string parm;
  if (args.haveBinary("-min", parm))
  {
    threshhold = atoi(parm.c_str());
  }

  StrList output;
  try
  {
    std::auto_ptr<Pattern> pattern;

    if (parameters.empty())
    {
      pattern = std::auto_ptr<Pattern>(new NickClusterPattern("*"));
    }
    else if (args.haveUnary("-r"))
    {
      pattern = std::auto_ptr<Pattern>(new RegExPattern(parameters));
    }
    else
    {
      pattern = std::auto_ptr<Pattern>(smartPattern(parameters, true));
    }

    users.reportSeedrand(output, pattern.get(), threshhold,
      args.haveUnary("-count"));
    from->send(output);
  }
  catch (OOMon::regex_error & e)
  {
    throw CommandParser::exception("*** RegEx error: " + e.what());
  }
}


void
CommandParser::cmdDie(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  from->send("As you wish...");
  if (!parameters.empty())
  {
    ::SendAll(from->handleAndBot() + " says I have to die now... (" +
      parameters + ")", UF_NONE, WatchSet(), from);
    Log::Write("DIE requested by " + from->handleAndBot() + " (" +
      parameters + ")");
    server.quit(parameters);
  }
  else
  {
    ::SendAll(from->handleAndBot() + " says I have to die now...", UF_NONE,
      WatchSet(), from);
    Log::Write("DIE requested by " + from->handleAndBot());
  }
  ::gracefuldie(SIGTERM); 
}


void
CommandParser::cmdConn(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string bot = FirstWord(parameters);
  if (bot.empty())
  {
    from->send("*** Please specify the bot to connect");
  }
  else
  {
    remotes.conn(from->handleAndBot(), bot);
  }
}


void
CommandParser::cmdDisconn(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string bot = FirstWord(parameters);
  if (bot.empty())
  {
    from->send("*** Please specify the bot to disconnect");
  }
  else
  {
    remotes.disconn(from->handleAndBot(), bot);
  }
}


void
CommandParser::cmdRaw(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  Log::Write("RAW by " + from->handleAndBot() + ": " + parameters);
  server.write(parameters + "\n");
}


void
CommandParser::cmdSave(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  if (Config::saveSettings())
  {
    std::string notice("*** Saved settings to file.");
    ::SendAll(notice, UF_OPER);
    Log::Write(notice);
  }
  else
  {
    from->send("*** Error saving settings to file!");
  }
}


void
CommandParser::cmdLoad(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  if (Config::loadSettings())
  {
    std::string notice("*** Loaded settings from file.");
    ::SendAll(notice, UF_OPER);
    Log::Write(notice);
  }
  else
  {
    from->send("*** Error loading settings from file!");
  }
}


void
CommandParser::cmdSpamsub(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string notice("*** " + from->handleAndBot() +
    " subscribing to SpamTrap");

  ::SendAll(notice, UF_OPER);
  Log::Write(notice);

  server.subSpamTrap(true);
}


void
CommandParser::cmdSpamunsub(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  std::string notice("*** " + from->handleAndBot() +
    " unsubscribing from SpamTrap");

  ::SendAll(notice, UF_OPER);
  Log::Write(notice);

  server.subSpamTrap(false);
}


void
CommandParser::cmdTest(BotClient::ptr from, const std::string & command,
  std::string parameters)
{
  server.onServerNotice(parameters);
}

