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

// C headers
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "oomon.h"

// C++ headers
#include <iostream>
#include <fstream>
#include <string>
#include "config.h"

// OOMon Headers
#include "util.h"
#include "help.h"
#include "trap.h"
#include "vars.h"
#include "pattern.h"
#include "botsock.h"
#include "log.h"
#include "botexcept.h"


#ifdef DEBUG
# define CONFIG_DEBUG
#endif


#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif



OperDef::OperDef(const OperDef & copy)
{
  this->pattern = smartPattern(copy.pattern->get(), false);
  this->Handle = copy.Handle;
  this->Passwd = copy.Passwd;
  this->Flags = copy.Flags;
}


LinkDef::LinkDef(const LinkDef & copy)
{
  this->pattern = smartPattern(copy.pattern->get(), false);
  this->Handle = copy.Handle;
  this->Passwd = copy.Passwd;
  this->Flags = copy.Flags;
}


OperList Config::Opers;
BotSock::Port Config::port = DEFAULT_PORT;
std::list<Pattern *> Config::Exceptions;
std::list<Pattern *> Config::Spoofers;
LinkList Config::Links;
ConnList Config::Connections;
YLineList Config::YLines;
std::string Config::Nick, Config::UserName, Config::IRCName, Config::OperNick, Config::OperPass;
std::string Config::OurHostName;
std::string Config::LogFile = ::expandPath(DEFAULT_LOGFILE, LOGDIR);
std::string Config::MOTD = ::expandPath(DEFAULT_MOTDFILE, ETCDIR);
struct server Config::Server;
std::string Config::proxyVhost;
std::string Config::helpFilename = ::expandPath(DEFAULT_HELPFILE, ETCDIR);
std::string Config::settingsFile = ::expandPath(DEFAULT_SETTINGSFILE, ETCDIR);
std::string Config::userDBFile = ::expandPath(DEFAULT_USERDBFILE, ETCDIR);

// AddOper(Handle, pattern, Passwd, Flags)
//
// This does the actually adding of an Oper to the list.
// The user@host mask is the only required parameter.
// Passwords and handles are highly recommended. In fact, a handle is
// required for authorization if you have a password! :P
// Supported flags are: (case sensitive!)
//  C - user can request channel ops
//  D - user can dline
//  G - user can gline
//  K - user can kline, kill, etc.
//  M - user is a master and can control linking
//  N - user can see nick changes
//  O - user is an oper (pretty mandatory)
//  R - user can remotely control other bots
//  W - user can see WALLOPS
//  X - user can see client connects/disconnects
//
void Config::AddOper(const std::string & Handle, const std::string & pattern,
	const std::string & Passwd, const std::string & Flags)
{
  try
  {
    OperDef temp;
    temp.Handle = Handle;
    temp.pattern = smartPattern(pattern, false);
    temp.Passwd = Passwd;
    temp.Flags = UF_NONE;
    if (Flags.find('C') != std::string::npos)
      temp.Flags |= UF_CHANOP;
    if (Flags.find('D') != std::string::npos)
      temp.Flags |= UF_DLINE;
    if (Flags.find('G') != std::string::npos)
      temp.Flags |= UF_GLINE;
    if (Flags.find('K') != std::string::npos)
      temp.Flags |= UF_KLINE;
    if (Flags.find('M') != std::string::npos)
      temp.Flags |= UF_MASTER;
    if (Flags.find('N') != std::string::npos)
      temp.Flags |= UF_NICK;
    if (Flags.find('O') != std::string::npos)
      temp.Flags |= UF_OPER;
    if (Flags.find('R') != std::string::npos)
      temp.Flags |= UF_REMOTE;
    if (Flags.find('W') != std::string::npos)
      temp.Flags |= UF_WALLOPS;
    if (Flags.find('X') != std::string::npos)
      temp.Flags |= UF_CONN;
    Opers.push_back(temp);
#ifdef CONFIG_DEBUG
    std::cout << "Oper: " << Handle << ", " << pattern << ", " << Passwd <<
      ", " << Flags << std::endl;
#endif
  }
  catch (OOMon::regex_error & e)
  {
    std::cerr << "RegEx error: " << e.what() << std::endl;
  }
}


