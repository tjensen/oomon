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

// Std C++ Headers
#include <fstream>
#include <string>

// OOMon Headers
#include "strtype"
#include "vars.h"
#include "setting.h"
#include "defaults.h"
#include "main.h"
#include "log.h"
#include "util.h"


#ifdef DEBUG
# define VARS_DEBUG
#endif


Vars vars;


Vars::Vars()
  : vec(VAR_COUNT)
{
  this->vec[VAR_AUTO_KLINE_HOST] =
    new BoolSetting("AUTO_KLINE_HOST", DEFAULT_AUTO_KLINE_HOST);
  this->vec[VAR_AUTO_KLINE_HOST_TIME] =
    new IntSetting("AUTO_KLINE_HOST_TIME", DEFAULT_AUTO_KLINE_HOST_TIME, 0);
  this->vec[VAR_AUTO_KLINE_NOIDENT] =
    new BoolSetting("AUTO_KLINE_NOIDENT", DEFAULT_AUTO_KLINE_NOIDENT);
  this->vec[VAR_AUTO_KLINE_NOIDENT_TIME] =
    new IntSetting("AUTO_KLINE_NOIDENT_TIME", DEFAULT_AUTO_KLINE_NOIDENT_TIME, 0);
  this->vec[VAR_AUTO_KLINE_USERHOST] =
    new BoolSetting("AUTO_KLINE_USERHOST", DEFAULT_AUTO_KLINE_USERHOST);
  this->vec[VAR_AUTO_KLINE_USERHOST_TIME] =
    new IntSetting("AUTO_KLINE_USERHOST_TIME", DEFAULT_AUTO_KLINE_USERHOST_TIME, 0);
  this->vec[VAR_AUTO_PILOT] = new BoolSetting("AUTO_PILOT", DEFAULT_AUTO_PILOT);
  this->vec[VAR_AUTO_SAVE] = new BoolSetting("AUTO_SAVE", DEFAULT_AUTO_SAVE);
  this->vec[VAR_BROKEN_HOSTNAME_MUNGING] =
    new BoolSetting("BROKEN_HOSTNAME_MUNGING", DEFAULT_BROKEN_HOSTNAME_MUNGING);
  this->vec[VAR_CHECK_FOR_SPOOFS] =
    new BoolSetting("CHECK_FOR_SPOOFS", DEFAULT_CHECK_FOR_SPOOFS);
  this->vec[VAR_DEFAULT_DLINE_TIMEOUT] =
    new IntSetting("DEFAULT_DLINE_TIMEOUT", DEFAULT_DEFAULT_DLINE_TIMEOUT, 0);
  this->vec[VAR_DEFAULT_KLINE_TIMEOUT] =
    new IntSetting("DEFAULT_KLINE_TIMEOUT", DEFAULT_DEFAULT_KLINE_TIMEOUT, 0);
  this->vec[VAR_DNSBL_PROXY_ACTION] =
    new ActionSetting("DNSBL_PROXY_ACTION", DEFAULT_DNSBL_PROXY_ACTION);
  this->vec[VAR_DNSBL_PROXY_REASON] =
    new StrSetting("DNSBL_PROXY_REASON", DEFAULT_DNSBL_PROXY_REASON);
  this->vec[VAR_DNSBL_PROXY_ZONE] =
    new StrSetting("DNSBL_PROXY_ZONE", DEFAULT_DNSBL_PROXY_ZONE);
  this->vec[VAR_EXTRA_KLINE_INFO] =
    new BoolSetting("EXTRA_KLINE_INFO", DEFAULT_EXTRA_KLINE_INFO);
  this->vec[VAR_FAKE_IP_SPOOF_ACTION] =
    new ActionSetting("FAKE_IP_SPOOF_ACTION", DEFAULT_FAKE_IP_SPOOF_ACTION);
  this->vec[VAR_FAKE_IP_SPOOF_REASON] =
    new StrSetting("FAKE_IP_SPOOF_REASON", DEFAULT_FAKE_IP_SPOOF_REASON);
  this->vec[VAR_FLOODER_ACTION] =
    new ActionSetting("FLOODER_ACTION", DEFAULT_FLOODER_ACTION);
  this->vec[VAR_FLOODER_MAX_COUNT] =
    new IntSetting("FLOODER_MAX_COUNT", DEFAULT_FLOODER_MAX_COUNT, 0);
  this->vec[VAR_FLOODER_MAX_TIME] =
    new IntSetting("FLOODER_MAX_TIME", DEFAULT_FLOODER_MAX_TIME, 1);
  this->vec[VAR_FLOODER_REASON] =
    new StrSetting("FLOODER_REASON", DEFAULT_FLOODER_REASON);
  this->vec[VAR_GLIST_FORMAT] =
    new StrSetting("GLIST_FORMAT", DEFAULT_GLIST_FORMAT);
  this->vec[VAR_IGNORE_UNKNOWN_COMMAND] =
    new BoolSetting("IGNORE_UNKNOWN_COMMAND", DEFAULT_IGNORE_UNKNOWN_COMMAND);
  this->vec[VAR_ILLEGAL_CHAR_SPOOF_ACTION] =
    new ActionSetting("ILLEGAL_CHAR_SPOOF_ACTION", DEFAULT_ILLEGAL_CHAR_SPOOF_ACTION);
  this->vec[VAR_ILLEGAL_CHAR_SPOOF_REASON] =
    new StrSetting("ILLEGAL_CHAR_SPOOF_REASON", DEFAULT_ILLEGAL_CHAR_SPOOF_REASON);
  this->vec[VAR_ILLEGAL_TLD_SPOOF_ACTION] =
    new ActionSetting("ILLEGAL_TLD_SPOOF_ACTION", DEFAULT_ILLEGAL_TLD_SPOOF_ACTION);
  this->vec[VAR_ILLEGAL_TLD_SPOOF_REASON] =
    new StrSetting("ILLEGAL_TLD_SPOOF_REASON", DEFAULT_ILLEGAL_TLD_SPOOF_REASON);
  this->vec[VAR_INFO_FLOOD_ACTION] =
    new ActionSetting("INFO_FLOOD_ACTION", DEFAULT_INFO_FLOOD_ACTION);
  this->vec[VAR_INFO_FLOOD_MAX_COUNT] =
    new IntSetting("INFO_FLOOD_MAX_COUNT", DEFAULT_INFO_FLOOD_MAX_COUNT, 1);
  this->vec[VAR_INFO_FLOOD_MAX_TIME] =
    new IntSetting("INFO_FLOOD_MAX_TIME", DEFAULT_INFO_FLOOD_MAX_TIME, 1);
  this->vec[VAR_INFO_FLOOD_REASON] =
    new StrSetting("INFO_FLOOD_REASON", DEFAULT_INFO_FLOOD_REASON);
  this->vec[VAR_INVALID_USERNAME_ACTION] =
    new ActionSetting("INVALID_USERNAME_ACTION", DEFAULT_INVALID_USERNAME_ACTION);
  this->vec[VAR_INVALID_USERNAME_REASON] =
    new StrSetting("INVALID_USERNAME_REASON", DEFAULT_INVALID_USERNAME_REASON);
  this->vec[VAR_JUPE_JOIN_ACTION] =
    new ActionSetting("JUPE_JOIN_ACTION", DEFAULT_JUPE_JOIN_ACTION,
    DEFAULT_JUPE_JOIN_ACTION_TIME);
  this->vec[VAR_JUPE_JOIN_IGNORE_CHANNEL] =
    new BoolSetting("JUPE_JOIN_IGNORE_CHANNEL", DEFAULT_JUPE_JOIN_IGNORE_CHANNEL);
  this->vec[VAR_JUPE_JOIN_MAX_COUNT] =
    new IntSetting("JUPE_JOIN_MAX_COUNT", DEFAULT_JUPE_JOIN_MAX_COUNT, 0);
  this->vec[VAR_JUPE_JOIN_MAX_TIME] =
    new IntSetting("JUPE_JOIN_MAX_TIME", DEFAULT_JUPE_JOIN_MAX_TIME, 0);
  this->vec[VAR_JUPE_JOIN_REASON] =
    new StrSetting("JUPE_JOIN_REASON", DEFAULT_JUPE_JOIN_REASON);
  this->vec[VAR_KILLLIST_REASON] =
    new StrSetting("KILLLIST_REASON", DEFAULT_KILLLIST_REASON);
  this->vec[VAR_KILLNFIND_REASON] =
    new StrSetting("KILLNFIND_REASON", DEFAULT_KILLNFIND_REASON);
  this->vec[VAR_LINKS_FLOOD_ACTION] =
    new ActionSetting("LINKS_FLOOD_ACTION", DEFAULT_LINKS_FLOOD_ACTION);
  this->vec[VAR_LINKS_FLOOD_MAX_COUNT] =
    new IntSetting("LINKS_FLOOD_MAX_COUNT", DEFAULT_LINKS_FLOOD_MAX_COUNT, 1);
  this->vec[VAR_LINKS_FLOOD_MAX_TIME] =
    new IntSetting("LINKS_FLOOD_MAX_TIME", DEFAULT_LINKS_FLOOD_MAX_TIME, 1);
  this->vec[VAR_LINKS_FLOOD_REASON] =
    new StrSetting("LINKS_FLOOD_REASON", DEFAULT_LINKS_FLOOD_REASON);
  this->vec[VAR_LIST_FORMAT] =
    new StrSetting("LIST_FORMAT", DEFAULT_LIST_FORMAT);
  this->vec[VAR_MOTD_FLOOD_ACTION] =
    new ActionSetting("MOTD_FLOOD_ACTION", DEFAULT_MOTD_FLOOD_ACTION);
  this->vec[VAR_MOTD_FLOOD_MAX_COUNT] =
    new IntSetting("MOTD_FLOOD_MAX_COUNT", DEFAULT_MOTD_FLOOD_MAX_COUNT, 1);
  this->vec[VAR_MOTD_FLOOD_MAX_TIME] =
    new IntSetting("MOTD_FLOOD_MAX_TIME", DEFAULT_MOTD_FLOOD_MAX_TIME, 1);
  this->vec[VAR_MOTD_FLOOD_REASON] =
    new StrSetting("MOTD_FLOOD_REASON", DEFAULT_MOTD_FLOOD_REASON);
  this->vec[VAR_MULTI_MIN] = new IntSetting("MULTI_MIN", DEFAULT_MULTI_MIN, 1);
  this->vec[VAR_NFIND_FORMAT] =
    new StrSetting("NFIND_FORMAT", DEFAULT_NFIND_FORMAT);
  this->vec[VAR_NICK_CHANGE_MAX_COUNT] =
    new IntSetting("NICK_CHANGE_MAX_COUNT", DEFAULT_NICK_CHANGE_MAX_COUNT, 1);
  this->vec[VAR_NICK_CHANGE_T1_TIME] =
    new IntSetting("NICK_CHANGE_T1_TIME", DEFAULT_NICK_CHANGE_T1_TIME, 1);
  this->vec[VAR_NICK_CHANGE_T2_TIME] =
    new IntSetting("NICK_CHANGE_T2_TIME", DEFAULT_NICK_CHANGE_T2_TIME, 1);
  this->vec[VAR_NICK_FLOOD_ACTION] =
    new ActionSetting("NICK_FLOOD_ACTION", DEFAULT_NICK_FLOOD_ACTION);
  this->vec[VAR_NICK_FLOOD_REASON] =
    new StrSetting("NICK_FLOOD_REASON", DEFAULT_NICK_FLOOD_REASON);
  this->vec[VAR_OPER_IN_MULTI] =
    new BoolSetting("OPER_IN_MULTI", DEFAULT_OPER_IN_MULTI);
  this->vec[VAR_OPER_NICK_IN_REASON] =
    new BoolSetting("OPER_NICK_IN_REASON", DEFAULT_OPER_NICK_IN_REASON);
  this->vec[VAR_OPER_ONLY_DCC] =
    new BoolSetting("OPER_ONLY_DCC", DEFAULT_OPER_ONLY_DCC);
  this->vec[VAR_RELAY_MSGS_TO_LOCOPS] =
    new BoolSetting("RELAY_MSGS_TO_LOCOPS", DEFAULT_RELAY_MSGS_TO_LOCOPS);
  this->vec[VAR_SCAN_FOR_PROXIES] =
    new BoolSetting("SCAN_FOR_PROXIES", DEFAULT_SCAN_FOR_PROXIES);
  this->vec[VAR_SCAN_PROXY_ACTION] =
    new ActionSetting("SCAN_PROXY_ACTION", DEFAULT_SCAN_PROXY_ACTION);
  this->vec[VAR_SCAN_PROXY_REASON] =
    new StrSetting("SCAN_PROXY_REASON", DEFAULT_SCAN_PROXY_REASON);
  this->vec[VAR_SEEDRAND_ACTION] =
    new ActionSetting("SEEDRAND_ACTION", DEFAULT_SEEDRAND_ACTION);
  this->vec[VAR_SEEDRAND_COMMAND_MIN] =
    new IntSetting("SEEDRAND_COMMAND_MIN", DEFAULT_SEEDRAND_COMMAND_MIN);
  this->vec[VAR_SEEDRAND_FORMAT] =
    new StrSetting("SEEDRAND_FORMAT", DEFAULT_SEEDRAND_FORMAT);
  this->vec[VAR_SEEDRAND_REASON] =
    new StrSetting("SEEDRAND_REASON", DEFAULT_SEEDRAND_REASON);
  this->vec[VAR_SEEDRAND_REPORT_MIN] =
    new IntSetting("SEEDRAND_REPORT_MIN", DEFAULT_SEEDRAND_REPORT_MIN);
  this->vec[VAR_SERVER_TIMEOUT] =
    new ServerTimeoutSetting("SERVER_TIMEOUT", DEFAULT_SERVER_TIMEOUT);
  this->vec[VAR_SERVICES_CHECK_INTERVAL] =
    new IntSetting("SERVICES_CHECK_INTERVAL", DEFAULT_SERVICES_CHECK_INTERVAL, 1);
  this->vec[VAR_SERVICES_CLONE_LIMIT] =
    new IntSetting("SERVICES_CLONE_LIMIT", DEFAULT_SERVICES_CLONE_LIMIT, 2);
  this->vec[VAR_SPAMBOT_ACTION] =
    new ActionSetting("SPAMBOT_ACTION", DEFAULT_SPAMBOT_ACTION);
  this->vec[VAR_SPAMBOT_MAX_COUNT] =
    new IntSetting("SPAMBOT_MAX_COUNT", DEFAULT_SPAMBOT_MAX_COUNT, 0);
  this->vec[VAR_SPAMBOT_MAX_TIME] =
    new IntSetting("SPAMBOT_MAX_TIME", DEFAULT_SPAMBOT_MAX_TIME, 1);
  this->vec[VAR_SPAMBOT_REASON] =
    new StrSetting("SPAMBOT_REASON", DEFAULT_SPAMBOT_REASON);
  this->vec[VAR_SPAMTRAP_ACTION] =
    new ActionSetting("SPAMTRAP_ACTION", DEFAULT_SPAMTRAP_ACTION);
  this->vec[VAR_SPAMTRAP_DEFAULT_REASON] =
    new StrSetting("SPAMTRAP_DEFAULT_REASON", DEFAULT_SPAMTRAP_DEFAULT_REASON);
  this->vec[VAR_SPAMTRAP_ENABLE] =
    new BoolSetting("SPAMTRAP_ENABLE", DEFAULT_SPAMTRAP_ENABLE);
  this->vec[VAR_SPAMTRAP_MIN_SCORE] =
    new IntSetting("SPAMTRAP_MIN_SCORE", DEFAULT_SPAMTRAP_MIN_SCORE, 0);
  this->vec[VAR_SPAMTRAP_NICK] =
    new StrSetting("SPAMTRAP_NICK", DEFAULT_SPAMTRAP_NICK);
  this->vec[VAR_SPAMTRAP_USERHOST] =
    new StrSetting("SPAMTRAP_USERHOST", DEFAULT_SPAMTRAP_USERHOST);
  this->vec[VAR_STATS_FLOOD_ACTION] =
    new ActionSetting("STATS_FLOOD_ACTION", DEFAULT_STATS_FLOOD_ACTION);
  this->vec[VAR_STATS_FLOOD_MAX_COUNT] =
    new IntSetting("STATS_FLOOD_MAX_COUNT", DEFAULT_STATS_FLOOD_MAX_COUNT, 1);
  this->vec[VAR_STATS_FLOOD_MAX_TIME] =
    new IntSetting("STATS_FLOOD_MAX_TIME", DEFAULT_STATS_FLOOD_MAX_TIME, 1);
  this->vec[VAR_STATS_FLOOD_REASON] =
    new StrSetting("STATS_FLOOD_REASON", DEFAULT_STATS_FLOOD_REASON);
  this->vec[VAR_STATSP_CASE_INSENSITIVE] =
    new BoolSetting("STATSP_CASE_INSENSITIVE", DEFAULT_STATSP_CASE_INSENSITIVE);
  this->vec[VAR_STATSP_MESSAGE] =
    new StrSetting("STATSP_MESSAGE", DEFAULT_STATSP_MESSAGE);
  this->vec[VAR_STATSP_REPLY] =
    new BoolSetting("STATSP_REPLY", DEFAULT_STATSP_REPLY);
  this->vec[VAR_STATSP_SHOW_IDLE] =
    new BoolSetting("STATSP_SHOW_IDLE", DEFAULT_STATSP_SHOW_IDLE);
  this->vec[VAR_STATSP_SHOW_USERHOST] =
    new BoolSetting("STATSP_SHOW_USERHOST", DEFAULT_STATSP_SHOW_USERHOST);
  this->vec[VAR_TOOMANY_ACTION] =
    new ActionSetting("TOOMANY_ACTION", DEFAULT_TOOMANY_ACTION,
    DEFAULT_TOOMANY_ACTION_TIME);
  this->vec[VAR_TOOMANY_IGNORE_USERNAME] =
    new BoolSetting("TOOMANY_IGNORE_USERNAME", DEFAULT_TOOMANY_IGNORE_USERNAME);
  this->vec[VAR_TOOMANY_MAX_COUNT] =
    new IntSetting("TOOMANY_MAX_COUNT", DEFAULT_TOOMANY_MAX_COUNT, 1);
  this->vec[VAR_TOOMANY_MAX_TIME] =
    new IntSetting("TOOMANY_MAX_TIME", DEFAULT_TOOMANY_MAX_TIME, 1);
  this->vec[VAR_TOOMANY_REASON] =
    new StrSetting("TOOMANY_REASON", DEFAULT_TOOMANY_REASON);
  this->vec[VAR_TRACE_FLOOD_ACTION] =
    new ActionSetting("TRACE_FLOOD_ACTION", DEFAULT_TRACE_FLOOD_ACTION);
  this->vec[VAR_TRACE_FLOOD_MAX_COUNT] =
    new IntSetting("TRACE_FLOOD_MAX_COUNT", DEFAULT_TRACE_FLOOD_MAX_COUNT, 1);
  this->vec[VAR_TRACE_FLOOD_MAX_TIME] =
    new IntSetting("TRACE_FLOOD_MAX_TIME", DEFAULT_TRACE_FLOOD_MAX_TIME, 1);
  this->vec[VAR_TRACE_FLOOD_REASON] =
    new StrSetting("TRACE_FLOOD_REASON", DEFAULT_TRACE_FLOOD_REASON);
  this->vec[VAR_TRACK_TEMP_DLINES] =
    new BoolSetting("TRACK_TEMP_DLINES", DEFAULT_TRACK_TEMP_DLINES);
  this->vec[VAR_TRACK_TEMP_KLINES] =
    new BoolSetting("TRACK_TEMP_KLINES", DEFAULT_TRACK_TEMP_KLINES);
  this->vec[VAR_TRAP_CONNECTS] =
    new BoolSetting("TRAP_CONNECTS", DEFAULT_TRAP_CONNECTS);
  this->vec[VAR_TRAP_NICK_CHANGES] =
    new BoolSetting("TRAP_NICK_CHANGES", DEFAULT_TRAP_NICK_CHANGES);
  this->vec[VAR_UMODE] = new UmodeSetting("UMODE", DEFAULT_UMODE);
  this->vec[VAR_UNAUTHED_MAY_CHAT] =
    new BoolSetting("UNAUTHED_MAY_CHAT", DEFAULT_UNAUTHED_MAY_CHAT);
  this->vec[VAR_USER_COUNT_DELTA_MAX] =
    new IntSetting("USER_COUNT_DELTA_MAX", DEFAULT_USER_COUNT_DELTA_MAX, 1);
  this->vec[VAR_WATCH_FLOODER_NOTICES] =
    new BoolSetting("WATCH_FLOODER_NOTICES", DEFAULT_WATCH_FLOODER_NOTICES);
  this->vec[VAR_WATCH_INFO_NOTICES] =
    new BoolSetting("WATCH_INFO_NOTICES", DEFAULT_WATCH_INFO_NOTICES);
  this->vec[VAR_WATCH_JUPE_NOTICES] =
    new BoolSetting("WATCH_JUPE_NOTICES", DEFAULT_WATCH_JUPE_NOTICES);
  this->vec[VAR_WATCH_LINKS_NOTICES] =
    new BoolSetting("WATCH_LINKS_NOTICES", DEFAULT_WATCH_LINKS_NOTICES);
  this->vec[VAR_WATCH_MOTD_NOTICES] =
    new BoolSetting("WATCH_MOTD_NOTICES", DEFAULT_WATCH_MOTD_NOTICES);
  this->vec[VAR_WATCH_SPAMBOT_NOTICES] =
    new BoolSetting("WATCH_SPAMBOT_NOTICES", DEFAULT_WATCH_SPAMBOT_NOTICES);
  this->vec[VAR_WATCH_STATS_NOTICES] =
    new BoolSetting("WATCH_STATS_NOTICES", DEFAULT_WATCH_STATS_NOTICES);
  this->vec[VAR_WATCH_TOOMANY_NOTICES] =
    new BoolSetting("WATCH_TOOMANY_NOTICES", DEFAULT_WATCH_TOOMANY_NOTICES);
  this->vec[VAR_WATCH_TRACE_NOTICES] =
    new BoolSetting("WATCH_TRACE_NOTICES", DEFAULT_WATCH_TRACE_NOTICES);
  this->vec[VAR_XO_SERVICES_ENABLE] =
    new BoolSetting("XO_SERVICES_ENABLE", DEFAULT_XO_SERVICES_ENABLE);
  this->vec[VAR_XO_SERVICES_REQUEST] =
    new StrSetting("XO_SERVICES_REQUEST", DEFAULT_XO_SERVICES_REQUEST);
  this->vec[VAR_XO_SERVICES_RESPONSE] =
    new StrSetting("XO_SERVICES_RESPONSE", DEFAULT_XO_SERVICES_RESPONSE);
}


