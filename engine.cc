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

// Std C++ headers
#include <string>
#include <map>

// Std C headers
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <netdb.h>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "engine.h"
#include "flood.h"
#include "log.h"
#include "main.h"
#include "irc.h"
#include "util.h"
#include "config.h"
#include "proxy.h"
#include "trap.h"
#include "dcclist.h"
#include "seedrand.h"
#include "vars.h"
#include "jupe.h"
#include "userhash.h"
#include "notice.h"
#include "botexcept.h"


#ifdef DEBUG
# define ENGINE_DEBUG
#endif



class NickChangeEntry
{
public:
  std::string	userhost;
  std::string	lastNick;
  int		nickChangeCount;
  time_t	firstNickChange;
  time_t	lastNickChange;
  bool		noticed;
};

static std::list<NickChangeEntry> nickChanges;


static FloodList linkLookers("LINKS", VAR_LINKS_FLOOD_ACTION,
  VAR_LINKS_FLOOD_REASON, VAR_LINKS_FLOOD_MAX_COUNT, VAR_LINKS_FLOOD_MAX_TIME,
  WATCH_LINKS);

static FloodList traceLookers("TRACE", VAR_TRACE_FLOOD_ACTION,
  VAR_TRACE_FLOOD_REASON, VAR_TRACE_FLOOD_MAX_COUNT, VAR_TRACE_FLOOD_MAX_TIME,
  WATCH_TRACES);

static FloodList motdLookers("MOTD", VAR_MOTD_FLOOD_ACTION,
  VAR_MOTD_FLOOD_REASON, VAR_MOTD_FLOOD_MAX_COUNT, VAR_MOTD_FLOOD_MAX_TIME,
  WATCH_MOTDS);

static FloodList infoLookers("INFO", VAR_INFO_FLOOD_ACTION,
  VAR_INFO_FLOOD_REASON, VAR_INFO_FLOOD_MAX_COUNT, VAR_INFO_FLOOD_MAX_TIME,
  WATCH_INFOS);

static FloodList statsLookers("STATS", VAR_STATS_FLOOD_ACTION,
  VAR_STATS_FLOOD_REASON, VAR_STATS_FLOOD_MAX_COUNT, VAR_STATS_FLOOD_MAX_TIME,
  WATCH_STATS);

static NoticeList<FloodNoticeEntry> possibleFlooders;

static NoticeList<SpambotNoticeEntry> spambots;

static NoticeList<TooManyConnNoticeEntry> tooManyConn;

static NoticeList<ConnectEntry> connects;


#define IP_TABLE_SIZE 300
typedef struct ip_entry
{
  BotSock::Address net;
  int		ip_count, total_count;
  time_t	last_ip, first_ip;
} IP_ENTRY;

IP_ENTRY ips[IP_TABLE_SIZE];


static std::string
GetReason(const int ReasonType)
{
  switch (ReasonType) {
  case MK_CLONES:
    return REASON_CLONES;
  case MK_FLOODING:
    return REASON_FLOODING;
  case MK_BOTS:
    return REASON_BOTS;
  case MK_SPAM:
    return REASON_SPAM;
  case MK_LINKS:
    return REASON_LINKS;
  case MK_TRACE:
    return REASON_TRACE;
  case MK_MOTD:
    return REASON_MOTD;
  case MK_INFO:
    return REASON_INFO;
  case MK_STATS:
    return REASON_STATS;
  case MK_PROXY:
    return REASON_PROXY;
  default:
    return "Undefined Reason";
  }
}

std::string
Make_Kline(const std::string & Mask, const KlineType ReasonType,
  const int dur = 0)
{
  std::string cmd;

  std::string Dur;
  if (dur)
  {
    Dur = IntToStr(dur) + " ";
  }

  switch (ReasonType)
  {
    case MK_CLONES:
      cmd = ".kclone ";
      break;
    case MK_FLOODING:
      cmd = ".kflood ";
      break;
    case MK_BOTS:
      cmd = ".kbot ";
      break;
    case MK_SPAM:
      cmd = ".kspam ";
      break;
    case MK_LINKS:
      cmd = ".klink ";
      break;
    case MK_TRACE:
      cmd = ".ktrace ";
      break;
    case MK_MOTD:
      cmd = ".kmotd ";
      break;
    case MK_INFO:
      cmd = ".kinfo ";
      break;
    case MK_STATS:
      cmd = ".kflood ";
      break;
    case MK_PROXY:
      cmd = ".kproxy ";
      break;
    default:
      cmd = ".kline ";
      break;
  }

  return cmd + Dur + Mask;
}


