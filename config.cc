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
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

// OOMon Headers
#include "config.h"
#include "util.h"
#include "help.h"
#include "trap.h"
#include "vars.h"
#include "pattern.h"
#include "botsock.h"
#include "irc.h"
#include "log.h"
#include "botexcept.h"
#include "userflags.h"
#include "botclient.h"


#ifdef DEBUG
# define CONFIG_DEBUG
#endif


#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif


struct Config::Oper
{
  Oper(const std::string & handle_, const PatternPtr & pattern_,
      const std::string & password_, const UserFlags & flags_)
    : handle(handle_), password(password_), pattern(pattern_), flags(flags_) { }
  const std::string handle, password;
  const PatternPtr pattern;
  const UserFlags flags;
};


struct Config::Link
{
  Link(const std::string & handle_, const PatternPtr & pattern_,
    const std::string & password_) : handle(handle_), password(password_),
    pattern(pattern_) { }
  const std::string handle, password;
  const PatternPtr pattern;
};


struct Config::Connect
{
  Connect(const std::string & handle_, const std::string & address_,
      const std::string & password_, const BotSock::Port & port_)
    : handle(handle_), address(address_), password(password_), port(port_) { }
  const std::string handle, address, password;
  const BotSock::Port port;
};


struct Config::Remote
{
  Remote(const PatternPtr pattern_, const UserFlags flags_)
    : pattern(pattern_), flags(flags_) { }
  const PatternPtr pattern;
  const UserFlags flags;
};


struct Config::Exempt
{
  Exempt(const PatternPtr pattern_, const Config::ExemptFlags flags_)
    : pattern(pattern_), flags(flags_) { }
  const PatternPtr pattern;
  const Config::ExemptFlags flags;
};


struct Config::Parser
{
  Parser(const StrVector::size_type args_,
      const Config::ParserFunction function_)
    : args(args_), function(function_) { }
  const StrVector::size_type args;
  const Config::ParserFunction function;
};


class Config::syntax_error : public OOMon::oomon_error
{
  public:
    syntax_error(const std::string & arg) : oomon_error(arg) { };
};


class Config::bad_port_number : public Config::syntax_error
{
  public:
    bad_port_number(void) : syntax_error("bad port number") { };
};


Config config;


Config::Config(void) : proxyTargetAddress_(INADDR_NONE)
{
  this->initialize();
}


Config::Config(const std::string & filename) : proxyTargetAddress_(INADDR_NONE)
{
  this->initialize();
  this->parse(filename);
}


Config::~Config(void)
{
}


void
Config::initialize(void)
{
  this->remotePort_ = DEFAULT_REMOTE_PORT;
  this->dccPort_ = DEFAULT_DCC_PORT;
  this->logFilename_ = ::expandPath(DEFAULT_LOGFILE, LOGDIR);
  this->motdFilename_ = ::expandPath(DEFAULT_MOTDFILE, ETCDIR);
  this->helpFilename_ = ::expandPath(DEFAULT_HELPFILE, ETCDIR);
  this->settingsFilename_ = ::expandPath(DEFAULT_SETTINGSFILE, ETCDIR);
  this->userDBFilename_ = ::expandPath(DEFAULT_USERDBFILE, ETCDIR);

  Help::flush();

  this->addParser("B", 6, boost::bind(&Config::parseBLine, this, _1));
  this->addParser("C", 4, boost::bind(&Config::parseCLine, this, _1));
  this->addParser("D", 1, boost::bind(&Config::parseDLine, this, _1));
  this->addParser("E", 1, boost::bind(&Config::parseELine, this, _1));
  this->addParser("EC", 1, boost::bind(&Config::parseEcLine, this, _1));
  this->addParser("F", 1, boost::bind(&Config::parseFLine, this, _1));
  this->addParser("G", 1, boost::bind(&Config::parseGLine, this, _1));
  this->addParser("H", 1, boost::bind(&Config::parseHLine, this, _1));
  this->addParser("I", 1, boost::bind(&Config::parseILine, this, _1));
  this->addParser("L", 3, boost::bind(&Config::parseLLine, this, _1));
  this->addParser("M", 1, boost::bind(&Config::parseMLine, this, _1));
  this->addParser("O", 4, boost::bind(&Config::parseOLine, this, _1));
  this->addParser("P", 1, boost::bind(&Config::parsePLine, this, _1));
  this->addParser("PROXY-MATCH", 1, boost::bind(&Config::parseProxyMatchLine,
        this, _1));
  this->addParser("PROXY-SEND", 1, boost::bind(&Config::parseProxySendLine,
        this, _1));
  this->addParser("PROXY-TARGET", 2, boost::bind(&Config::parseProxyTargetLine,
        this, _1));
  this->addParser("PROXY-VHOST", 1, boost::bind(&Config::parseProxyVhostLine,
        this, _1));
  this->addParser("R", 2, boost::bind(&Config::parseRLine, this, _1));
  this->addParser("S", 4, boost::bind(&Config::parseSLine, this, _1));
  this->addParser("T", 1, boost::bind(&Config::parseTLine, this, _1));
  this->addParser("U", 1, boost::bind(&Config::parseULine, this, _1));
  this->addParser("W", 1, boost::bind(&Config::parseProxyVhostLine, this, _1));
  this->addParser("Y", 2, boost::bind(&Config::parseYLine, this, _1));
}


