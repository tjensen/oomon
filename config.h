#ifndef __CONFIG_H__
#define __CONFIG_H__
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

// C++ Headers
#include <string>
#include <list>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "botsock.h"
#include "pattern.h"

// User flags
enum UserFlags
{
  UF_NONE	= 0x000,
  UF_AUTHED	= 0x001,
  UF_CHANOP	= 0x002,
  UF_OPER	= 0x004,
  UF_WALLOPS	= 0x008,
  UF_CONN	= 0x010,
  UF_NICK	= 0x020,
  UF_KLINE	= 0x040,
  UF_GLINE	= 0x080,
  UF_DLINE	= 0x100,
  UF_REMOTE	= 0x200,
  UF_MASTER	= 0x400,
  UF_ALL	= 0x7FF
};

// Link flags
enum LinkFlags
{
  LF_NONE	= 0x0000
};

// Connect flags
enum ConnectFlags
{
  CF_NONE	= 0x0000
};

struct OperDef
{
  OperDef() { };
  OperDef(const OperDef & copy);
  virtual ~OperDef() { delete this->pattern; };

  std::string Handle, Passwd;
  Pattern *pattern;
  int Flags;
};

struct LinkDef
{
  LinkDef() { };
  LinkDef(const LinkDef & copy);
  virtual ~LinkDef() { delete this->pattern; };

  std::string Handle, Passwd;
  Pattern *pattern;
  int Flags;
};

struct ConnDef
{
  ConnDef() { };

  std::string Handle, Hostname, Passwd;
  int Flags;
  BotSock::Port port;
};

struct YLine
{
  YLine() { };

  std::string YClass;
  std::string Description;
};

struct server {
  std::string Hostname, Password, Channels;
  BotSock::Port port;
};

typedef std::list<OperDef> OperList;
typedef std::list<LinkDef> LinkList;
typedef std::list<ConnDef> ConnList;
typedef std::list<YLine> YLineList;

class Config
{
private:
  static OperList Opers;
  static BotSock::Port port;
  static std::list<Pattern *> Exceptions;
  static std::list<Pattern *> Spoofers;
  static LinkList Links;
  static ConnList Connections;
  static YLineList YLines;
  static std::string Nick, UserName, IRCName, OperNick, OperPass;
  static std::string OurHostName;
  static std::string LogFile;
  static std::string MOTD;
  static struct server Server;
  static std::string proxyVhost;
  static std::string helpFilename;
  static std::string settingsFile;
  static std::string userDBFile;

  static void AddOper(const std::string & Handle, const std::string & pattern,
    const std::string & Passwd, const std::string & Flags);
  static void AddException(const std::string & pattern);
  static void AddSpoofer(const std::string & pattern);
  static void AddLink(const std::string & Handle, const std::string & pattern,
    const std::string & Passwd, const std::string & Flags);
  static void AddConn(const std::string & Handle, const std::string & Hostname,
    const std::string & Passwd, const std::string & Flags,
    const BotSock::Port port);
  static void AddYLine(const std::string & YClass,
    const std::string & Description);
public:
  static void Clear();
  static void Load(const std::string filename = DEFAULT_CFGFILE);
  static bool Auth(const std::string & Handle, const std::string & UserName,
    const std::string & Password, int & Flags, std::string & NewHandle);
  static std::string GetHostName();
  static std::string GetUserName();
  static std::string GetNick() { return Nick; }
  static std::string GetIRCName() { return IRCName; }
  static std::string GetServerHostName() { return Server.Hostname; }
  static BotSock::Port GetServerPort() { return Server.port; }
  static std::string GetServerPassword() { return Server.Password; }
  static std::string GetOperNick() { return OperNick; }
  static std::string GetOperPass() { return OperPass; }
  static std::string GetChannels() { return Server.Channels; }
  static bool haveChannel(const std::string & channel);
  static std::string GetLogFile() { return LogFile; }
  static std::string GetMOTD() { return MOTD; }
  static BotSock::Port GetPort() { return port; }
  static bool IsOKHost(const std::string & userhost);
  static bool IsOKHost(const std::string & userhost, const std::string & ip);
  static bool IsOKHost(const std::string & userhost,
    const BotSock::Address & ip);
  static bool IsOper(const std::string & userhost);
  static bool IsOper(const std::string & userhost, const std::string & ip);
  static bool IsOper(const std::string & userhost, const BotSock::Address & ip);
  static bool IsLinkable(const std::string &);
  static bool GetConn(std::string &, std::string &, BotSock::Port &,
    std::string &, int &);
  static bool AuthBot(const std::string &, const std::string &,
    const std::string &);
  static std::string GetYLineDescription(const std::string &);
  static std::string getProxyVhost();
  static bool IsSpoofer(const std::string &);
  static std::string getHelpFilename() { return helpFilename; }
  static std::string getUserDBFile() { return userDBFile; }
  static bool saveSettings();
  static bool loadSettings();
};

#endif /* __CONFIG_H__ */

