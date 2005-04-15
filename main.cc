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
#include <fstream>
#include <string>
#include <list>
#include <cerrno>
#include <ctime>
#include <csignal>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// Std C Headers
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

// OOMon Headers
#include "oomon.h"
#include "main.h"
#include "config.h"
#include "irc.h"
#include "services.h"
#include "log.h"
#include "proxylist.h"
#include "util.h"
#include "dcclist.h"
#include "dcc.h"
#include "watch.h"
#include "remotelist.h"
#include "botexcept.h"
#include "botclient.h"
#include "userdb.h"
#include "adnswrap.h"
#include "dnsbl.h"
#include "engine.h"
#include "userhash.h"


#ifdef DEBUG
# define MAIN_DEBUG
#endif


#ifdef FORK_OOMON
static bool forkme = true;
#else
static bool forkme = false;
#endif

static std::string configFile = DEFAULT_CFGFILE;
static std::string pidFile = DEFAULT_PIDFILE;

static std::time_t startTime = 0;


void
SendAll(const std::string & message, const UserFlags flags,
  const WatchSet & watches, const BotClient * skip)
{
  clients.sendAll(message, flags, watches, skip);

  if ((0 == skip) || Same(skip->bot(), config.nickname()))
  {
    remotes.sendBroadcast(config.nickname(), message,
      UserFlags::getNames(flags, ','),
      WatchSet::getWatchNames(watches, false, ','));
  }
  else
  {
    remotes.sendBroadcastId(config.nickname(), skip->id(), skip->bot(), message,
      UserFlags::getNames(flags, ','),
      WatchSet::getWatchNames(watches, false, ','));
  }
}


RETSIGTYPE
gracefuldie(int sig)
{
  if (sig == SIGTERM)
  {
    ::SendAll("Caught SIGTERM -- OOMon terminating");
    Log::Write("Caught SIGTERM -- OOMon stopped");
    if (server.isConnected())
    {
      server.quit();
    }
  }
  else if (sig == SIGINT)
  {
    ::SendAll("Caught SIGINT -- OOMon terminating");
    Log::Write("Caught SIGINT -- OOMon stopped");
    if (server.isConnected())
    {
      server.quit("Caught SIGINT -- User pressed Ctrl+C?");
    }
  }
  else
  {
    if (server.isConnected())
    {
      server.quit("Abnormal termination!");
    }
    abort();
  }

  Log::Stop();

  exit(EXIT_NOERROR);
}


void
reload(void)
{
  try
  {
    Config newConfig(configFile);
    config = newConfig;

    Log::Stop();
    Log::Start();
    config.loadSettings();
    remotes.listen();
    try
    {
      boost::shared_ptr<UserDB> newConfig(new UserDB(config.userDBFilename()));
      userConfig.swap(newConfig);
    }
    catch (OOMon::botdb_error & e)
    {
      Log::Write(e.what() + " (" + e.why() + ")");
      ::SendAll("*** " + e.what() + " (" + e.why() + ")", UserFlags::MASTER);
    }
  }
  catch (Config::parse_failed & e)
  {
    Log::Write(e.what());
    ::SendAll("*** " + e.what(), UserFlags::MASTER);
    std::cerr << "*** " << e.what() << std::endl;
  }
}


void
ReloadConfig(const std::string & from)
{
  std::string notice("Reload CONFIG requested by ");
  notice += from;
  Log::Write(notice);
  ::SendAll("*** " + notice, UserFlags::OPER);
  reload();
}


RETSIGTYPE
hangup(int sig)
{
  if (sig == SIGHUP)
  {
    std::string notice("Caught SIGHUP -- Reloading config");
    Log::Write(notice);
    ::SendAll("*** " + notice, UserFlags::OPER);
    reload();
    std::signal(SIGHUP, hangup);
  }
}


bool
process()
{
  std::time_t nextConnectAttempt(0);
  std::time_t attemptWait(1);

  for (;;)
  {
    std::time_t now(time(0));

    if (!server.isConnected() && !server.isConnecting() &&
        (now >= nextConnectAttempt))
    {
      nextConnectAttempt = now + attemptWait;

      std::string notice("Connecting to IRC server at ");
      notice += config.serverAddress();
      notice += ':';
      notice += boost::lexical_cast<std::string>(config.serverPort());
#ifdef MAIN_DEBUG
      std::cout << notice << std::endl;
#endif
      Log::Write(notice);
      server.bindTo(config.hostname());
      if (server.connect(config.serverAddress(), config.serverPort()))
      {
        // Success!
        attemptWait = 1;
      }
      else
      {
        if (attemptWait < 120)
        {
          attemptWait *= 2;
        }
        std::string notice("Connect failed");
#ifdef MAIN_DEBUG
        std::cout << notice << std::endl;
#endif
        Log::Write(notice);
	server.reset();
      }
    }

    fd_set readfds, writefds, exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    int maxfd = 0;

    struct timeval time_out;
    time_out.tv_sec = 5;
    time_out.tv_usec = 0;

    struct timeval * tv_mod = &time_out;
    adns.preSelect(maxfd, readfds, writefds, exceptfds, tv_mod);
    server.preSelect(readfds, writefds);
    clients.preSelect(readfds, writefds);
    remotes.preSelect(readfds, writefds);
    proxies.preSelect(readfds, writefds);

    int fds = select(FD_SETSIZE, &readfds, &writefds, NULL, tv_mod);

    if (fds >= 0)
    {
      adns.postSelect(maxfd, readfds, writefds, exceptfds);

      try
      {
        if ((server.isConnected() || server.isConnecting()) &&
          !server.postSelect(readfds, writefds))
        {
          std::string notice("Disconnected from IRC server.");
#ifdef MAIN_DEBUG
          std::cout << notice << std::endl;
#endif
          Log::Write(notice);
          server.reset();
        }
      }
      catch (OOMon::timeout_error)
      {
        std::cerr << "Connection to IRC server timed out." << std::endl;
        server.quit("Server inactive for " +
	  boost::lexical_cast<std::string>(server.getIdle()) + " seconds");
      }

      server.checkUserDelta();

      clients.postSelect(readfds, writefds);

      remotes.postSelect(readfds, writefds);

#ifdef HAVE_LIBADNS
      dnsbl.process();
#endif /* HAVE_LIBADNS */

      proxies.postSelect(readfds, writefds);

      if (server.isConnected())
      {
        services.check();
      }
    }
    else if (fds == -1)
    {
      // some sort of error occurred (probably EINTR because of signal)
    }
  }
  return true;
}