void
Config::addParser(const std::string & key, const StrVector::size_type args,
    const Config::ParserFunction function)
{
  boost::shared_ptr<Config::Parser> parser(new Config::Parser(args, function));
  ParserMap::value_type node(UpCase(key), parser);

  this->parser_.insert(node);
}


void
Config::parse(const std::string & filename)
{
  unsigned int line(1);

  try
  {
    std::ifstream file(filename.c_str());
    if (file.good())
    {
      while (file.good())
      {
        std::string text;
        std::getline(file, text);
        text = trimLeft(text);

        if (!text.empty() && (text[0] != '#'))
        {
          StrVector fields;
          StrSplit(fields, text, ":");

          Config::ParserMap::iterator pos =
            this->parser_.find(UpCase(fields[0]));
          if (pos != this->parser_.end())
          {
            fields.erase(fields.begin());
            if (fields.size() >= pos->second->args)
            {
              pos->second->function(fields);
            }
            else
            {
              throw Config::syntax_error("not enough fields");
            }
          }
          else
          {
            throw Config::syntax_error("unknown line type '" + fields[0] + "'");
          }
        }
        ++line;
      }

      file.close();
    }
    else
    {
      std::string result("Failed to open ");
      result += filename;
      result += " for reading!";
      throw Config::parse_failed(result);
    }
  }
  catch (Config::syntax_error & e)
  {
    std::string result("Syntax error in ");
    result += filename;
    result += " at line ";
    result += boost::lexical_cast<std::string>(line);
    result += ": ";
    result += e.what();
    throw Config::parse_failed(result);
  }
  catch (OOMon::regex_error & e)
  {
    std::string result("Regex error in ");
    result += filename;
    result += " at line ";
    result += boost::lexical_cast<std::string>(line);
    result += ": ";
    result += e.what();
    throw Config::parse_failed(result);
  }
}


void
Config::parseBLine(const StrVector & fields)
{
  this->nick_ = fields[0];
  if (!IRC::validNick(this->nick_))
  {
    throw Config::syntax_error("bad nickname '" + fields[0] + "'");
  }

  this->username_ = fields[1];
  this->hostname_ = fields[2];
  this->realName_ = fields[3];

  this->operName_ = fields[4];
  if (this->operName_.empty())
  {
    throw Config::syntax_error("bad oper name '" + fields[4] + "'");
  }

  this->operPassword_ = fields[5];
  if (this->operPassword_.empty())
  {
    throw Config::syntax_error("no oper password");
  }
}


void
Config::parseCLine(const StrVector & fields)
{
  std::string handle(fields[0]);
  if (!IRC::validNick(handle))
  {
    throw Config::syntax_error("bad bot handle '" + handle + "'");
  }
  std::string address(fields[1]);
  if (address.empty())
  {
    throw Config::syntax_error("no bot address");
  }
  std::string password(fields[2]);
  if (password.empty())
  {
    throw Config::syntax_error("no password");
  }
  try
  {
    BotSock::Port port(boost::lexical_cast<BotSock::Port>(fields[3]));
    boost::shared_ptr<Config::Connect> connect(new Config::Connect(handle,
          address, password, port));
    this->connects_.insert(Config::ConnectMap::value_type(UpCase(handle),
          connect));
  }
  catch (boost::bad_lexical_cast)
  {
    throw Config::bad_port_number();
  }
}


