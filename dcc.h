#ifndef __DCC_H__
#define __DCC_H__
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

#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <list>
#include <set>

#include "strtype"
#include "oomon.h"
#include "config.h"
#include "botsock.h"
#include "watch.h"
#include "userhash.h"


class DCC : public BotSock
{
public:
  DCC(const std::string & nick = "", const std::string & userhost = "");
  DCC(DCC *listener, const std::string & nick = "",
    const std::string & userhost = "");

  bool connect(const BotSock::Address address, const BotSock::Port port,
    const std::string nick, const std::string userhost);
  bool listen(const std::string nick, const std::string userhost,
    const BotSock::Port port = 0, const int backlog = 1);

  void chat(const std::string & text);
  void send(const std::string & message, const int flags = UF_NONE,
    const WatchSet & watches = WatchSet());
  void send(StrList & message, const int flags = UF_NONE,
    const WatchSet & watches = WatchSet());

  std::string getUserhost() const { return UserHost; };
  std::string getHandle(const bool atBot = true) const;
  std::string getNick() const { return Nick; };
  std::string getFlags() const;

  bool isOper() const { return ((this->Flags & UF_OPER) != 0); };
  bool isAuthed() const { return ((this->Flags & UF_AUTHED) != 0); };

  virtual bool onConnect();

protected:
  virtual bool onRead(std::string text);

private:
  bool echoMyChatter;
  std::string UserHost, Nick, Handle;
  int Flags;
  WatchSet watches;

  bool parse(std::string text);
  void motd();
  void kline(const int Minutes, const std::string & Target,
    const std::string & Reason);

  void list(const std::string & to, std::string text);
  void glist(const std::string & to, std::string text);
  void killList(const std::string & to, std::string text);
  void nFind(const std::string & to, std::string text);
  void killNFind(const std::string & to, std::string text);
  void findK(const std::string & to, std::string text);
  void findD(const std::string & to, std::string text);
  void seedrand(const std::string & to, std::string text);
  void status(const std::string & to, std::string text);
  void watch(const std::string & to, std::string text);
  void trap(const std::string & to, std::string text);
  void save(const std::string & to, std::string text);
  void load(const std::string & to, std::string text);
  void set(const std::string & to, std::string text);
  void spamsub(const std::string & to, std::string text);
  void spamunsub(const std::string & to, std::string text);

  void kClone(const int Minutes, const std::string & Target);
  void kFlood(const int Minutes, const std::string & Target);
  void kBot(const int Minutes, const std::string & Target);
  void kSpam(const int Minutes, const std::string & Target);
  void kLink(const int Minutes, const std::string & Target);
  void kTrace(const int Minutes, const std::string & Target);
  void kMotd(const int Minutes, const std::string & Target);
  void kInfo(const int Minutes, const std::string & Target);
  void kWingate(const int Minutes, const std::string & Target);
  void kProxy(const int Minutes, const std::string & Target);
  void kPerm(const std::string & Target, const std::string & Reason);
  void unkline(const std::string & Target);
  void dline(const int Minutes, const std::string & Target,
    const std::string & Reason);
  void undline(const std::string & Target);
  void help(std::string);
  void doEcho(const std::string & text);
  void loadConfig();
  void noAccess();
  void notRemote();
  void notAuthed();

  enum DccCommand
  {
    // Unknown command
    DCC_UNKNOWN,

    // Generic commands
    DCC_AUTH, DCC_QUIT, DCC_CHAT, DCC_WHO, DCC_LINKS, DCC_HELP, DCC_WATCH,
    DCC_MOTD, DCC_ECHO, DCC_INFO, DCC_STATUS,

    // +K commands
    DCC_KLINE, DCC_UNKLINE, DCC_KCLONE, DCC_KFLOOD, DCC_KBOT, DCC_KLINK,
    DCC_KSPAM, DCC_KPERM, DCC_KTRACE, DCC_KMOTD, DCC_KINFO, DCC_KPROXY,

    // +G commands
    DCC_GLINE, DCC_UNGLINE,

    // +D commands
    DCC_DLINE, DCC_UNDLINE,

    // +O commands
    DCC_KILL, DCC_KILLLIST, DCC_KILLNFIND, DCC_MULTI, DCC_NFIND, DCC_TRACE,
    DCC_CLONES, DCC_GLIST, DCC_LIST, DCC_FINDK, DCC_RELOAD, DCC_DOMAINS,
    DCC_CLASS, DCC_HMULTI, DCC_UMULTI, DCC_VMULTI, DCC_FINDD, DCC_SEEDRAND,
    DCC_NETS,

    // +M commands
    DCC_DIE, DCC_CONN, DCC_DISCONN, DCC_RAW, DCC_TRAP, DCC_TEST, DCC_SAVE,
    DCC_LOAD, DCC_SET, DCC_SPAMSUB, DCC_SPAMUNSUB,

    // +C commands
    DCC_OP, DCC_JOIN, DCC_PART,
  };

  static DccCommand getCommand(const std::string & text);
};


#endif /* __DCC_H__ */