void
suggestKline(const bool kline, const std::string & User,
  const std::string & Host, const bool different, const bool identd,
  const KlineType Reason)
{
  std::string Notice;
  std::string UserAtHost;
  
  UserAtHost = User + "@" + Host;

  BotSock::Address ip = users.getIP("", UserAtHost);

  if (std::string::npos != Host.find_first_of("*?"))
  {
    ::SendAll("Bogus DNS spoofed host " + UserAtHost, UF_OPER);
    return;
  }

  if (Config::IsOKHost(UserAtHost, ip) && Config::IsOper(UserAtHost, ip))
  {
    // Don't k-line our friendlies!
    return;
  }

  if (isDynamic("", Host))
  {
    if (vars[VAR_AUTO_KLINE_HOST]->getBool() &&
      vars[VAR_AUTO_PILOT]->getBool() && kline)
    {
      Notice = "Adding auto-kline for *@" + Host + " :" + GetReason(Reason);
      server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_HOST_TIME]->getInt(),
        "*@" + Host, GetReason(Reason));
    }
    else
    {
      Notice = Make_Kline("*@" + Host, Reason,
        vars[VAR_AUTO_KLINE_HOST_TIME]->getInt());
    }
  }
  else
  {
    std::string suggestedHost;
    if (isIP(Host))
    {
      suggestedHost = getDomain(Host, true) + "*";
    }
    else
    {
      suggestedHost = "*" + getDomain(Host, true);
    }

    if (identd)
    {
      if (different)
      {
        if (vars[VAR_AUTO_KLINE_HOST]->getBool() &&
          vars[VAR_AUTO_PILOT]->getBool() && kline)
        {
          Notice = "Adding auto-kline for *@" + Host + " :" +
            GetReason(Reason);
          server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_HOST_TIME]->getInt(),
            "*@" + Host, GetReason(Reason));
        }
        else
        {
          Notice = Make_Kline("*@" + Host, Reason,
            vars[VAR_AUTO_KLINE_HOST_TIME]->getInt());
        }
      }
      else
      {
        if (vars[VAR_AUTO_KLINE_USERHOST]->getBool() &&
          vars[VAR_AUTO_PILOT]->getBool() && kline)
        {
          Notice = "Adding auto-kline for *" + User + "@" + suggestedHost +
            " :" + GetReason(Reason);
          server.kline("Auto-Kline",
	    vars[VAR_AUTO_KLINE_USERHOST_TIME]->getInt(),
            "*" + User + "@" + suggestedHost, GetReason(Reason));
        }
        else
        {
          Notice = Make_Kline("*" + User + "@" + suggestedHost, Reason,
            vars[VAR_AUTO_KLINE_USERHOST_TIME]->getInt());
        }
      }
    }
    else
    {
      if (vars[VAR_AUTO_KLINE_NOIDENT]->getBool() &&
        vars[VAR_AUTO_PILOT]->getBool() && kline)
      {
        Notice = "Adding auto-kline for ~*@" + suggestedHost + " :" +
          GetReason(Reason);
        server.kline("Auto-Kline", vars[VAR_AUTO_KLINE_NOIDENT_TIME]->getInt(),
          "~*@" + suggestedHost, GetReason(Reason));
      }
      else
      {
        Notice = Make_Kline("~*@" + suggestedHost, Reason,
          vars[VAR_AUTO_KLINE_NOIDENT_TIME]->getInt());
      }
    }
  }

  if (Notice.length() > 0)
  {
    ::SendAll(Notice, UF_OPER, WATCH_KLINES, NULL);
  }
}


void
Init_Nick_Change_Table()
{
  nickChanges.clear();
}


