#ifndef __VARS_H__
#define __VARS_H__
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

// ===========================================================================
// File Description:
//
//  OOMon is now highly configurable at runtime.  In older versions, OOMon
//  had to be configured at compile-time, which was tedious and sometimes
//  difficult to do correctly.  Now, it's possible to view or modify any of
//  OOMon's settings with the ".set" command.
//
//  The Vars class' interface should be quiet straightforward.  Reading
//  a single setting should be done using the [] operator.  For example:
//    vars[VAR_SOMETHING]->getInt()
//  Refer to setting.h for the interface to each individual setting.
//
//  Other public member functions include:
//    findVar(string)
//      Search for a setting with a symbol name matching the string.  If
//      one setting matches, return the index to that setting.  If no
//      settings match, return -1.  If two or more settings match, return
//      VAR_COUNT.
//    set(string, string, string)
//      The ".set" command uses this function to modify settings.  It
//      returns a string that should be reported back to the user.
//    get(client, string)
//      The ".set" command uses this function to view settings.  It returns
//      the number of settings matching the string.
//    save(ofstream)
//      Write all modified settings to a file.
// ===========================================================================

// Std C++ Headers
#include <fstream>
#include <string>
#include <vector>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>

// OOMon Headers
#include "setting.h"


enum VarNum
{
  VAR_AUTO_KLINE_HOST,
  VAR_AUTO_KLINE_HOST_REASON,
  VAR_AUTO_KLINE_HOST_TIME,
  VAR_AUTO_KLINE_NET,
  VAR_AUTO_KLINE_NET_REASON,
  VAR_AUTO_KLINE_NET_TIME,
  VAR_AUTO_KLINE_NOIDENT,
  VAR_AUTO_KLINE_NOIDENT_REASON,
  VAR_AUTO_KLINE_NOIDENT_TIME,
  VAR_AUTO_KLINE_USERHOST,
  VAR_AUTO_KLINE_USERHOST_REASON,
  VAR_AUTO_KLINE_USERHOST_TIME,
  VAR_AUTO_KLINE_USERNET,
  VAR_AUTO_KLINE_USERNET_REASON,
  VAR_AUTO_KLINE_USERNET_TIME,
  VAR_AUTO_PILOT,
  VAR_AUTO_SAVE,
  VAR_BROKEN_HOSTNAME_MUNGING,
  VAR_CHECK_FOR_SPOOFS,
  VAR_CLONE_MAX_TIME,
  VAR_CLONE_MIN_COUNT,
  VAR_CLONE_REPORT_FORMAT,
  VAR_CLONE_REPORT_INTERVAL,
  VAR_CONNECT_FLOOD_ACTION,
  VAR_CONNECT_FLOOD_MAX_COUNT,
  VAR_CONNECT_FLOOD_MAX_TIME,
  VAR_CONNECT_FLOOD_REASON,
  VAR_CTCPVERSION_ENABLE,
  VAR_CTCPVERSION_TIMEOUT,
  VAR_CTCPVERSION_TIMEOUT_ACTION,
  VAR_CTCPVERSION_TIMEOUT_REASON,
  VAR_DEFAULT_DLINE_TIMEOUT,
  VAR_DEFAULT_KLINE_TIMEOUT,
  VAR_DNSBL_PROXY_ACTION,
  VAR_DNSBL_PROXY_ENABLE,
  VAR_DNSBL_PROXY_REASON,
  VAR_DNSBL_PROXY_ZONE,
  VAR_EXTRA_KLINE_INFO,
  VAR_FAKE_IP_SPOOF_ACTION,
  VAR_FAKE_IP_SPOOF_REASON,
  VAR_FLOODER_ACTION,
  VAR_FLOODER_MAX_COUNT,
  VAR_FLOODER_MAX_TIME,
  VAR_FLOODER_REASON,
  VAR_IGNORE_UNKNOWN_COMMAND,
  VAR_ILLEGAL_CHAR_SPOOF_ACTION,
  VAR_ILLEGAL_CHAR_SPOOF_REASON,
  VAR_ILLEGAL_TLD_SPOOF_ACTION,
  VAR_ILLEGAL_TLD_SPOOF_REASON,
  VAR_INFO_FLOOD_ACTION,
  VAR_INFO_FLOOD_MAX_COUNT,
  VAR_INFO_FLOOD_MAX_TIME,
  VAR_INFO_FLOOD_REASON,
  VAR_INVALID_USERNAME_ACTION,
  VAR_INVALID_USERNAME_REASON,
  VAR_JUPE_JOIN_ACTION,
  VAR_JUPE_JOIN_IGNORE_CHANNEL,
  VAR_JUPE_JOIN_MAX_COUNT,
  VAR_JUPE_JOIN_MAX_TIME,
  VAR_JUPE_JOIN_REASON,
  VAR_KILLLIST_REASON,
  VAR_KILLNFIND_REASON,
  VAR_LINKS_FLOOD_ACTION,
  VAR_LINKS_FLOOD_MAX_COUNT,
  VAR_LINKS_FLOOD_MAX_TIME,
  VAR_LINKS_FLOOD_REASON,
  VAR_MOTD_FLOOD_ACTION,
  VAR_MOTD_FLOOD_MAX_COUNT,
  VAR_MOTD_FLOOD_MAX_TIME,
  VAR_MOTD_FLOOD_REASON,
  VAR_MULTI_MIN,
  VAR_NICK_CHANGE_MAX_COUNT,
  VAR_NICK_CHANGE_T1_TIME,
  VAR_NICK_CHANGE_T2_TIME,
  VAR_NICK_FLOOD_ACTION,
  VAR_NICK_FLOOD_REASON,
  VAR_OPER_IN_MULTI,
  VAR_OPER_NICK_IN_REASON,
  VAR_OPER_ONLY_DCC,
  VAR_OPERFAIL_ACTION,
  VAR_OPERFAIL_MAX_COUNT,
  VAR_OPERFAIL_MAX_TIME,
  VAR_OPERFAIL_REASON,
  VAR_RELAY_MSGS_TO_LOCOPS,
  VAR_SCAN_FOR_PROXIES,
  VAR_SCAN_HTTP_CONNECT_PORTS,
  VAR_SCAN_PROXY_ACTION,
  VAR_SCAN_PROXY_REASON,
  VAR_SCAN_SOCKS4_PORTS,
  VAR_SCAN_SOCKS5_PORTS,
  VAR_SCAN_WINGATE_PORTS,
  VAR_SEEDRAND_ACTION,
  VAR_SEEDRAND_COMMAND_MIN,
  VAR_SEEDRAND_FORMAT,
  VAR_SEEDRAND_REASON,
  VAR_SEEDRAND_REPORT_MIN,
  VAR_SERVER_TIMEOUT,
  VAR_SERVICES_CHECK_INTERVAL,
  VAR_SERVICES_CLONE_LIMIT,
  VAR_SPAMBOT_ACTION,
  VAR_SPAMBOT_MAX_COUNT,
  VAR_SPAMBOT_MAX_TIME,
  VAR_SPAMBOT_REASON,
  VAR_SPAMTRAP_ACTION,
  VAR_SPAMTRAP_DEFAULT_REASON,
  VAR_SPAMTRAP_ENABLE,
  VAR_SPAMTRAP_MIN_SCORE,
  VAR_SPAMTRAP_NICK,
  VAR_SPAMTRAP_USERHOST,
  VAR_STATS_FLOOD_ACTION,
  VAR_STATS_FLOOD_MAX_COUNT,
  VAR_STATS_FLOOD_MAX_TIME,
  VAR_STATS_FLOOD_REASON,
  VAR_STATSP_CASE_INSENSITIVE,
  VAR_STATSP_MESSAGE,
  VAR_STATSP_REPLY,
  VAR_STATSP_SHOW_IDLE,
  VAR_STATSP_SHOW_USERHOST,
  VAR_TOOMANY_ACTION,
  VAR_TOOMANY_IGNORE_USERNAME,
  VAR_TOOMANY_MAX_COUNT,
  VAR_TOOMANY_MAX_TIME,
  VAR_TOOMANY_REASON,
  VAR_TRACE_FLOOD_ACTION,
  VAR_TRACE_FLOOD_MAX_COUNT,
  VAR_TRACE_FLOOD_MAX_TIME,
  VAR_TRACE_FLOOD_REASON,
  VAR_TRACK_PERM_DLINES,
  VAR_TRACK_PERM_KLINES,
  VAR_TRACK_TEMP_DLINES,
  VAR_TRACK_TEMP_KLINES,
  VAR_TRAP_CONNECTS,
  VAR_TRAP_CTCP_VERSIONS,
  VAR_TRAP_NICK_CHANGES,
  VAR_TRAP_NOTICES,
  VAR_TRAP_PRIVMSGS,
  VAR_UMODE,
  VAR_UNAUTHED_MAY_CHAT,
  VAR_USER_COUNT_DELTA_MAX,
  VAR_WATCH_FLOODER_NOTICES,
  VAR_WATCH_INFO_NOTICES,
  VAR_WATCH_JUPE_NOTICES,
  VAR_WATCH_LINKS_NOTICES,
  VAR_WATCH_MOTD_NOTICES,
  VAR_WATCH_OPERFAIL_NOTICES,
  VAR_WATCH_SPAMBOT_NOTICES,
  VAR_WATCH_STATS_NOTICES,
  VAR_WATCH_TOOMANY_NOTICES,
  VAR_WATCH_TRACE_NOTICES,
  VAR_XO_SERVICES_CLONE_ACTION,
  VAR_XO_SERVICES_CLONE_REASON,
  VAR_XO_SERVICES_ENABLE,
  VAR_XO_SERVICES_REQUEST,
  VAR_XO_SERVICES_RESPONSE,
  VAR_COUNT
};


class Vars
{
public:
  typedef boost::shared_ptr<Setting> SettingPtr;
  typedef std::vector<SettingPtr> VarVector;
  typedef VarVector::size_type size_type;
  typedef VarVector::const_reference const_reference;

  Vars(void);
  virtual ~Vars(void) { };

  int findVar(const std::string & name) const;
  std::string set(const std::string & name, const std::string & value,
    const std::string & handle = "");
  int get(class BotClient * client, const std::string & name) const;

  const_reference operator[](size_type n) const
  {
    return vec[n];
  };

  void save(std::ofstream & file) const;

private:
  VarVector vec;
};


extern Vars vars;


#endif /* __VARS_H__ */

