#ifndef __CMDPARSER_H__
#define __CMDPARSER_H__
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
#include <vector>
#include <string>

// Boost C++ Headers
#include <boost/function.hpp>

// OOMon Headers
#include "oomon.h"
#include "strtype"
#include "botexcept.h"
#include "botclient.h"
#include "util.h"


class CommandParser
{
public:
  CommandParser(void);
  virtual ~CommandParser(void) { }

  class exception : public OOMon::oomon_error
  {
  public:
    exception(const std::string & arg) : oomon_error(arg) { };
  };
  static void insufficient_privileges(void) throw(CommandParser::exception)
  {
    throw CommandParser::exception("*** Insufficient privileges!");
  }
  static void syntax(const std::string & command, const std::string & args)
    throw(CommandParser::exception)
  {
    throw CommandParser::exception("*** Syntax: ." + command + " " + args);
  }

  void parse(BotClient::ptr from, const std::string & command,
    const std::string & parameters);

  typedef boost::function<void (BotClient::ptr, const std::string &,
    std::string)> CommandFunction;

  enum { NONE = 0, EXACT_ONLY = 1 } Option;

private:
  struct Command
  {
    Command(const std::string & _name, const UserFlags _flags,
      const int _options, CommandParser::CommandFunction _func)
      : name(DownCase(_name)), flags(_flags), func(_func)
    {
      this->exactOnly = (_options & EXACT_ONLY);
    }
    std::string name;
    UserFlags flags;
    bool exactOnly;
    CommandFunction func;
  };

  class partial_match
  {
  public:
    partial_match(const std::string & name) : _name(DownCase(name)),
      _namelen(name.length()) { }
    bool operator()(const Command & cmd)
    {
      bool result = false;
      if (!cmd.exactOnly && (this->_namelen <= cmd.name.length()))
      {
        result = (this->_name == cmd.name.substr(0, this->_namelen));
      }
      return result;
    }
  private:
    const std::string _name;
    const std::string::size_type _namelen;
  };
  class exact_match
  {
  public:
    exact_match(const std::string & name) : _name(DownCase(name)) { }
    bool operator()(const Command & cmd)
    {
      return (this->_name == cmd.name);
    }
  private:
    const std::string _name;
  };
  typedef std::vector<Command> CommandVector;
  CommandVector commands;

public:
  void addCommand(const std::string & command, 
    CommandParser::CommandFunction func, const UserFlags flags,
    const int options = NONE);

private:
  void addCommand(const std::string & command,
    void (CommandParser::*)(BotClient::ptr from, const std::string & command,
    std::string parameters), const UserFlags flags, const int options = NONE);

  void cmdWho(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdLinks(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdMotd(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdStatus(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void cmdJoin(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdOp(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdPart(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void cmdKill(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdKilllist(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdKillnfind(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdReload(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdTrap(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdSet(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdNfind(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdList(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdGlist(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdFindk(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdFindd(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdClass(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdDomains(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdMulti(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdClones(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdSeedrand(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void cmdKline(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdUnkline(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void cmdDline(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdUndline(BotClient::ptr from, const std::string & command,
    std::string parameters);

  void cmdDie(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdConn(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdDisconn(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdRaw(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdSave(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdLoad(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdSpamsub(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdSpamunsub(BotClient::ptr from, const std::string & command,
    std::string parameters);
  void cmdTest(BotClient::ptr from, const std::string & command,
    std::string parameters);
};

#endif /* __CMDPARSER_H__ */