static void
addToNickChangeList(const std::string & userhost, const std::string & oldNick,
  const std::string & lastNick)
{
  time_t currentTime = time(NULL);
   
  bool foundEntry = false;
   
  for (std::list<NickChangeEntry>::iterator ncp = nickChanges.begin();
    ncp != nickChanges.end(); ++ncp)
  {         
    time_t timeDifference = currentTime - ncp->lastNickChange;

    /* is it stale ? */
    if (timeDifference >= vars[VAR_NICK_CHANGE_T2_TIME]->getInt())
    {
      nickChanges.erase(ncp);
      --ncp;
    }
    else
    {
      /* how many 10 second intervals do we have? */
      int timeTicks = timeDifference / vars[VAR_NICK_CHANGE_T1_TIME]->getInt();

      /* is it stale? */
      if (timeTicks >= ncp->nickChangeCount)
      {
	nickChanges.erase(ncp);
	--ncp;
      }
      else
      {
        /* just decrement 10 second units of nick changes */
        ncp->nickChangeCount -= timeTicks;
        if (ncp->userhost == userhost)
        {
          ncp->lastNickChange = currentTime;
          ncp->lastNick = lastNick;
          ncp->nickChangeCount++;
          foundEntry = true;
        }

        /* now, check for a nick flooder */

        if ((ncp->nickChangeCount >= vars[VAR_NICK_CHANGE_MAX_COUNT]->getInt())
	  && !ncp->noticed)
        {
	  std::string notice("Nick flood " + ncp->userhost + " (" +
	    ncp->lastNick + ") " + IntToStr(ncp->nickChangeCount) + " in " +
	    IntToStr(ncp->lastNickChange - ncp->firstNickChange) + " seconds");

          ::SendAll(notice, UF_OPER);
	  Log::Write(notice);

	  BotSock::Address ip = users.getIP(oldNick, userhost);

          if (!Config::IsOKHost(userhost, ip) && !Config::IsOper(userhost, ip))
	  {
            doAction(lastNick, userhost, ip,
              vars[VAR_NICK_FLOOD_ACTION]->getAction(),
              vars[VAR_NICK_FLOOD_ACTION]->getInt(),
	      vars[VAR_NICK_FLOOD_REASON]->getString(), true);
          }

          ncp->noticed = true;
        }
      }
    }
  }

  if (!foundEntry)
  {
    NickChangeEntry ncp;

    ncp.userhost = userhost;
    ncp.firstNickChange = currentTime;
    ncp.lastNickChange = currentTime;
    ncp.nickChangeCount = 1;
    ncp.noticed = false;

    nickChanges.push_back(ncp);
  }
}


void Init_Link_Look_Table()
{
  linkLookers.clear();
}


// onTraceUser(Type, Class, Nick, UserHost, IP)
//
//
//
void
onTraceUser(const std::string & Type, const std::string & Class,
  const std::string & Nick, std::string UserHost,
  std::string IP = "")
{
  if ((Type == "") || (Nick == "") || (UserHost == ""))
    return;

  // Remove the brackets surrounding the user@host
  if (UserHost.length() >= 2)
  {
    UserHost = UserHost.substr(1, UserHost.length() - 2);
  }

  // This might not be necessary, but we'll do it just in case...
  if ((UserHost.length() >= 3) && (UserHost.substr(0, 3) == "(+)"))
  {
    UserHost = UserHost.substr(3, std::string::npos);
  }

  // Remove the parentheses surrounding the IP
  if (IP.length() >= 2)
  {
    IP = IP.substr(1, IP.length() - 2);
  }

  users.add(Nick, UserHost, IP, true, (Type == "Oper"), Class);
}


// onTraceUser(Type, Class, NUH, IP)
//
//
//
void
onTraceUser(const std::string & Type, const std::string & Class,
  const std::string & NUH, std::string IP = "")
{
  std::string Nick;
  std::string UserHost;

  if ((Type == "") || (NUH == ""))
    return;

  // Split the nick and user@host from the nick[user@host]
  Nick = chopNick(NUH);
  UserHost = chopUserhost(NUH);

  // hybrid-6 borked trace :P
  if ((UserHost.length() >= 3) && (UserHost.substr(0, 3) == "(+)"))
  {
    UserHost = UserHost.substr(3);
  }

  // Remove the parentheses surrounding the IP
  if (IP.length() >= 2)
  {
    IP = IP.substr(1, IP.length() - 2);
  }

  users.add(Nick, UserHost, IP, true, (Type == "Oper"), Class);
}


// onETraceUser(Type, Class, Nick, User, Host, IP, Gecos)
//
//
//
void
onETraceUser(const std::string & Type, const std::string & Class,
  const std::string & Nick, std::string User, const std::string & Host,
  const std::string & IP, const std::string & Gecos)
{
  if ((Type == "") || (Nick == "") || (User == "") || (Host == ""))
    return;

  users.add(Nick, User + '@' + Host, IP, true, (Type == "Oper"), Class, Gecos);
}


