#ifndef __CONFIG_H__
#define __CONFIG_H__
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

// C++ Headers
#include <string>
#include <list>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "botsock.h"
#include "pattern.h"

struct server {
  std::string Hostname, Password, Channels;
  BotSock::Port port;
};

typedef std::list<class OperDef> OperList;
typedef std::list<class LinkDef> LinkList;
typedef std::list<class ConnDef> ConnList;
typedef std::list<class YLine> YLineList;
typedef std::list<class RemoteClient> RemoteClientList;

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
  static RemoteClientList remoteClients;
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
    const std::string & Passwd);
  static void AddConn(const std::string & Handle, const std::string & Hostname,
    const std::string & Passwd, const BotSock::Port port);
  static void AddYLine(const std::string & YClass,
    const std::string & Description);
  static void addRemoteClient(const std::string & mask,
    const std::string & flags);
public:
  static void Clear();
  static void Load(const std::string filename = DEFAULT_CFGFILE);
  static bool Auth(const std::string & Handle, const std::string & UserName,
    const std::string & Password, class UserFlags & Flags,
    std::string & NewHandle);
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
    std::string &);
  static bool AuthBot(const std::string &, const std::string &,
    const std::string &);
  static std::string GetYLineDescription(const std::string &);
  static UserFlags getRemoteFlags(const std::string & client);
  static UserFlags parseUserFlags(const std::string & text);
  static std::string getProxyVhost();
  static bool IsSpoofer(const std::string &);
  static std::string getHelpFilename() { return helpFilename; }
  static std::string getUserDBFile() { return userDBFile; }
  static bool saveSettings();
  static bool loadSettings();
};

#endif /* __CONFIG_H__ */