void
Config::parseDLine(const StrVector & fields)
{
  try
  {
    this->dccPort_ = boost::lexical_cast<BotSock::Port>(fields[0]);
  }
  catch (boost::bad_lexical_cast)
  {
    throw Config::bad_port_number();
  }
}


void
Config::parseELine(const StrVector & fields)
{
  PatternPtr pattern;
  ExemptFlags flags;

  if (fields.size() == 1)
  {
    flags.set();
    pattern = smartPattern(fields[0], false);
  }
  else
  {
    flags = Config::exemptFlags(fields[0]);
    pattern = smartPattern(fields[1], false);
  }

  boost::shared_ptr<Config::Exempt> exempt(new Config::Exempt(pattern, flags));
  this->exempts_.push_back(exempt);
}


void
Config::parseEcLine(const StrVector & fields)
{
  PatternPtr pattern;
  ExemptFlags flags;

  if (fields.size() == 1)
  {
    flags.set();
    pattern = smartPattern(fields[0], false);
  }
  else
  {
    flags = Config::exemptFlags(fields[0]);
    pattern = smartPattern(fields[1], false);
  }

  boost::shared_ptr<Config::Exempt> exempt(new Config::Exempt(pattern, flags));
  this->classExempts_.push_back(exempt);
}


void
Config::parseFLine(const StrVector & fields)
{
  PatternPtr pattern(smartPattern(fields[0], false));
  this->spoofers_.push_back(pattern);
}


void
Config::parseGLine(const StrVector & fields)
{
  this->logFilename_ = ::expandPath(fields[0], LOGDIR);
}


void
Config::parseHLine(const StrVector & fields)
{
  this->helpFilename_ = ::expandPath(fields[0], ETCDIR);
}


void
Config::parseILine(const StrVector & fields)
{
  this->parse(::expandPath(fields[0], ETCDIR));
}


void
Config::parseLLine(const StrVector & fields)
{
  std::string handle(fields[0]);
  if (!IRC::validNick(handle))
  {
    throw Config::syntax_error("bad bot handle '" + handle + "'");
  }

  PatternPtr pattern(smartPattern(fields[1], false));

  std::string password(fields[2]);
  if (password.empty())
  {
    throw Config::syntax_error("no password");
  }

  boost::shared_ptr<Config::Link> link(new Config::Link(handle, pattern,
        password));

  this->links_.insert(Config::LinkMap::value_type(UpCase(handle), link));
}


void
Config::parseMLine(const StrVector & fields)
{
  this->motdFilename_ = ::expandPath(fields[0], ETCDIR);
}


void
Config::parseOLine(const StrVector & fields)
{
  std::string handle(fields[0]);
  if (!IRC::validNick(handle))
  {
    throw Config::syntax_error("bad user handle '" + handle + "'");
  }

  PatternPtr pattern(smartPattern(fields[1], false));

  std::string password(fields[2]);
  if (password.empty())
  {
    throw Config::syntax_error("no password");
  }

  UserFlags flags(Config::userFlags(fields[3]));

  boost::shared_ptr<Config::Oper> oper(new Config::Oper(handle, pattern,
        password, flags));

  this->opers_.insert(Config::OperMap::value_type(UpCase(handle), oper));
}


void
Config::parsePLine(const StrVector & fields)
{
  try
  {
    this->remotePort_ = boost::lexical_cast<BotSock::Port>(fields[0]);
  }
  catch (boost::bad_lexical_cast)
  {
    throw Config::bad_port_number();
  }
}


void
Config::parseProxyMatchLine(const StrVector & fields)
{
  PatternPtr pattern(smartPattern(fields[0], false));
  this->proxyMatches_.push_back(pattern);
}


void
Config::parseProxySendLine(const StrVector & fields)
{
  std::string tmp;

  // Reconstruct the original line
  for (StrVector::const_iterator pos = fields.begin(); pos != fields.end();
      ++pos)
  {
    if (!tmp.empty())
    {
      tmp += ':';
    }

    tmp += *pos;
  }
  this->proxySendLines_.push_back(tmp);
}