void
onClientConnect(std::string Text)
{
// IRC >> :plasma.toast.pc NOTICE OOMon :*** Notice -- Client connecting: Toast (toast@Plasma.Toast.PC) [192.168.1.1] {opers} [blah]
  std::string Nick = FirstWord(Text);
  std::string UH = FirstWord(Text);
  std::string IP = FirstWord(Text);
  std::string Class = FirstWord(Text);
  std::string Gecos = Text;
  if ((UH[0] == '(') && (UH[UH.length() - 1] == ')'))
  {
    // Remove the parentheses surrounding the user@host and copy to a char
    // array for later disposal.
    if (UH.length() >= 2)
    {
      UH = UH.substr(1, UH.length() - 2);
    }

    // Remove the brackets surrounding the IP
    if (IP.length() >= 2)
    {
      IP = IP.substr(1, IP.length() - 2);
    }

    // Remove the curly brackets surrounding the class
    if (Class.length() >= 2)
    {
      Class = Class.substr(1, Class.length() - 2);
    }

    if ((Gecos.length() >= 2) && (Gecos[0] == '[') &&
      (Gecos[Gecos.length() - 1] == ']'))
    {
      Gecos = Gecos.substr(1, Gecos.length() - 2);
    }
    else
    {
      Gecos = "";
    }

    connects.onNotice(Nick + ' ' + UH + ' ' + IP);

    users.add(Nick, UH, IP, false, false, Class, Gecos);
  }
}


void
onClientExit(std::string Text)
{
  // "cihuyy (ngocol@p6.nas2.is4.u-net.net) [Leaving] [195.102.196.13]"
  std::string Nick = FirstWord(Text);
  std::string UH = FirstWord(Text);
  if (UH.length() >= 2)
  {
    UH = UH.substr(1, UH.length() - 2);

    BotSock::Address ip;

    std::string::size_type ipFind = Text.rfind(' ');
    if (std::string::npos != ipFind)
    {
      std::string IP = Text.substr(ipFind + 1);
      if (IP.length() >= 2)
      {
        IP = IP.substr(1, IP.length() - 2);
      }
      ip = BotSock::inet_addr(IP);
    }
    else
    {
      ip = 0;
    }

    users.remove(Nick, UH, ip);
  }
}

void Bot_Reject(std::string Text)
{
}

void CS_Clones(std::string Text)
{
  bool identd = true;
  char server_notice[MAX_BUFF];
  char *user;
  char *host;
  char *p;
  char *nick_reported;
  char *user_host; 

  strncpy(server_notice, Text.c_str(), sizeof(server_notice));
  server_notice[sizeof(server_notice) - 1] = 0;

  nick_reported = strtok(server_notice," ");
  if (nick_reported == NULL) return;
    
  user_host = strtok(NULL, " ");
  if (user_host == NULL) return;
    
/*
  Lets try and get it right folks... [user@host] or (user@host)
*/  
 
  if(*user_host == '[')
    {
      user_host++;
      p = strrchr(user_host,']');
      if(p)  
        *p = '\0';
    }
  else if(*user_host == '(')
    {
      user_host++;
      p = strrchr(user_host,')');
      if(p)
        *p = '\0'; 
    }
  
  ::SendAll("CS clones: " + std::string(user_host), UF_OPER);
  Log::Write("CS clones: " + std::string(user_host));
  
  p = strdup(user_host);
  user = strtok(user_host,"@");
  if(*user == '~')
    {
      user++;
      identd = false;
    } 
      
  host = strtok(NULL, "");
    
  suggestKline(false,user,host,false,identd,MK_CLONES);
  
  delete p;
}     

void CS_Nick_Flood(std::string Text)
{
  char server_notice[MAX_BUFF];
  char *nick_reported;
  char *user_host;
  char *p;

  strncpy(server_notice, Text.c_str(), sizeof(server_notice));
  server_notice[sizeof(server_notice) - 1] = 0;

  nick_reported = strtok(server_notice," ");
  if (nick_reported == NULL) return;
      
  user_host = strtok(NULL, " ");
  if (user_host == NULL) return;

/*    
  Lets try and get it right folks... [user@host] or (user@host)
*/      
      
  if(*user_host == '[')
  {
    user_host++;
    p = strrchr(user_host,']');
    if(p)
      *p = '\0';
  }
  else if(*user_host == '(')
  {
    user_host++;
    p = strrchr(user_host,')');
    if(p)
      *p = '\0'; 
  } 
  
  ::SendAll("CS nick flood: " + std::string(user_host), UF_OPER);
  Log::Write("CS nick flood: " + std::string(user_host));

  BotSock::Address ip = users.getIP(nick_reported, user_host);
      
  if ((!Config::IsOKHost(user_host, ip)) && (!Config::IsOper(user_host, ip)))
  {
    doAction(nick_reported, user_host, ip,
      vars[VAR_NICK_FLOOD_ACTION]->getAction(),
      vars[VAR_NICK_FLOOD_ACTION]->getInt(),
      vars[VAR_NICK_FLOOD_REASON]->getString(), true);
  } 
}     