// AddException(pattern)
//
// Adds an exception pattern to the list
//
void Config::AddException(const std::string & pattern)
{
  try
  {
    Exceptions.push_back(smartPattern(pattern, false));
#ifdef CONFIG_DEBUG
    std::cout << "Exception: " << pattern << std::endl;
#endif
  }
  catch (OOMon::regex_error & e)
  {
    std::cerr << "RegEx error: " << e.what() << std::endl;
  }
}


// AddSpoofer(pattern)
//
// Adds an spoofer ip pattern to the list
//
void Config::AddSpoofer(const std::string & pattern)
{
  try
  {
    Spoofers.push_back(smartPattern(pattern, false));
#ifdef CONFIG_DEBUG
    std::cout << "Spoofer: " << pattern << std::endl;
#endif
  }
  catch (OOMon::regex_error & e)
  {
    std::cerr << "RegEx error: " << e.what() << std::endl;
  }
}


// AddLink(Handle, pattern, Passwd, Flags)
//
// Adds a link definition to the list.
// A handle and host pattern are required! Passwords are highly recommended!
// Supported flags are: (case sensitive!)
//
//
void Config::AddLink(const std::string & Handle, const std::string & pattern,
	const std::string & Passwd, const std::string & Flags)
{
  if (Handle != "")
  {
    try
    {
      LinkDef temp;
      temp.Handle = Handle;
      temp.pattern = smartPattern(pattern, false);
      temp.Passwd = Passwd;
      //temp.Flags = Flags;
      Links.push_back(temp);
#ifdef CONFIG_DEBUG
      std::cout << "Link: " << Handle << ", " << pattern << ", " << Passwd <<
        ", " << Flags << std::endl;
#endif
    }
    catch (OOMon::regex_error & e)
    {
      std::cerr << "RegEx error: " << e.what() << std::endl;
    }
  }
}


// AddConn(Handle, Hostname, Passwd, Flags, port)
//
// Adds a connection definition to the list
// A handle, hostname, and port number are required! Password requirements
// depend on the bot you are connecting to.
// Supported flags are the same as for AddLink()
//
void Config::AddConn(const std::string & Handle, const std::string & Hostname,
  const std::string & Passwd, const std::string & Flags,
  const BotSock::Port port)
{
  if ((Handle != "") && (Hostname != "") && (port != 0)) {
    ConnDef temp;
    temp.Handle = Handle;
    temp.Hostname = Hostname;
    temp.Passwd = Passwd;
    //temp.Flags = Flags;
    temp.port = port;
    Connections.push_back(temp);
    #ifdef CONFIG_DEBUG
      std::cout << "Conn: " << Handle << ", " << Hostname << ", " << Passwd <<
	", " << Flags << ", " << port << std::endl;
    #endif
  }
}


// AddYLine(YClass, Description)
//
// Adds a connection class (Y-line) to the list
// A YClass and Description are required.
//
void Config::AddYLine(const std::string & YClass,
  const std::string & Description)
{
  if (Description != "")
  {
    YLine temp;
    temp.YClass = YClass;
    temp.Description = Description;
    YLines.push_back(temp);
    #ifdef CONFIG_DEBUG
      std::cout << "Y-Line: " << YClass << ", " << Description << std::endl;
    #endif
  }
}