void
Config::parseProxyTargetLine(const StrVector & fields)
{
  this->proxyTargetAddress_ = BotSock::inet_addr(fields[0]);
  if (INADDR_NONE == this->proxyTargetAddress_)
  {
    throw Config::syntax_error("bad address '" + fields[0] + "'");
  }

  try
  {
    this->proxyTargetPort_ = boost::lexical_cast<BotSock::Address>(fields[1]);
  }
  catch (boost::bad_lexical_cast)
  {
    throw Config::bad_port_number();
  }
}


void
Config::parseProxyVhostLine(const StrVector & fields)
{
  this->proxyVhost_ = fields[0];
}


void
Config::parseRLine(const StrVector & fields)
{
  PatternPtr pattern(smartPattern(fields[0], false));
  UserFlags flags(Config::userFlags(fields[1], true));

  boost::shared_ptr<Config::Remote> remote(new Config::Remote(pattern, flags));

  this->remotes_.push_back(remote);
}


void
Config::parseSLine(const StrVector & fields)
{
  this->serverAddress_ = fields[0];
  if (serverAddress_.empty())
  {
    throw Config::syntax_error("no server address");
  }

  try
  {
    this->serverPort_ = boost::lexical_cast<BotSock::Port>(fields[1]);
  }
  catch (boost::bad_lexical_cast)
  {
    throw Config::bad_port_number();
  }

  this->serverPassword_ = fields[2];
  this->channels_ = fields[3];
}


void
Config::parseTLine(const StrVector & fields)
{
  this->settingsFilename_ = ::expandPath(fields[0], ETCDIR);
}


void
Config::parseULine(const StrVector & fields)
{
  this->userDBFilename_ = ::expandPath(fields[0], ETCDIR);
}


void
Config::parseYLine(const StrVector & fields)
{
  std::string name(fields[0]);
  if (name.empty())
  {
    throw Config::syntax_error("no class name");
  }

  this->ylines_.insert(YLineMap::value_type(UpCase(name), fields[1]));
}