void
onNickChange(std::string Text)
{
  std::string nick1, nick2, userhost;

// From ToastTEST to Toast1234 [toast@Plasma.Toast.PC]

  if (FirstWord(Text) != "From")
    return;

  nick1 = FirstWord(Text);

  if (FirstWord(Text) != "to")
    return;

  nick2 = FirstWord(Text);

  userhost = FirstWord(Text);

  // Remove the brackets surrounding the user@host
  if (userhost.length() >= 2)
  {
    userhost = userhost.substr(1, userhost.length() - 2);
  }

  addToNickChangeList(server.downCase(userhost), nick1, nick2);
  users.updateNick(nick1, userhost, nick2);
}


// "OOMon. From Toast Path: plasma.engr.Arizona.EDU!Toast (test)"
void Kill_Add_Report(std::string Text)
{
  bool Global;

  std::string Nick = FirstWord(Text); // Killed nick
  if (Nick[Nick.length() - 1] == '.') // Remove the trailing .
    Nick = Nick.substr(0, Nick.length() - 1);
  if (users.have(Nick)) {
    // We only want to report kills on local users
    FirstWord(Text); // "From"
    std::string Killer = FirstWord(Text); // Killer's nick
    if ((std::string::npos != Killer.find('@')) || 
      (std::string::npos != Killer.find('!')) || 
      (std::string::npos == Killer.find('.')))
    {
      // We don't want to see server kills :P
      FirstWord(Text); // "Path:"
      std::string Path = FirstWord(Text); // Path
      // Hybrid-6 changes the format of kill paths
      Global = !users.have(Killer);
      if (Global) {
        ::SendAll("Global kill for " + Nick + " by " + Killer + " " + Text,
	  UF_OPER, WATCH_KILLS, NULL);
	Log::Write("Global kill for " + Nick + " by " + Killer + " " + Text);
      } else {
        ::SendAll("Local kill for " + Nick + " by " + Killer + " " + Text,
	  UF_OPER, WATCH_KILLS, NULL);
	Log::Write("Local kill for " + Nick + " by " + Killer + " " + Text);
      }
    }
  }
}


void
onLinksNotice(std::string text)
{
  std::string notice("[LINKS " + text + "]");

  if ((text.length() > 0) && (text[0] == '\''))
  {
    /* This is a +th server, skip the '...' part */

    std::string::size_type end = text.find('\'', 1);

    if (std::string::npos == end)
    {
      // broken. ignore it.
      return;
    }
    else
    {
      std::string::size_type next = text.find_first_not_of(' ', end + 1);

      if (std::string::npos == next)
      {
	// broken. ignore it.
	return;
      }
      else
      {
        text = text.substr(next);
      }
    }
  }

  linkLookers.onNotice(notice, text);
}


void
onTraceNotice(const std::string & text)
{
  std::string notice("[TRACE " + text + "]");

  traceLookers.onNotice(notice, text);
}


void
onMotdNotice(const std::string & text)
{
  std::string notice("[MOTD " + text + "]");

  motdLookers.onNotice(notice, text);
}


void
onInfoNotice(const std::string & text)
{
  std::string notice("[INFO " + text + "]");

  infoLookers.onNotice(notice, text);
}


void
onStatsNotice(std::string text)
{
  std::string notice("[STATS " + text + "]");

  std::string statsType = FirstWord(text);

  if (vars[VAR_STATSP_REPLY]->getBool() && ((statsType == "p") ||
    (vars[VAR_STATSP_CASE_INSENSITIVE]->getBool() && (statsType == "P"))))
  {
    std::string copy = text;

    if (server.downCase(FirstWord(copy)) != "requested")
    {
      // broken.  ignore it.
      return;
    }

    if (server.downCase(FirstWord(copy)) != "by")
    {
      // broken.  ignore it.
      return;
    }

    std::string nick = FirstWord(copy);

    if (nick == "")
    {
      // broken.  ignore it.
      return;
    }

    if (!vars[VAR_WATCH_STATS_NOTICES]->getBool())
    {
      ::SendAll(notice, UF_OPER);
    }

    // Assume this is a server with "/stats p" support.  Only report
    // bot DCC connections with +O status
    StrList Output;
    clients.statsP(Output);

    std::string message = vars[VAR_STATSP_MESSAGE]->getString();
    if (message.length() > 0)
    {
      Output.push_back(message);
    }

    server.notice(std::string(nick), Output);
  }

  if (vars[VAR_WATCH_STATS_NOTICES]->getBool())
  {
    statsLookers.onNotice(notice, text);
  }
}