// motd()
//
// Writes the Message Of The Day
//
void
motd(BotClient * client)
{
  std::ifstream	motdfile(config.motdFilename().c_str());

  if (motdfile.good())
  {
    client->send("Message Of The Day:");

    std::string line;
    while (std::getline(motdfile, line))
    {
      // Remove trailing CR if file uses DOS-style line terminators
      if (!line.empty() && ('\r' == line[line.length() - 1]))
      {
        line.erase(line.length() - 1);
      }

      client->send(line);
    }
    client->send("End of MOTD");
  }
  else
  {
    client->send("No MOTD");
  }
}


std::string
getUptime(void)
{
  return timeDiff(std::time(NULL) - startTime);
}


static	bool
alreadyRunning(void)
{
  bool result = false;

  std::ifstream pidFileStream(pidFile.c_str());
  if (pidFileStream)
  {
    pid_t pid;
    pidFileStream >> pid;
    pidFileStream.close();

    // kill(2) should return 0 if the process is still running (and we have
    // the necessary permissions to send signals to it).
    result = (0 == kill(pid, 0));
  }

  return result;
}


static void
initModules(void)
{
  engine_init();
  Config::init();
  CommandParser::init();
  IRC::init();
  DCC::init();
  Services::init();
  UserHash::init();
  Dnsbl::init();
  ProxyList::init();
}


static	void
printhelp(char *name)
{
  std::cout << "Usage:  " << name << " [-dh] [-c config_file] [-p pid_file]" <<
    std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << " -c config_file   load the specified config file" << std::endl;
  std::cout << " -d               do not fork into the background" << std::endl;
  std::cout << " -h               show this help" << std::endl;
  std::cout << " -p pid_file      write to the specified PID file" << std::endl;
}


int
parseargs(int argc, char **argv)
{
  int result = 0;
  int ch;

  while ((ch = getopt(argc, argv, "dhc:p:")) != -1)
  {
    switch (ch)
    {
      case 'c':
	configFile = optarg;
	break;
      case 'd':
        forkme = false;
	break;
      case 'p':
	pidFile = optarg;
	break;
      case 'h':
      case '?':
      default:
        printhelp(argv[0]);
	return -1;
	break;
    }
  }

  return result;
}


int
main(int argc, char **argv)
{
  startTime = std::time(NULL);

  if (parseargs(argc, argv))
  {
    return EXIT_CMDLINE_ERROR;
  }

  try
  {
    config = Config(configFile);
  }
  catch (Config::parse_failed & e)
  {
    std::cerr << "*** " << e.what() << std::endl;
    return EXIT_CONFIG_ERROR;
  }

  initModules();

  config.loadSettings();

  if (alreadyRunning())
  {
    exit(EXIT_ALREADY_RUNNING);
  }

  try
  {
    boost::shared_ptr<UserDB> newConfig(new UserDB(config.userDBFilename()));
    userConfig.swap(newConfig);
  }
  catch (OOMon::botdb_error & reason)
  {
    std::cerr << reason.what() <<  " (" << reason.why() << ")" << std::endl;
    exit(EXIT_BOTDB_ERROR);
  }

  if (forkme)
  {
    pid_t forkresult = fork();

    if (0 == forkresult)
    {
      // redirect output to /dev/null
      int fd = open("/dev/null", O_RDWR);
      dup2(fd, 1);	// stdout
      dup2(fd, 2);	// stderr
    }
    else
    {
      if (forkresult == -1)
      {
        std::cerr << "Unable to fork! Error " << errno << std::endl;
        return EXIT_FORK_ERROR;
      }
      return EXIT_NOERROR;
    }
  }

  std::signal(SIGSEGV, gracefuldie);
  std::signal(SIGBUS, gracefuldie);
  std::signal(SIGTERM, gracefuldie);
  std::signal(SIGINT, gracefuldie);
  std::signal(SIGHUP, hangup);
  std::signal(SIGPIPE, SIG_IGN);

  std::ofstream PIDfile;
  PIDfile.open(pidFile.c_str());
  PIDfile << getpid();
  PIDfile.close();

  Log::Start();
  Log::Write("OOMon started");

  remotes.listen();

  while (process());

  Log::Write("OOMon stopped");
  Log::Stop();

  return EXIT_NOERROR;
}