Vars::~Vars()
{
  for (VarVector::const_iterator pos = this->vec.begin();
    pos != this->vec.end(); ++pos)
  {
    delete *pos;
  }
}


int
Vars::findVar(const std::string & name) const
{
  int result = -1;

  for (int i = 0; i < VAR_COUNT; ++i)
  {
    if (Same(this->vec[i]->getName(), name))
    {
      // Exact match!
      result = i;
      break;
    }
    else if (Same(this->vec[i]->getName(), name, name.length()))
    {
      // Partial match
      if (result < 0)
      {
	result = i;
      }
      else
      {
	result = VAR_COUNT;
	break;
      }
    }
  }

  return result;
}


std::string
Vars::set(const std::string & name, const std::string & value,
  const std::string & handle)
{
  int idx = findVar(name);

  if (idx < 0)
  {
    return "*** No such variable: " + name;
  }
  else if (idx >= VAR_COUNT)
  {
    return "*** Ambiguous variable name: " + name;
  }
  else
  {
    std::string error = this->vec[idx]->set(value);
    if ((error.length() == 0) && (handle.length() != 0))
    {
      std::string newValue = this->vec[idx]->get();
      std::string notice;

      if (newValue.length() > 0)
      {
        notice = "*** " + handle + " set " + this->vec[idx]->getName() +
          " to " + newValue;
      }
      else
      {
        notice = "*** " + handle + " cleared " + this->vec[idx]->getName();
      }
      Log::Write(notice);
      ::SendAll(notice, UF_OPER);
      
      if (vars[VAR_AUTO_SAVE]->getBool())
      {
        Config::saveSettings();
      }
    }
    return error;
  }
}


int
Vars::get(StrList & output, const std::string & name) const
{
  int count = 0;

  for (int i = 0; i < VAR_COUNT; ++i)
  {
    if (Same(this->vec[i]->getName(), name, name.length()))
    {
      std::string value = this->vec[i]->get();
      if (value.length() > 0)
      {
        output.push_back("*** " + this->vec[i]->getName() + " = " + value);
      }
      else
      {
        output.push_back("*** " + this->vec[i]->getName() + " is empty.");
      }

      ++count;
    }
  }

  return count;
}


void
Vars::save(std::ofstream & file) const
{
  for (int i = 0; i < VAR_COUNT; ++i)
  {
    if (this->vec[i]->hasChanged())
    {
      file << "SET " << this->vec[i]->getName() << " " <<
        this->vec[i]->get() << std::endl;
    }
  }
}