void
onFlooderNotice(const std::string & text)
{
  possibleFlooders.onNotice(text);
}


void
onSpambotNotice(const std::string & text)
{
  spambots.onNotice(text);
}


void
onTooManyConnNotice(const std::string & text)
{
  tooManyConn.onNotice(text);
}


void
addIP(std::string ip)
{
  int first_empty_entry = -1;
  bool found_entry = false;
  int i;

  time_t current_time = time(NULL);

  BotSock::Address IP = BotSock::inet_addr(ip);

  ip = ip.substr(0, ip.rfind('.')) + ".";

  for (i = 0; i < IP_TABLE_SIZE; i++ )
  {
    if (ips[i].net)
    { 
      if (ips[i].net == (IP & BotSock::vhost_netmask))
      {
        found_entry = true;
  
        /* if its an old old entry, let it drop to 0, then start counting
           (this should be very unlikely case)
         */
        if ((ips[i].last_ip + MAX_IP_TIME) < current_time)
        {
          ips[i].ip_count = 0;
          ips[i].total_count = 0;
          ips[i].first_ip = current_time;
        }

        ips[i].ip_count++;
        ips[i].total_count++;

        if (ips[i].ip_count >= MAX_IP_CONNECTS)
        {
          char buffer[MAX_BUFF];
          sprintf(buffer,
	    "Possible VHost clones from *@%s* (%d connects in %ld seconds)",
	    ip.c_str(), ips[i].total_count,
	    static_cast<long>(current_time - ips[i].first_ip));
          ::SendAll(buffer, UF_OPER);
          Log::Write(buffer);

          ips[i].ip_count = 0;
          ips[i].last_ip = current_time;
        }       
        else            
        {     
          ips[i].last_ip = current_time;
        }     
      }         
      else
      {         
        if ((ips[i].last_ip + MAX_IP_TIME) < current_time)
        {     
          ips[i].net = 0;
        }   
      }
    }         
    else
    {         
      if (first_empty_entry < 0)
      {
        first_empty_entry = i;
      }
    }             
  }               
                
/*            
   If this is a new entry, then found_entry will still be NO
*/                
  if (!found_entry)    
  {     
    if (first_empty_entry >= 0)
    {     
      ips[first_empty_entry].net = IP & BotSock::vhost_netmask;
      ips[first_empty_entry].last_ip = current_time;
      ips[first_empty_entry].first_ip = current_time;
      ips[first_empty_entry].ip_count = 1;
      ips[first_empty_entry].total_count = 1;
    }   
  }
}


void
onDetectDNSBLOpenProxy(const std::string & ip, const std::string & nick,
  const std::string & userhost)
{
  // This is a proxy listed by the DNSBL
  std::string notice = std::string("DNSBL Open Proxy detected for ") +
    nick + "!" + userhost + " [" + ip + "]";
  Log::Write(notice);
  ::SendAll(notice, UF_OPER);

  doAction(nick, userhost, BotSock::inet_addr(ip),
    vars[VAR_DNSBL_PROXY_ACTION]->getAction(),
    vars[VAR_DNSBL_PROXY_ACTION]->getInt(),
    vars[VAR_DNSBL_PROXY_REASON]->getString(), false);
}


static	bool
CheckDNSBL(const std::string & ip, const std::string & nick,
  const std::string & userhost)
{
  std::string dns = vars[VAR_DNSBL_PROXY_ZONE]->getString();

  if ((dns.length() > 0) && isIP(ip))
  {
    std::string::size_type pos = 0;

    // Reverse the order of the octets and use them to prefix the blackhole
    // zone.
    int dots = CountChars(ip, '.');
    while (dots >= 0)
    {
      std::string::size_type nextDot = ip.find('.', pos);
      dns = ip.substr(pos, nextDot - pos) + "." + dns;
      pos = nextDot + 1;
      dots--;
    }

    struct hostent *result = gethostbyname(dns.c_str());

    if (NULL != result)
    {
      onDetectDNSBLOpenProxy(ip, nick, userhost);

      return true;
    }
  }
  return false;
}