// Load(filename)
//
// This does the actual loading of the configuration file.
//
// I ought to trim any leading white space before parsing, but oh well.
//
void Config::Load(const std::string filename)
{
  char		line[MAX_BUFF];
  char		*key,
		*value;
  std::string	handle,
		mask,
		hostname,
		passwd,
		flags,
		yclass;
  int		port;
  StrVector Parameters;
 
  #ifdef CONFIG_DEBUG
    std::cout << "Reading configuration file (" << filename << ")" << std::endl;
  #endif

  Help::flush();

  std::ifstream CfgFile(filename.c_str());
  while (CfgFile.getline(line, MAX_BUFF-1))
  {
    if (line[0] == '#')
    {
      continue;
    }

    key = strtok(line, ":");
    if (key == NULL)
    {
      continue;
    }
    value = strtok(NULL, "\r\n");
    if (value == NULL)
    {
      continue;
    }
      
    StrSplit(Parameters, value, ":");

    switch (*key)
    {
      case 'b':
      case 'B':
	if (Parameters.size() > 5)
	{
	  // Bot parameters
	  #ifdef CONFIG_DEBUG
	    std::cout << "Bot: ";
	    //Parameters.Print();
	  #endif
	  Nick = Parameters[0];
	  UserName = Parameters[1];
	  OurHostName = Parameters[2];
	  IRCName = Parameters[3];
	  OperNick = Parameters[4];
	  OperPass = Parameters[5];
	}
	break;
      case 'c':
      case 'C':
	if (Parameters.size() > 4)
	{
	  // Connection parameters
	  handle = Parameters[0];
	  hostname = Parameters[1];
	  passwd = Parameters[2];
	  flags = Parameters[3];
	  port = atoi(Parameters[4].c_str());
	  AddConn(handle, hostname, passwd, UpCase(flags), port);
	}
	break;
      case 'e':
      case 'E':
	if (Parameters.size() > 0)
	{
	  // Exception
	  AddException(Parameters[0]);
	}
	break;
      case 'f':
      case 'F':
	if (Parameters.size() > 0)
	{
	  // Spoofers
	  AddSpoofer(Parameters[0]);
	}
	break;
      case 'g':
      case 'G':
	if ((Parameters.size() > 0) && (Parameters[0].length() > 0))
	{
	  // Log file
	  LogFile = ::expandPath(Parameters[0], LOGDIR);
        }
	break;
      case 'h':
      case 'H':
	if ((Parameters.size() > 0) && (Parameters[0].length() > 0))
	{
	  // Help file
	  helpFilename = ::expandPath(Parameters[0], ETCDIR);
	}
	break;
      case 'i':
      case 'I':
	if ((Parameters.size() > 0) && (Parameters[0].length() > 0))
	{
	  // Include file
	  Load(::expandPath(Parameters[0], ETCDIR));
	}
	break;
      case 'l':
      case 'L':
	if (Parameters.size() > 3)
	{
	  // Link parameters
	  handle = Parameters[0];
	  mask = Parameters[1];
	  passwd = Parameters[2];
	  flags = Parameters[3];
	  AddLink(handle, mask, passwd, UpCase(flags));
	}
	break;
      case 'm':
      case 'M':
	if ((Parameters.size() > 0) && (Parameters[0].length() > 0))
	{
	  // MOTD file
	  MOTD = ::expandPath(Parameters[0], ETCDIR);
	}
	break;
      case 'o':
      case 'O':
	if (Parameters.size() > 3)
	{
	  // Oper definition
	  handle = Parameters[0];
	  mask = Parameters[1];
	  passwd = Parameters[2];
	  flags = Parameters[3];
	  AddOper(handle, mask, passwd, UpCase(flags));
	}
	break;
      case 'p':
      case 'P':
	if (Parameters.size() > 0)
	{
	  // Port number
	  Config::port = atoi(Parameters[0].c_str());
#ifdef CONFIG_DEBUG
          std::cout << "Port: " << Config::port << std::endl;
#endif
	}
	break;
      case 's':
      case 'S':
	if (Parameters.size() > 3)
	{
	  // Server Parameters
	  #ifdef CONFIG_DEBUG
	    std::cout << "Server: ";
	    //Parameters.Print();
	  #endif
	  Server.Hostname = Parameters[0];
	  Server.port = atoi(Parameters[1].c_str());
	  Server.Password = Parameters[2];
	  Server.Channels =  Parameters[3];
	}
	break;
      case 't':
      case 'T':
	if ((Parameters.size() > 0) && (Parameters[0].length() > 0))
	{
	  // OOMon Settings file
	  settingsFile = ::expandPath(Parameters[0], ETCDIR);
	}
	break;
      case 'u':
      case 'U':
	if ((Parameters.size() > 0) && (Parameters[0].length() > 0))
	{
	  // User settings file
	  userDBFile = ::expandPath(Parameters[0], ETCDIR);
	}
	break;
      case 'w':
      case 'W':
	if (Parameters.size() > 0)
	{
	  // WinGate/Proxy checker virtual hostname
	  proxyVhost = Parameters[0];
	}
	break;
      case 'y':
      case 'Y':
	if (Parameters.size() > 1)
	{
	  // User classes (Y-lines)
	  #ifdef CONFIG_DEBUG
	    std::cout << "Y-line: ";
	    //Parameters.Print();
	  #endif
	  yclass = Parameters[0];
	  hostname = Parameters[1];
	  AddYLine(yclass, hostname);
	}
        break;
    }
  }
  CfgFile.close();

  #ifdef CONFIG_DEBUG
    std::cout << "Completed reading configuration (" << filename << ")" <<
      std::endl;
  #endif
}


