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
#include <boost/utility.hpp>

// OOMon Headers
#include "oomon.h"
#include "strtype"
#include "botexcept.h"
#include "userflags.h"
#include "util.h"


class CommandParser : private boost::noncopyable
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

  void parse(class BotClient *from, const std::string & command,
    const std::string & parameters);

  typedef boost::function<void (class BotClient *, const std::string &,
    std::string)> CommandFunction;

  enum { NONE = 0, EXACT_ONLY = 1 } Option;

private:
  struct Command
  {
    Command(const std::string & name_, const class UserFlags flags_,
      const int options_, CommandParser::CommandFunction func_)
      : name(DownCase(name_)), flags(flags_), func(func_)
    {
      this->exactOnly = (options_ & EXACT_ONLY);
    }
    std::string name;
    UserFlags flags;
    bool exactOnly;
    CommandFunction func;
  };

  class partial_match
  {
  public:
    partial_match(const std::string & name) : name_(DownCase(name)),
      namelen_(name.length()) { }
    bool operator()(const Command & cmd)
    {
      bool result = false;
      if (!cmd.exactOnly && (this->namelen_ <= cmd.name.length()))
      {
        result = (this->name_ == cmd.name.substr(0, this->namelen_));
      }
      return result;
    }
  private:
    const std::string name_;
    const std::string::size_type namelen_;
  };
  class exact_match
  {
  public:
    exact_match(const std::string & name) : name_(DownCase(name)) { }
    bool operator()(const Command & cmd)
    {
      return (this->name_ == cmd.name);
    }
  private:
    const std::string name_;
  };
  typedef std::vector<Command> CommandVector;
  CommandVector commands;

public:
  void addCommand(const std::string & command, 
    CommandParser::CommandFunction func, const class UserFlags flags,
    const int options = NONE);

private:
  void addCommand(const std::string & command,
    void (CommandParser::*)(class BotClient *from, const std::string & command,
    std::string parameters), const class UserFlags flags,
    const int options = NONE);

  void cmdWho(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdLinks(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdMotd(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdStatus(class BotClient *from, const std::string & command,
    std::string parameters);

  void cmdJoin(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdOp(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdPart(class BotClient *from, const std::string & command,
    std::string parameters);

  void cmdKill(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdKilllist(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdKillnfind(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdReload(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdTrap(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdSet(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdNfind(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdList(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdGlist(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdFindk(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdFindd(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdClass(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdDomains(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdMulti(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdClones(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdSeedrand(class BotClient *from, const std::string & command,
    std::string parameters);

  void cmdKline(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdUnkline(class BotClient *from, const std::string & command,
    std::string parameters);

  void cmdDline(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdUndline(class BotClient *from, const std::string & command,
    std::string parameters);

  void cmdDie(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdRaw(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdSave(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdLoad(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdSpamsub(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdSpamunsub(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdVersion(class BotClient *from, const std::string & command,
    std::string parameters);
  void cmdTest(class BotClient *from, const std::string & command,
    std::string parameters);
};

#endif /* __CMDPARSER_H__ */