void
CheckProxy(const std::string & ip, const std::string & host,
  const std::string & nick, const std::string & userhost)
{
  // If the IP is listed by the DNSBL, there's no reason to do a Wingate
  // or SOCKS proxy check of our own!
  if (CheckDNSBL(ip, nick, userhost))
  {
    return;
  }

  if (vars[VAR_SCAN_FOR_PROXIES]->getBool())
  {
    Proxy::check(ip, host, nick, userhost);
  }
}


void
status(StrList & output)
{
  output.push_back("Nick changers: " + IntToStr(nickChanges.size()));
  output.push_back("Connect flooders: " + IntToStr(connects.size()));
  if (vars[VAR_WATCH_LINKS_NOTICES]->getBool())
  {
    output.push_back("Links lookers: " + IntToStr(linkLookers.size()));
  }
  if (vars[VAR_WATCH_TRACE_NOTICES]->getBool())
  {
    output.push_back("Trace lookers: " + IntToStr(traceLookers.size()));
  }
  if (vars[VAR_WATCH_MOTD_NOTICES]->getBool())
  {
    output.push_back("Motd lookers: " + IntToStr(motdLookers.size()));
  }
  if (vars[VAR_WATCH_INFO_NOTICES]->getBool())
  {
    output.push_back("Info lookers: " + IntToStr(infoLookers.size()));
  }
  if (vars[VAR_WATCH_STATS_NOTICES]->getBool())
  {
    output.push_back("Stats lookers: " + IntToStr(statsLookers.size()));
  }
  if (vars[VAR_WATCH_FLOODER_NOTICES]->getBool())
  {
    output.push_back("Possible flooders: " + IntToStr(possibleFlooders.size()));
  }
  if (vars[VAR_WATCH_SPAMBOT_NOTICES]->getBool())
  {
    output.push_back("Possible spambots: " + IntToStr(spambots.size()));
  }
  if (vars[VAR_WATCH_TOOMANY_NOTICES]->getBool())
  {
    output.push_back("Too many connections: " + IntToStr(tooManyConn.size()));
  }

  if (vars[VAR_WATCH_JUPE_NOTICES]->getBool())
  {
    jupeJoiners.status(output);
  }

  output.push_back("Up Time: " + ::getUptime());
}


void
onOperNotice(std::string text)
{
  std::string nick = FirstWord(text);
  std::string userhost = FirstWord(text);

  if (userhost.length() > 2)
  {
    userhost = userhost.substr(1, userhost.length() - 2);
  }

  if ("is" != server.downCase(FirstWord(text)))
    return;
  if ("now" != server.downCase(FirstWord(text)))
    return;
  if ("an" != server.downCase(FirstWord(text)))
    return;
  if ("operator" != server.downCase(FirstWord(text)))
    return;

  users.updateOper(nick, userhost, true);
}


