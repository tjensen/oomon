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
#include "vars.h"
#include "remotelist.h"
#include "botexcept.h"
#include "botclient.h"
#include "userdb.h"
#include "adnswrap.h"
#include "dnsbl.h"


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

  if ((0 == skip) || Same(skip->bot(), Config::GetNick()))
  {
    remotes.sendBroadcast(Config::GetNick(), message,
      UserFlags::getNames(flags, ','),
      WatchSet::getWatchNames(watches, false, ','));
  }
  else
  {
    remotes.sendBroadcastId(Config::GetNick(), skip->id(), skip->bot(), message,
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
  Config::Clear();
  Config::Load(configFile);
  Config::loadSettings();
  Log::Stop();
  Log::Start();
  remotes.listen();
}


void
ReloadConfig(const std::string & from)
{
  ::SendAll("Reload CONFIG requested by " + from, UserFlags::OPER);
  reload();
}


RETSIGTYPE
hangup(int sig)
{
  if (sig == SIGHUP)
  {
    Log::Write("Caught SIGHUP -- Reloading config");
    ::SendAll("*** Caught SIGHUP.", UserFlags::OPER);
    reload();
    std::signal(SIGHUP, hangup);
  }
}


bool
process()
{
  for (;;)
  {
    if (!server.isConnected() && !server.isConnecting())
    {
#ifdef MAIN_DEBUG
      std::cout << "Connecting to IRC server." << std::endl;
#endif
      server.bindTo(Config::GetHostName());
      if (!server.connect(Config::GetServerHostName(), Config::GetServerPort()))
      {
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
    adns.beforeselect(maxfd, readfds, writefds, exceptfds, tv_mod);
    server.setFD(readfds, writefds);
    clients.setAllFD(readfds, writefds);
    remotes.setFD(readfds, writefds);
    proxies.setAllFD(readfds, writefds);

    int fds = select(FD_SETSIZE, &readfds, &writefds, NULL, tv_mod);

    if (fds >= 0)
    {
      adns.afterselect(maxfd, readfds, writefds, exceptfds);

      try
      {
        if ((server.isConnected() || server.isConnecting()) &&
          !server.process(readfds, writefds))
        {
#ifdef MAIN_DEBUG
          std::cout << "Disconnected from IRC server." << std::endl;
#endif
          server.reset();
        }
      }
      catch (OOMon::timeout_error)
      {
        std::cerr << "Connection from IRC server timed out." << std::endl;
        server.quit("Server inactive for " + IntToStr(server.getIdle()) +
          " seconds");
      }

      server.checkUserDelta();

      clients.processAll(readfds, writefds);

      remotes.processAll(readfds, writefds);

#ifdef HAVE_LIBADNS
      dnsbl.process();
#endif /* HAVE_LIBADNS */

      proxies.processAll(readfds, writefds);

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
  std::ifstream	motdfile;
  char		line[MAX_BUFF];

  motdfile.open(Config::GetMOTD().c_str());
  if (!motdfile.fail())
  {
    client->send("Message Of The Day:");
    while (motdfile.getline(line, MAX_BUFF - 1))
    {
      client->send(std::string(line));
    }
    client->send("End of MOTD");
  }
  else
  {
    client->send("No MOTD");
  }
  motdfile.close();
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

  Config::Load(configFile);
  Config::loadSettings();

  if (alreadyRunning())
  {
    exit(EXIT_ALREADY_RUNNING);
  }

  try
  {
    userConfig.reset(new UserDB(Config::getUserDBFile()));
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
        std::cerr << "Unable to fork! Error " << IntToStr(errno) << std::endl;
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

