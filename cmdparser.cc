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

// Boost C++ Headers
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "oomon.h"
#include "cmdparser.h"
#include "dcclist.h"
#include "remotelist.h"
#include "proxylist.h"
#include "main.h"
#include "log.h"
#include "engine.h"
#include "userhash.h"
#include "arglist.h"
#include "pattern.h"
#include "trap.h"
#include "vars.h"
#include "config.h"
#include "help.h"
#include "dnsbl.h"


#ifdef DEBUG
# define CMDPARSER_DEBUG
#endif


CommandParser::CommandParser(void)
{
  // UserFlags::AUTHED commands
  this->addCommand("WHO", &CommandParser::cmdWho, UserFlags::AUTHED);
  this->addCommand("LINKS", &CommandParser::cmdLinks, UserFlags::AUTHED);
  this->addCommand("MOTD", &CommandParser::cmdMotd, UserFlags::AUTHED);
  this->addCommand("STATUS", &CommandParser::cmdStatus, UserFlags::AUTHED);
  this->addCommand("VERSION", &CommandParser::cmdVersion, UserFlags::AUTHED);

  // UserFlags::CHANOP commands
  this->addCommand("JOIN", &CommandParser::cmdJoin, UserFlags::CHANOP);
  this->addCommand("OP", &CommandParser::cmdOp, UserFlags::CHANOP);
  this->addCommand("PART", &CommandParser::cmdPart, UserFlags::CHANOP);

  // UserFlags::KLINE commands
  this->addCommand("KLINE", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KBOT", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KCLONE", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KFLOOD", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KINFO", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KLINK", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KMOTD", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KPERM", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KPROXY", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KWINGATE", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KSPAM", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("KTRACE", &CommandParser::cmdKline, UserFlags::KLINE);
  this->addCommand("UNKLINE", &CommandParser::cmdUnkline, UserFlags::KLINE);

  // UserFlags::DLINED commands
  this->addCommand("DLINE", &CommandParser::cmdDline, UserFlags::DLINE);
  this->addCommand("UNDLINE", &CommandParser::cmdUndline, UserFlags::DLINE);

  // UserFlags::OPER commands
  this->addCommand("KILL", &CommandParser::cmdKill, UserFlags::OPER);
  this->addCommand("KILLLIST", &CommandParser::cmdKilllist, UserFlags::OPER,
    EXACT_ONLY);
  this->addCommand("KL", &CommandParser::cmdKilllist, UserFlags::OPER,
    EXACT_ONLY);
  this->addCommand("KILLNFIND", &CommandParser::cmdKillnfind, UserFlags::OPER,
    EXACT_ONLY);
  this->addCommand("KN", &CommandParser::cmdKillnfind, UserFlags::OPER,
    EXACT_ONLY);
  this->addCommand("RELOAD", &CommandParser::cmdReload, UserFlags::OPER);
  this->addCommand("TRACE", &CommandParser::cmdReload, UserFlags::OPER);
  this->addCommand("TRAP", &CommandParser::cmdTrap, UserFlags::OPER);
  this->addCommand("SET", &CommandParser::cmdSet, UserFlags::OPER);
  this->addCommand("NFIND", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("LIST", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("GLIST", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("ULIST", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("HLIST", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("IPLIST", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("FINDU", &CommandParser::cmdFindu, UserFlags::OPER);
  this->addCommand("FINDK", &CommandParser::cmdFindk, UserFlags::OPER);
  this->addCommand("FINDD", &CommandParser::cmdFindd, UserFlags::OPER);
  this->addCommand("CLASS", &CommandParser::cmdClass, UserFlags::OPER);
  this->addCommand("DOMAINS", &CommandParser::cmdDomains, UserFlags::OPER);
  this->addCommand("NETS", &CommandParser::cmdDomains, UserFlags::OPER);
  this->addCommand("MULTI", &CommandParser::cmdMulti, UserFlags::OPER);
  this->addCommand("BOTS", &CommandParser::cmdMulti, UserFlags::OPER);
  this->addCommand("HMULTI", &CommandParser::cmdMulti, UserFlags::OPER);
  this->addCommand("UMULTI", &CommandParser::cmdMulti, UserFlags::OPER);
  this->addCommand("VMULTI", &CommandParser::cmdMulti, UserFlags::OPER);
  this->addCommand("VBOTS", &CommandParser::cmdMulti, UserFlags::OPER);
  this->addCommand("CLONES", &CommandParser::cmdClones, UserFlags::OPER);
  this->addCommand("VCLONES", &CommandParser::cmdClones, UserFlags::OPER);
  this->addCommand("SEEDRAND", &CommandParser::cmdSeedrand, UserFlags::OPER);
  this->addCommand("DRONES", &CommandParser::cmdSeedrand, UserFlags::OPER);

  // UserFlags::MASTER commands
  this->addCommand("DIE", &CommandParser::cmdDie, UserFlags::MASTER,
    EXACT_ONLY);
  this->addCommand("CONN",
    boost::bind(&RemoteList::cmdConn, &remotes, _1, _2, _3), UserFlags::MASTER);
  this->addCommand("DISCONN",
    boost::bind(&RemoteList::cmdDisconn, &remotes, _1, _2, _3),
    UserFlags::MASTER);
  this->addCommand("RAW", &CommandParser::cmdRaw, UserFlags::MASTER,
    EXACT_ONLY);
  this->addCommand("SAVE", &CommandParser::cmdSave, UserFlags::MASTER);
  this->addCommand("LOAD", &CommandParser::cmdLoad, UserFlags::MASTER);
  this->addCommand("SPAMSUB", &CommandParser::cmdSpamsub, UserFlags::MASTER);
#ifdef CMDPARSER_DEBUG
  this->addCommand("TEST", &CommandParser::cmdTest, UserFlags::MASTER,
    EXACT_ONLY);
#endif /* CMDPARSER_DEBUG */
}


void
CommandParser::addCommand(const std::string & command, 
  CommandParser::CommandFunction func, const UserFlags flags,
  const int options)
{
  this->commands.push_back(Command(command, flags, options, func));
}


void
CommandParser::addCommand(const std::string & command,
  void (CommandParser::*func)(BotClient * from, const std::string & command,
  std::string parameters), const UserFlags flags, const int options)
{
  Command tmp(command, flags, options, boost::bind(func, this, _1, _2, _3));

  this->commands.push_back(tmp);
}


void
CommandParser::parse(BotClient * from, const std::string & command,
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
      cmd->func(from, cmd->name, parameters);
    }
    else
    {
      CommandParser::insufficient_privileges();
    }
  }
}


void
CommandParser::cmdWho(BotClient * from, const std::string & command,
  std::string parameters)
{
  clients.who(from);
}


void
CommandParser::cmdLinks(BotClient * from, const std::string & command,
  std::string parameters)
{
  remotes.getLinks(from);
}


void
CommandParser::cmdMotd(BotClient * from, const std::string & command,
  std::string parameters)
{
  ::motd(from);
}


void
CommandParser::cmdStatus(BotClient * from, const std::string & command,
  std::string parameters)
{
  users.status(from);
  ::status(from);
  server.status(from);
  clients.status(from);
  proxies.status(from);
  dnsbl.status(from);
}


void
CommandParser::cmdKline(BotClient * from, const std::string & command,
  std::string parameters)
{
  unsigned int minutes = 0;

  std::string parm = FirstWord(parameters);
  try
  {
    minutes = boost::lexical_cast<unsigned int>(parm);
    parm = FirstWord(parameters);
  }
  catch (boost::bad_lexical_cast)
  {
    minutes = vars[VAR_DEFAULT_KLINE_TIMEOUT]->getInt();
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
        ::SendAll("KLINE " + boost::lexical_cast<std::string>(minutes) + " " +
	  parm + " (" + reason + " [" + from->handleAndBot() + "])",
	  UserFlags::OPER, WATCH_KLINES, from);
      }
      else
      {
        ::SendAll("KLINE " + parm + " (" + reason + " [" +
	  from->handleAndBot() + "])", UserFlags::OPER, WATCH_KLINES, from);
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
CommandParser::cmdUnkline(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string target = FirstWord(parameters);

  if (!target.empty())
  {
    ::SendAll("UNKLINE " + target + " [" + from->handleAndBot() + "]",
      UserFlags::OPER, WATCH_KLINES, from);
    server.unkline(from->handleAndBot(), target);
  }
  else
  {
    throw CommandParser::exception("*** Please include a userhost mask to unkline.");
  }
}


void
CommandParser::cmdDline(BotClient * from, const std::string & command,
  std::string parameters)
{
  unsigned int minutes = vars[VAR_DEFAULT_DLINE_TIMEOUT]->getInt();

  std::string parm = FirstWord(parameters);
  try
  {
    minutes = boost::lexical_cast<unsigned int>(parm);
    parm = FirstWord(parameters);
  }
  catch (boost::bad_lexical_cast)
  {
    minutes = vars[VAR_DEFAULT_DLINE_TIMEOUT]->getInt();
  }

  if (!parm.empty())
  {
    if (!parameters.empty())
    {
      if (minutes)
      {
        ::SendAll("DLINE " + boost::lexical_cast<std::string>(minutes) + " " +
	  parm + " (" + parameters + " [" + from->handleAndBot() + "])",
	  UserFlags::OPER, WATCH_DLINES, from);
      }
      else
      {
        ::SendAll("DLINE " + parm + " (" + parameters + " [" +
	  from->handleAndBot() + "])", UserFlags::OPER, WATCH_DLINES, from);
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
CommandParser::cmdUndline(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string target = FirstWord(parameters);

  if (!target.empty())
  {
    ::SendAll("UNDLINE " + target + " [" + from->handleAndBot() + "]",
      UserFlags::OPER, WATCH_KLINES, from);
    server.undline(from->handleAndBot(), target);
  }
  else
  {
    throw CommandParser::exception("*** Please include an ip mask to undline.");
  }
}


void
CommandParser::cmdJoin(BotClient * from, const std::string & command,
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
CommandParser::cmdOp(BotClient * from, const std::string & command,
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
CommandParser::cmdPart(BotClient * from, const std::string & command,
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
CommandParser::cmdKill(BotClient * from, const std::string & command,
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
      parameters + ")", UserFlags::OPER, WATCH_KILLS, from);
    server.kill(from->handleAndBot(), nick, parameters);
  }
}


void
CommandParser::cmdKilllist(BotClient * from, const std::string & command,
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
    CommandParser::syntax(command, "[-class <name>] <pattern> [<reason>]");
  }
  else
  {
    PatternPtr pattern;

    try
    {
      if (args.haveUnary("-r"))
      {
        pattern.reset(new RegExPattern(mask));
      }
      else
      {
        pattern = smartPattern(mask, false);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }

    try
    {
      if (parameters.empty())
      {
        ::SendAll("KILLLIST " + mask + " [" + from->handleAndBot() + "]",
          UserFlags::OPER, WATCH_KILLS, from);
        users.listUsers(from, pattern, className, UserHash::LIST_KILL,
          from->handleAndBot(), vars[VAR_KILLLIST_REASON]->getString());
      }
      else
      {
        ::SendAll("KILLLIST " + mask + " (" + parameters + ") [" +
          from->handleAndBot() + "]", UserFlags::OPER, WATCH_KILLS, from);
        users.listUsers(from, pattern, className, UserHash::LIST_KILL,
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
CommandParser::cmdKillnfind(BotClient * from, const std::string & command,
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
    CommandParser::syntax(command, "[-class <name>] <pattern> [<reason>]");
  }
  else
  {
    PatternPtr pattern;

    try
    {
      if (args.haveUnary("-r"))
      {
        pattern.reset(new RegExPattern(mask));
      }
      else
      {
        pattern = smartPattern(mask, true);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }

    try
    {
      if (parameters.empty())
      {
        ::SendAll("KILLNFIND " + mask + " [" + from->handleAndBot() + "]",
          UserFlags::OPER, WATCH_KILLS, from);
        users.listNicks(from, pattern, className, UserHash::LIST_KILL,
          from->handleAndBot(), vars[VAR_KILLNFIND_REASON]->getString());
      }
      else
      {
        ::SendAll("KILLNFIND " + mask + " (" + parameters + ") [" +
          from->handleAndBot() + "]", UserFlags::OPER, WATCH_KILLS, from);
        users.listNicks(from, pattern, className, UserHash::LIST_KILL,
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
CommandParser::cmdReload(BotClient * from, const std::string & command,
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
CommandParser::cmdTrap(BotClient * from, const std::string & command,
  std::string parameters)
{
  TrapList::cmd(from, parameters);
}


void
CommandParser::cmdSet(BotClient * from, const std::string & command,
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
    if (!vars.get(from, varName) > 0)
    {
      from->send("No such variable \"" + varName + "\"");
    }
  }
  else
  {
    if (from->flags().has(UserFlags::MASTER))
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
CommandParser::cmdFindu(BotClient * from, const std::string & command,
  std::string parameters)
{
  try
  {
    if (0 == command.compare("findu"))
    {
      if (parameters.empty())
      {
        CommandParser::syntax(command, "<filter>");
      }
      else
      {
        Filter filter(parameters, Filter::FIELD_NUHG);

        users.findUsers(from, filter, false);
      }
    }
    else
    {
      ArgList args("-count");
      args.addPatterns("-class");

      if (-1 == args.parseCommand(parameters))
      {
        throw CommandParser::exception("*** Invalid parameter: " +
          args.getInvalid());
      }

      if (parameters.empty())
      {
        CommandParser::syntax(command, "[-count] [-class <pattern>] <pattern>");
      }
      else
      {
        std::string pattern(grabPattern(parameters));

        Filter::Field field(Filter::FIELD_NICK);
        if (0 == command.compare("ulist"))
        {
          field = Filter::FIELD_USER;
        }
        else if (0 == command.compare("hlist"))
        {
          field = Filter::FIELD_HOST;
        }
        else if (0 == command.compare("list"))
        {
          field = Filter::FIELD_UH;
        }
        else if (0 == command.compare("iplist"))
        {
          field = Filter::FIELD_IP;
        }
        else if (0 == command.compare("glist"))
        {
          field = Filter::FIELD_GECOS;
        }

        Filter filter(field, smartPattern(pattern,
              Filter::FIELD_NICK == field));

        PatternPtr classPattern;
        if (args.havePattern("-class", classPattern))
        {
          filter.add(Filter::FIELD_CLASS, classPattern);
        }

        users.findUsers(from, filter, args.haveUnary("-count"));
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    throw CommandParser::exception("*** RegEx error: " + e.what());
  }
  catch (Filter::bad_field & e)
  {
    throw CommandParser::exception(e.what());
  }
}


void
CommandParser::cmdFindk(BotClient * from, const std::string & command,
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
    if (!from->flags().has(UserFlags::KLINE))
    {
      CommandParser::insufficient_privileges();
    }
  }

  if (parameters.empty())
  {
    CommandParser::syntax(command,
      "[-count|-remove] [-reason] [-temp] [-perm] <pattern>");
  }
  else
  {
    try
    {
      PatternPtr pattern;

      if (args.haveUnary("-r"))
      {
        pattern.reset(new RegExPattern(parameters));
      }
      else
      {
        pattern = smartPattern(parameters, false);
      }

      if (args.haveUnary("-remove"))
      {
        int removed = server.findAndRemoveK(from->handleAndBot(), pattern.get(),
          searchPerms, searchTemps, searchReason);
        from->send("*** " + boost::lexical_cast<std::string>(removed) +
	  " K-lines removed.");
      }
      else
      {
        server.findK(from, pattern.get(), args.haveUnary("-count"),
	  searchPerms, searchTemps, searchReason);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdFindd(BotClient * from, const std::string & command,
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
    if (!from->flags().has(UserFlags::DLINE))
    {
      CommandParser::insufficient_privileges();
    }
  }

  if (parameters.empty())
  {
    CommandParser::syntax(command,
      "[-count|-remove] [-reason] [-temp] [-perm] <pattern>");
  }
  else
  {
    try
    {
      PatternPtr pattern;

      if (args.haveUnary("-r"))
      {
        pattern.reset(new RegExPattern(parameters));
      }
      else
      {
        pattern = smartPattern(parameters, false);
      }

      if (args.haveUnary("-remove"))
      {
        int removed = server.findAndRemoveD(from->handleAndBot(), pattern.get(),
          searchPerms, searchTemps, searchReason);
        from->send("*** " + boost::lexical_cast<std::string>(removed) +
	  " D-lines removed.");
      }
      else
      {
        server.findD(from, pattern.get(), args.haveUnary("-count"),
	  searchPerms, searchTemps, searchReason);
      }
    }
    catch (OOMon::regex_error & e)
    {
      throw CommandParser::exception("*** RegEx error: " + e.what());
    }
  }
}


void
CommandParser::cmdClass(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string className = FirstWord(parameters);

  users.reportClasses(from, className);
}


void
CommandParser::cmdDomains(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string min_str = FirstWord(parameters);
  int min = 5;

  if (!min_str.empty())
  {
    try
    {
      min = boost::lexical_cast<int>(min_str);
    }
    catch (boost::bad_lexical_cast)
    {
      // just ignore the error
    }
  }

  if (min >= 1)
  {
    if (command == "domains")
    {
      users.reportDomains(from, min);
    }
    else if (command == "nets")
    {
      users.reportNets(from, min);
    }
  }
  else
  {
    throw CommandParser::exception("*** Invalid minimum user size!");
  }
}


void
CommandParser::cmdMulti(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string min_str = FirstWord(parameters);

  int min;
  try
  {
    min = boost::lexical_cast<int>(min_str);
  }
  catch (boost::bad_lexical_cast)
  {
    min = 0;
  }

  if ((command == "multi") || (command == "bots"))
  {
    users.reportMulti(from, min);
  }
  else if (command == "hmulti")
  {
    users.reportHMulti(from, min);
  }
  else if (command == "umulti")
  {
    users.reportUMulti(from, min);
  }
  else if ((command == "vmulti") || (command == "vbots"))
  {
    users.reportVMulti(from, min);
  }
}


void
CommandParser::cmdClones(BotClient * from, const std::string & command,
  std::string parameters)
{
  if (0 == command.compare("vclones"))
  {
    users.reportVClones(from);
  }
  else
  {
    users.reportClones(from);
  }
}


void
CommandParser::cmdSeedrand(BotClient * from, const std::string & command,
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
    try
    {
      threshhold = boost::lexical_cast<int>(parm);
    }
    catch (boost::bad_lexical_cast)
    {
      threshhold = 0;
    }
  }

  try
  {
    PatternPtr pattern;

    if (parameters.empty())
    {
      pattern.reset(new NickClusterPattern("*"));
    }
    else if (args.haveUnary("-r"))
    {
      pattern.reset(new RegExPattern(parameters));
    }
    else
    {
      pattern = smartPattern(parameters, true);
    }

    users.reportSeedrand(from, pattern, threshhold,
      args.haveUnary("-count"));
  }
  catch (OOMon::regex_error & e)
  {
    throw CommandParser::exception("*** RegEx error: " + e.what());
  }
}


void
CommandParser::cmdDie(BotClient * from, const std::string & command,
  std::string parameters)
{
  from->send("As you wish...");
  if (!parameters.empty())
  {
    ::SendAll(from->handleAndBot() + " says I have to die now... (" +
      parameters + ")", UserFlags::NONE(), WatchSet(), from);
    Log::Write("DIE requested by " + from->handleAndBot() + " (" +
      parameters + ")");
    server.quit(parameters);
  }
  else
  {
    ::SendAll(from->handleAndBot() + " says I have to die now...",
      UserFlags::NONE(), WatchSet(), from);
    Log::Write("DIE requested by " + from->handleAndBot());
  }
  ::gracefuldie(SIGTERM); 
}


void
CommandParser::cmdRaw(BotClient * from, const std::string & command,
  std::string parameters)
{
  Log::Write("RAW by " + from->handleAndBot() + ": " + parameters);
  server.write(parameters + "\n");
}


void
CommandParser::cmdSave(BotClient * from, const std::string & command,
  std::string parameters)
{
  if (config.saveSettings())
  {
    std::string notice("*** Saved settings to file.");
    ::SendAll(notice, UserFlags::OPER);
    Log::Write(notice);
  }
  else
  {
    from->send("*** Error saving settings to file!");
  }
}


void
CommandParser::cmdLoad(BotClient * from, const std::string & command,
  std::string parameters)
{
  if (config.loadSettings())
  {
    std::string notice("*** Loaded settings from file.");
    ::SendAll(notice, UserFlags::OPER);
    Log::Write(notice);
  }
  else
  {
    from->send("*** Error loading settings from file!");
  }
}


void
CommandParser::cmdSpamsub(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string notice("*** " + from->handleAndBot() +
    " subscribing to SpamTrap");

  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);

  server.subSpamTrap(true);
}


void
CommandParser::cmdSpamunsub(BotClient * from, const std::string & command,
  std::string parameters)
{
  std::string notice("*** " + from->handleAndBot() +
    " unsubscribing from SpamTrap");

  ::SendAll(notice, UserFlags::OPER);
  Log::Write(notice);

  server.subSpamTrap(false);
}


void
CommandParser::cmdVersion(BotClient * from, const std::string & command,
  std::string parameters)
{
  from->send("OOMon version " OOMON_VERSION
#if defined(__DATE__) && defined(__TIME__)
    " (Built " __DATE__ " " __TIME__ ")"
#endif
    );
  from->send("Supported options:"
#if HAVE_CRYPT
    " crypt"
#endif
#if HAVE_LIBADNS
    " ADNS"
#endif
#if HAVE_LIBGDBM
    " GDBM"
#endif
#if HAVE_LIBPCRE
    " PCRE"
#endif
    );
}


void
CommandParser::cmdTest(BotClient * from, const std::string & command,
  std::string parameters)
{
  server.onServerNotice(parameters);
}