bool
checkForSpoof(const std::string & nick, const std::string & user, 
  const std::string & host, const std::string & ip)
{
  if (vars[VAR_CHECK_FOR_SPOOFS]->getBool())
  {
    std::string userhost = user + '@' + host;

    if (!Config::IsOper(userhost, ip) && !Config::IsOKHost(userhost, ip) &&
      !Config::IsSpoofer(ip))
    {
      if (isIP(host))
      {
        // If we're dealing with an IP that that doesn't reverse-resolve,
        // make sure the "hostname" and ip match up.
        if ((ip != "") && isIP(host))
        {
          if ((host != ip))
          {
            std::string notice("Fake IP Spoof: " + nick + " (" + userhost +
	      ") [" + ip + "]");
	    Log::Write(notice);
	    ::SendAll(notice, UF_OPER);
	    doAction(nick, userhost, BotSock::inet_addr(ip),
	      vars[VAR_FAKE_IP_SPOOF_ACTION]->getAction(),
	      vars[VAR_FAKE_IP_SPOOF_ACTION]->getInt(),
	      vars[VAR_FAKE_IP_SPOOF_REASON]->getString(), false);
            return true;
          }
        }
      }
      else
      {
        std::string::size_type lastDot = host.rfind('.');
        if (std::string::npos != lastDot)
        {
          std::string tld = host.substr(lastDot + 1);

          std::string::size_type len = tld.length();

          bool legal_top_level = false;

          if (2 == len)
	  {
	    legal_top_level = true;
	  }
          else if (3 == len)
          {
            // Don't forget .ARPA !!!! :P
            if (tld == "net") legal_top_level = true;
            if (tld == "com") legal_top_level = true;
            if (tld == "org") legal_top_level = true;
            if (tld == "gov") legal_top_level = true;
            if (tld == "edu") legal_top_level = true;
            if (tld == "mil") legal_top_level = true;
            if (tld == "int") legal_top_level = true;
            if (tld == "biz") legal_top_level = true;
          }
	  else if (4 == len)
	  {
            if (tld == "arpa") legal_top_level = true;
            if (tld == "info") legal_top_level = true;
            if (tld == "name") legal_top_level = true;
	  }

          if (!legal_top_level)
          {
	    std::string notice("Illegal TLD Spoof: " + nick + " (" + userhost +
	      ") [" + ip + "]");
	    Log::Write(notice);
	    ::SendAll(notice, UF_OPER);
	    doAction(nick, userhost, BotSock::inet_addr(ip),
	      vars[VAR_ILLEGAL_TLD_SPOOF_ACTION]->getAction(),
	      vars[VAR_ILLEGAL_TLD_SPOOF_ACTION]->getInt(),
	      vars[VAR_ILLEGAL_TLD_SPOOF_REASON]->getString(), false);
            return true;
          }
        }
        if (std::string::npos != host.find_first_of("@*?"))
        {
	  std::string notice("Illegal Character Spoof: " + nick + " (" +
	    userhost + ") [" + ip + "]");
	  Log::Write(notice);
	  ::SendAll(notice, UF_OPER);
	  doAction(nick, userhost, BotSock::inet_addr(ip),
	    vars[VAR_ILLEGAL_CHAR_SPOOF_ACTION]->getAction(),
	    vars[VAR_ILLEGAL_CHAR_SPOOF_ACTION]->getInt(),
	    vars[VAR_ILLEGAL_CHAR_SPOOF_REASON]->getString(), false);
          return true;
        }
      }
    }
  }
  return false;
}


void
onInvalidUsername(std::string text)
{
  std::string nick = FirstWord(text);

  std::string userhost = FirstWord(text);
  if (userhost.length() >= 2)
  {
    userhost = userhost.substr(1, userhost.length() - 2);
  }

  doAction(nick, userhost, INADDR_NONE,
    vars[VAR_INVALID_USERNAME_ACTION]->getAction(),
    vars[VAR_INVALID_USERNAME_ACTION]->getInt(),
    vars[VAR_INVALID_USERNAME_REASON]->getString(), false);
}


void
onClearTempKlines(const std::string & text)
{
  std::string copy = text;

  std::string nick = FirstWord(copy);

  if (copy == "is clearing temp klines")
  {
    ::SendAll("*** " + text, UF_OPER, WATCH_KLINES);
    Log::Write("*** " + text);
    server.reloadKlines();
  }
}


void
onClearTempDlines(const std::string & text)
{
  std::string copy = text;

  std::string nick = FirstWord(copy);

  if (copy == "is clearing temp dlines")
  {
    ::SendAll("*** " + text, UF_OPER, WATCH_KLINES);
    Log::Write("*** " + text);
    server.reloadDlines();
  }
}


void
onGlineRequest(const std::string & text)
{
  std::string copy = text;

  std::string nuh = FirstWord(copy);

  if ("on" != server.downCase(FirstWord(copy)))
    return;

  std::string serverName = FirstWord(copy);

  std::string ishas = FirstWord(copy);

  std::string voodoo;

  if (server.downCase(ishas) == "is")
  {
    if ("requesting" != server.downCase(FirstWord(copy)))
      return;

    voodoo = " requested ";
  }
  else if ("has" == server.downCase(ishas))
  {
    if ("triggered" != server.downCase(FirstWord(copy)))
      return;
    
    voodoo = " triggered ";
  }
  else return;

  if ("gline" != server.downCase(FirstWord(copy)))
    return;

  if ("for" != server.downCase(FirstWord(copy)))
    return;

  std::string mask = FirstWord(copy);
  if (mask.length() > 2)
  {
    mask = mask.substr(1, mask.length() - 2);
  }

  std::string reason = copy;
  if (reason.length() > 2)
  {
    reason = reason.substr(1, reason.length() - 2);
  }

  std::string msg(getNick(nuh) + voodoo + "G-line: " + mask + " (" + reason +
    ')');

  ::SendAll(msg, UF_OPER, WATCH_GLINES);
  Log::Write(msg);
}