// authUser(handle, userhost, password, flags, newHandle)
//
// Attempts to receive authorization.
//
// Handle and Password should be entered by the user.  The UserHost is taken
// from the user's address.  Flags and NewHandle are returned to the caller
// as defined in the configuration file.
//
// Returns true if authorization is granted, false otherwise.
//
// Note that Flags and NewHandle are NOT modified if authorization fails.
//
bool
Config::authUser(const std::string & handle, const std::string & userhost,
    const std::string & password, UserFlags & flags,
    std::string & newHandle) const
{
#ifdef CONFIG_DEBUG
  std::cout << "Config::authUser(handle = \"" << handle <<
    "\", userhost = \"" << userhost << "\", password = \"" << password <<
    "\", ...)" << std::endl;
#endif
  try
  {
    const std::string key(UpCase(handle));

    for (Config::OperMap::const_iterator pos = this->opers_.lower_bound(key);
        pos != this->opers_.upper_bound(key); ++pos)
    {
      if (Same(pos->second->handle, handle) &&
          pos->second->pattern->match(userhost))
      {
#ifdef CONFIG_DEBUG
        std::cout << "Config::authUser(): found match" << std::endl;
#endif
        if (ChkPass(pos->second->password, password))
        {
#ifdef CONFIG_DEBUG
          std::cout << "Config::authUser(): password is correct" << std::endl;
#endif
	  flags = UserFlags::AUTHED;
          newHandle = pos->second->handle;
          flags |= pos->second->flags;
          return true;
        }
        else
        {
#ifdef CONFIG_DEBUG
          std::cout << "Config::authUser(): incorrect password" << std::endl;
#endif
        }
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::authUser(): " + e.what());
    std::cerr << "RegEx error in Config::authUser(): " << e.what() << std::endl;
  }
  return false;
}


std::string
Config::username(void) const
{
  std::string result = this->username_;

  if (result.empty())
  {
    char *login = getlogin();

    if (login && *login)
    {
      result = login;
    }
    else
    {
      result = "oomon";
    }
  }

  return result;
}


bool
Config::mayChat(const std::string & userhost) const
{
  bool result(false);

  if (vars[VAR_OPER_ONLY_DCC]->getBool())
  {
    try
    {
      for (Config::OperMap::const_iterator pos = this->opers_.begin();
          pos != this->opers_.end(); ++pos)
      {
        if (pos->second->pattern->match(userhost))
        {
          result = true;
          break;
        }
      }
    }
    catch (OOMon::regex_error & e)
    {
      Log::Write("RegEx error in Config::mayChat(): " + e.what());
      std::cerr << "RegEx error in Config::mayChat(): " << e.what() <<
                                                           std::endl;
    }
  }
  else
  {
    result = true;
  }

  return result;
}


bool
Config::mayChat(const std::string & userhost, const BotSock::Address & ip)
  const
{
  std::string user = userhost.substr(0, userhost.find('@'));
  std::string userip = user + "@" + BotSock::inet_ntoa(ip);

  return this->mayChat(userhost) || this->mayChat(userip);
}


bool
Config::isExempt(const std::string & userhost, const Config::ExemptFlag flag)
  const
{
  try
  {
    for (Config::ExemptList::const_iterator pos = this->exempts_.begin();
        pos != this->exempts_.end(); ++pos)
    {
      Config::ExemptList::value_type exempt(*pos);

      if (exempt->flags.test(flag) && exempt->pattern->match(userhost))
      {
        return true;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::isExempt(): " + e.what());
    std::cerr << "RegEx error in Config::isExempt(): " << e.what() <<
                                                            std::endl;
  }
  return false;
}


bool
Config::isExempt(const std::string & userhost, const std::string & ip,
    const Config::ExemptFlag flag) const
{
  if (ip.empty())
  {
    return this->isExempt(userhost, flag);
  }
  else
  {
    std::string user = userhost.substr(0, userhost.find('@'));
    std::string userip = user + "@" + ip;

    return this->isExempt(userhost, flag) || this->isExempt(userip, flag);
  }
}


bool
Config::isExempt(const std::string & userhost, const BotSock::Address & ip,
    const Config::ExemptFlag flag) const
{
  return this->isExempt(userhost, BotSock::inet_ntoa(ip), flag);
}


bool
Config::isExempt(const UserEntryPtr & user, const Config::ExemptFlag flag) const
{
  return (this->isExempt(user->getUserHost(), user->getIP(), flag) ||
      this->isExemptClass(user->getClass(), flag));
}


bool
Config::isExemptClass(const std::string & name, const Config::ExemptFlag flag)
  const
{
  try
  {
    for (Config::ExemptList::const_iterator pos = this->classExempts_.begin();
        pos != this->classExempts_.end(); ++pos)
    {
      Config::ExemptList::value_type exempt(*pos);

      if (exempt->flags.test(flag) && exempt->pattern->match(name))
      {
        return true;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::isExemptClass(): " + e.what());
    std::cerr << "RegEx error in Config::isExemptClass(): " << e.what() <<
                                                                 std::endl;
  }
  return false;
}


bool
Config::isOper(const std::string & userhost) const
{
  try
  {
    for (Config::OperMap::const_iterator pos = this->opers_.begin();
        pos != this->opers_.end(); ++pos)
    {
      if (pos->second->pattern->match(userhost))
      {
        return pos->second->flags.has(UserFlags::OPER);
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::isOper(): " + e.what());
    std::cerr << "RegEx error in Config::isOper(): " << e.what() << std::endl;
  }
  return false;
}


bool
Config::isOper(const std::string & userhost, const std::string & ip) const
{
  if (ip.empty())
  {
    return this->isOper(userhost);
  }
  else
  {
    std::string user = userhost.substr(0, userhost.find('@'));
    std::string userip = user + "@" + ip;

    return this->isOper(userhost) || this->isOper(userip);
  }
}


bool
Config::isOper(const std::string & userhost, const BotSock::Address & ip) const
{
  return this->isOper(userhost, BotSock::inet_ntoa(ip));
}


bool
Config::isOper(const UserEntryPtr & user) const
{
  return this->isOper(user->getUserHost(), user->getIP());
}


bool
Config::linkable(const std::string & host) const
{
  bool result(false);

  try
  {
    for (Config::LinkMap::const_iterator pos = this->links_.begin();
        pos != this->links_.end(); ++pos)
    {
      if (pos->second->pattern->match(host))
      {
        result = true;
        break;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::linkable(): " + e.what());
    std::cerr << "RegEx error in Config::linkable(): " << e.what() <<
                                                          std::endl;
  }
  return result;
}


bool
Config::connect(std::string & handle, std::string & address,
    BotSock::Port & port, std::string & password) const
{
  Config::ConnectMap::const_iterator pos = this->connects_.find(UpCase(handle));
  bool result(false);

  if (pos != this->connects_.end())
  {
    handle = pos->second->handle;
    address = pos->second->address;
    port = pos->second->port;
    password = pos->second->password;
    result = true;
  }

  return result;
}


bool
Config::authBot(const std::string & handle, const std::string & address,
    const std::string & password) const
{
  bool result(false);

  try
  {
    std::string key(UpCase(handle));

    for (Config::LinkMap::const_iterator pos = this->links_.lower_bound(key);
        pos != this->links_.upper_bound(key); ++pos)
    {
      if (pos->second->pattern->match(address) &&
          (pos->second->password == password))
      {
        result = true;
        break;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::authBot(): " + e.what());
    std::cerr << "RegEx error in Config::authBot(): " << e.what() << std::endl;
  }

  return result;
}


std::string
Config::classDescription(const std::string & name) const
{
  std::string result;

  Config::YLineMap::const_iterator pos = this->ylines_.find(UpCase(name));
  if (pos != this->ylines_.end())
  {
    result = pos->second;
  }
  return result;
}


UserFlags
Config::remoteFlags(const std::string & client) const
{
  UserFlags flags(UserFlags::NONE());

  try
  {
    for (Config::RemoteList::const_iterator pos = this->remotes_.begin();
        pos != this->remotes_.end(); ++pos)
    {
      if ((*pos)->pattern->match(client))
      {
        flags |= UserFlags::AUTHED;
        flags |= (*pos)->flags;
        break;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    std::cerr << "Regex error: " << e.what() << std::endl;
  }

  return flags;
}


bool
Config::isOpenProxyLine(const std::string & text) const
{
  bool result(false);

  try
  {
    for (Config::PatternList::const_iterator pos = this->proxyMatches_.begin();
        pos != this->proxyMatches_.end(); ++pos)
    {
      if ((*pos)->match(text))
      {
        result = true;
        break;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::isOpenProxyLine(): " + e.what());
    std::cerr << "RegEx error in Config::isOpenProxyLine(): " << e.what() <<
                                                                 std::endl;
  }

  return result;
}


StrVector
Config::proxySendLines(void) const
{
  return this->proxySendLines_;
}


BotSock::Address
Config::proxyTargetAddress(void) const
{
  return this->proxyTargetAddress_;
}


BotSock::Address
Config::proxyTargetPort(void) const
{
  return this->proxyTargetPort_;
}


std::string
Config::proxyVhost(void) const
{
  return this->proxyVhost_.empty() ? this->hostname() : this->proxyVhost_;
}


bool
Config::spoofer(const std::string & ip) const
{
  bool result(false);

  try
  {
    for (Config::PatternList::const_iterator pos = this->spoofers_.begin();
        pos != this->spoofers_.end(); ++pos)
    {
      if ((*pos)->match(ip))
      {
        result = true;
        break;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::spoofer(): " + e.what());
    std::cerr << "RegEx error in Config::spoofer(): " << e.what() << std::endl;
  }

  return result;
}


bool
Config::saveSettings(void) const
{
  std::ofstream file(this->settingsFilename().c_str());

  if (file)
  {
    // Write header
    file << "# This file was generated by OOMon-" OOMON_VERSION << std::endl;
    file << "#" << std::endl;
    file << "# Please do not modify this file directly!" << std::endl;
    file << "#" << std::endl;

    // Save TRAP list
    TrapList::save(file);

    // Save SET variables
    vars.save(file);

    // Done!
    file.close();

    return true;
  }
  else
  {
    return false;
  }
}


class ConfigClient : public BotClient
{
public:
  ConfigClient(void) { }

  virtual std::string handle(void) const { return std::string(); }
  virtual std::string bot(void) const { return std::string(); }
  virtual std::string id(void) const { return "CONFIG"; }
  virtual UserFlags flags(void) const { return UserFlags::ALL(); }
  virtual void send(const std::string & text)
  {
    std::cerr << text << std::endl;
  }
};


bool
Config::loadSettings(void)
{
  bool result = true;

  std::ifstream file(this->settingsFilename().c_str());

  if (file)
  {
    // Prepare TRAP list for load
    TrapList::preLoad();

    std::string line;

    while (std::getline(file, line))
    {
      std::string cmd = UpCase(FirstWord(line));

      if ((cmd.length() > 0) && (cmd[0] == '#'))
      {
	// Just ignore the comments
#ifdef CONFIG_DEBUG
        std::cout << "Ignoring comments" << std::endl;
#endif
      }
      else if (cmd == "TRAP")
      {
	ConfigClient client;

	TrapList::cmd(&client, line);

#ifdef CONFIG_DEBUG
        std::cout << "Added trap: " << line << std::endl;
#endif
      }
      else if (cmd == "SET")
      {
	std::string varName = FirstWord(line);

	vars.set(varName, line);

#ifdef CONFIG_DEBUG
        std::cout << "Set: " << varName << " = " << line << std::endl;
#endif
      }
      else
      {
        std::cerr << "Unknown setting type: " << cmd << std::endl;
	result = false;
        break;
      }
    }

    // Done!
    file.close();

    // Perform post-loading actions on TRAP list
    TrapList::postLoad();
  }
  else
  {
     result = false;
  }

  return result;
}


//////////////////////////////////////////////////////////////////////
// haveChannel(channel)
//
// Description:
//  Determines whether a channel is listed in the bot's config file.
//
// Parameters:
//  channel - The channel to look for in the config file.
//
// Return Value:
//  The function returns true if the channel is listed in the config
//  file and false otherwise.
//////////////////////////////////////////////////////////////////////
bool
Config::haveChannel(const std::string & channel) const
{
  StrVector channels;

  StrSplit(channels, server.downCase(this->channels_), ",");

  return (channels.end() != std::find(channels.begin(), channels.end(),
    server.downCase(channel)));
}


UserFlags
Config::userFlags(const std::string & text, const bool remote)
{
  UserFlags result(UserFlags::NONE());

  for (std::string::const_iterator pos = text.begin(); pos != text.end(); ++pos)
  {
    switch (UpCase(*pos))
    {
      case '\t':
      case ' ':
        // Ignore whitespace
        break;

      case 'C':
        result |= UserFlags::CHANOP;
        break;

      case 'D':
        result |= UserFlags::DLINE;
        break;

      case 'G':
        result |= UserFlags::GLINE;
        break;

      case 'K':
        result |= UserFlags::KLINE;
        break;

      case 'M':
        result |= UserFlags::MASTER;
        break;

      case 'N':
        result |= UserFlags::NICK;
        break;

      case 'O':
        result |= UserFlags::OPER;
        break;

      case 'W':
        result |= UserFlags::WALLOPS;
        break;

      case 'X':
        result |= UserFlags::CONN;
        break;

      case 'R':
        if (!remote)
        {
          result |= UserFlags::REMOTE;
          break;
        }

      default:
        throw Config::syntax_error("bad user flag '" + std::string(1, *pos) +
            "'");
    }
  }

  return result;
}


Config::ExemptFlags
Config::exemptFlags(const std::string & text)
{
  if (text.empty())
  {
    throw Config::syntax_error("no exempt flags");
  }

  ExemptFlags result;

  StrVector flags;
  StrSplit(flags, text, " ,", true);

  for (StrVector::iterator pos = flags.begin(); pos != flags.end(); ++pos)
  {
    std::string flag(UpCase(*pos));

    if (0 == flag.compare("ALL"))
    {
      result.set();
    }
    else if (0 == flag.compare("CLONE"))
    {
      result.set(Config::EXEMPT_CLONE);
    }
    else if (0 == flag.compare("FLOOD"))
    {
      result.set(Config::EXEMPT_FLOOD);
    }
    else if (0 == flag.compare("SPOOF"))
    {
      result.set(Config::EXEMPT_SPOOF);
    }
    else if (0 == flag.compare("TRAP"))
    {
      result.set(Config::EXEMPT_TRAP);
    }
    else if (0 == flag.compare("PROXY"))
    {
      result.set(Config::EXEMPT_PROXY);
    }
    else if (0 == flag.compare("JUPE"))
    {
      result.set(Config::EXEMPT_JUPE);
    }
    else if (0 == flag.compare("SEEDRAND"))
    {
      result.set(Config::EXEMPT_SEEDRAND);
    }
    else if (0 == flag.compare("VERSION"))
    {
      result.set(Config::EXEMPT_VERSION);
    }
    else
    {
      throw Config::syntax_error("bad exempt flag '" + *pos + "'");
    }
  }

  return result;
}