// Clear()
//
// Clears the current configuration. This is usually done immediately before
// loading the configuration from file.
//
void Config::Clear()
{
  Opers.clear();
  Exceptions.clear();
  Spoofers.clear();
  Links.clear();
  Connections.clear();
  YLines.clear();
  port = DEFAULT_PORT;
  Server.Hostname = "";
  Server.port = 0;
  Server.Password = "";
  Server.Channels = "";
  LogFile = ::expandPath(DEFAULT_LOGFILE, LOGDIR);
  MOTD = ::expandPath(DEFAULT_MOTDFILE, ETCDIR);
  helpFilename = ::expandPath(DEFAULT_HELPFILE, ETCDIR);
  settingsFile = ::expandPath(DEFAULT_SETTINGSFILE, ETCDIR);
  userDBFile = ::expandPath(DEFAULT_USERDBFILE, ETCDIR);
}


// Auth(Handle, UserHost, Password, Flags, NewHandle)
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
bool Config::Auth(const std::string & Handle, const std::string & UserHost,
	const std::string & Password, int & Flags, std::string & NewHandle)
{
#ifdef CONFIG_DEBUG
  std::cout << "Config::Auth(Handle = \"" << Handle << "\", UserHost = \"" <<
    UserHost << "\", Password = \"" << Password << "\", ...)" << std::endl;
#endif
  try
  {
    for (OperList::iterator pos = Opers.begin(); pos != Opers.end(); ++pos)
    {
      if (Same(pos->Handle, Handle) && pos->pattern->match(UserHost))
      {
#ifdef CONFIG_DEBUG
        std::cout << "Config::Auth(): found match" << std::endl;
#endif
        if (ChkPass(pos->Passwd, Password))
        {
#ifdef CONFIG_DEBUG
          std::cout << "Config::Auth(): password is correct" << std::endl;
#endif
          if (pos->Handle != "")
	  {
	    // Only people with registered handles should be allowed to do
	    // anything
            NewHandle = pos->Handle;
            Flags = pos->Flags | UF_AUTHED;
          }
	  else
	  {
	    // You might want to set up an open O: line *@* in the config file
	    // (with no nick) to allow anyone to chat
	    Flags = UF_AUTHED;
          }
          return true;
        }
        else
        {
#ifdef CONFIG_DEBUG
          std::cout << "Config::Auth(): incorrect password" << std::endl;
#endif
        }
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::Auth(): " + e.what());
    std::cerr << "RegEx error in Config::Auth(): " << e.what() << std::endl;
  }
  return false;
}

std::string Config::GetHostName()
{
  return OurHostName;
}

std::string Config::GetUserName()
{
  if (UserName == "")
  {
    char *login = getlogin();

    if (login && *login)
      return login;
    else
      return "oomon";
  }
  else
  {
    return UserName;
  }
}


bool Config::IsOKHost(const std::string & userhost)
{
  try
  {
    for (std::list<Pattern *>::iterator pos = Exceptions.begin();
      pos != Exceptions.end(); ++pos)
    {
      if ((*pos)->match(userhost))
      {
        return true;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::IsOKHost(): " + e.what());
    std::cerr << "RegEx error in Config::IsOKHost(): " << e.what() << std::endl;
  }
  return false;
}


bool Config::IsOKHost(const std::string & userhost, const std::string & ip)
{
  if (ip == "")
  {
    return Config::IsOKHost(userhost);
  }
  else
  {
    std::string user = userhost.substr(0, userhost.find('@'));
    std::string userip = user + "@" + ip;

    return Config::IsOKHost(userhost) || Config::IsOKHost(userip);
  }
}


bool Config::IsOKHost(const std::string & userhost, const BotSock::Address & ip)
{
  return IsOKHost(userhost, BotSock::inet_ntoa(ip));
}


bool Config::IsOper(const std::string & userhost)
{
  try
  {
    for (OperList::iterator pos = Opers.begin(); pos != Opers.end(); ++pos)
    {
      if (pos->pattern->match(userhost))
      {
        return (pos->Flags & UF_OPER);
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::IsOper(): " + e.what());
    std::cerr << "RegEx error in Config::IsOper(): " << e.what() << std::endl;
  }
  return false;
}


bool Config::IsOper(const std::string & userhost, const std::string & ip)
{
  if (ip == "")
  {
    return Config::IsOper(userhost);
  }
  else
  {
    std::string user = userhost.substr(0, userhost.find('@'));
    std::string userip = user + "@" + ip;

    return Config::IsOper(userhost) || Config::IsOper(userip);
  }
}


bool Config::IsOper(const std::string & userhost, const BotSock::Address & ip)
{
  return Config::IsOper(userhost, BotSock::inet_ntoa(ip));
}


bool Config::IsLinkable(const std::string & host)
{
  try
  {
    for (LinkList::iterator pos = Links.begin(); pos != Links.end(); ++pos)
    {
      if (pos->pattern->match(host))
      {
        return true;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::IsLinkable(): " + e.what());
    std::cerr << "RegEx error in Config::IsLinkable(): " << e.what() <<
      std::endl;
  }
  return false;
}

bool Config::GetConn(std::string & BotHandle, std::string & Host,
  BotSock::Port & port, std::string & Passwd, int & Flags)
{
  for (ConnList::iterator pos = Connections.begin(); pos != Connections.end();
    ++pos)
  {
    if (Same(BotHandle, pos->Handle))
    {
      BotHandle = pos->Handle;
      Host = pos->Hostname;
      port = pos->port;
      Passwd = pos->Passwd;
      Flags = pos->Flags;
      return true;
    }
  }
  return false;
}

bool Config::AuthBot(const std::string & Handle, const std::string & Host,
	const std::string & Passwd)
{
  try
  {
    for (LinkList::iterator pos = Links.begin(); pos != Links.end(); ++pos)
    {
      if (Same(Handle, pos->Handle) && pos->pattern->match(Host) &&
        (pos->Passwd == Passwd))
      {
        return true;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::AuthBot(): " + e.what());
    std::cerr << "RegEx error in Config::AuthBot(): " << e.what() << std::endl;
  }
  return false;
}

std::string Config::GetYLineDescription(const std::string & YClass)
{
  for (YLineList::iterator pos = YLines.begin(); pos != YLines.end(); ++pos)
  {
    if (YClass == pos->YClass)
    {
      return pos->Description;
    }
  }
  return "";
}

std::string
Config::getProxyVhost()
{
  if (proxyVhost == "")
    return Config::GetHostName();
  else
    return proxyVhost;
}

bool Config::IsSpoofer(const std::string & ip)
{
  try
  {
    for (std::list<Pattern *>::iterator pos = Spoofers.begin();
      pos != Spoofers.end(); ++pos)
    {
      if ((*pos)->match(ip))
      {
        return true;
      }
    }
  }
  catch (OOMon::regex_error & e)
  {
    Log::Write("RegEx error in Config::IsSpoofer(): " + e.what());
    std::cerr << "RegEx error in Config::IsSpoofer(): " << e.what() <<
      std::endl;
  }
  return false;
}


bool
Config::saveSettings()
{
  std::ofstream file(settingsFile.c_str());

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


bool
Config::loadSettings()
{
  std::ifstream file(settingsFile.c_str());

  if (file)
  {
    // First clear out the TRAP list!
    TrapList::clear();

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
	StrList discard;

	TrapList::cmd(discard, line, true);

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
#ifdef CONFIG_DEBUG
        std::cout << "Unknown setting type: " << cmd << std::endl;
#endif
	file.close();
	return false;
      }
    }

    // Done!
    file.close();

    return true;
  }
  else
  {
     return false;
  }
}

