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
#include "config.h"
#include "botclient.h"


#ifdef DEBUG
# define VARS_DEBUG
#endif


Vars vars;


Vars::Vars(void) : vec(VAR_COUNT)
{
  this->vec[VAR_AUTO_KLINE_HOST] = SettingPtr(new BoolSetting("AUTO_KLINE_HOST",
    DEFAULT_AUTO_KLINE_HOST));
  this->vec[VAR_AUTO_KLINE_HOST_REASON] =
    SettingPtr(new StrSetting("AUTO_KLINE_HOST_REASON",
    DEFAULT_AUTO_KLINE_HOST_REASON));
  this->vec[VAR_AUTO_KLINE_HOST_TIME] =
    SettingPtr(new IntSetting("AUTO_KLINE_HOST_TIME",
    DEFAULT_AUTO_KLINE_HOST_TIME, 0));
  this->vec[VAR_AUTO_KLINE_NET] = SettingPtr(new BoolSetting("AUTO_KLINE_NET",
    DEFAULT_AUTO_KLINE_NET));
  this->vec[VAR_AUTO_KLINE_NET_REASON] =
    SettingPtr(new StrSetting("AUTO_KLINE_NET_REASON",
    DEFAULT_AUTO_KLINE_NET_REASON));
  this->vec[VAR_AUTO_KLINE_NET_TIME] =
    SettingPtr(new IntSetting("AUTO_KLINE_NET_TIME",
    DEFAULT_AUTO_KLINE_NET_TIME, 0));
  this->vec[VAR_AUTO_KLINE_NOIDENT] =
    SettingPtr(new BoolSetting("AUTO_KLINE_NOIDENT",
    DEFAULT_AUTO_KLINE_NOIDENT));
  this->vec[VAR_AUTO_KLINE_NOIDENT_REASON] =
    SettingPtr(new StrSetting("AUTO_KLINE_NOIDENT_REASON",
    DEFAULT_AUTO_KLINE_NOIDENT_REASON));
  this->vec[VAR_AUTO_KLINE_NOIDENT_TIME] =
    SettingPtr(new IntSetting("AUTO_KLINE_NOIDENT_TIME",
    DEFAULT_AUTO_KLINE_NOIDENT_TIME, 0));
  this->vec[VAR_AUTO_KLINE_USERHOST] =
    SettingPtr(new BoolSetting("AUTO_KLINE_USERHOST",
    DEFAULT_AUTO_KLINE_USERHOST));
  this->vec[VAR_AUTO_KLINE_USERHOST_REASON] =
    SettingPtr(new StrSetting("AUTO_KLINE_USERHOST_REASON",
    DEFAULT_AUTO_KLINE_USERHOST_REASON));
  this->vec[VAR_AUTO_KLINE_USERHOST_TIME] =
    SettingPtr(new IntSetting("AUTO_KLINE_USERHOST_TIME",
    DEFAULT_AUTO_KLINE_USERHOST_TIME, 0));
  this->vec[VAR_AUTO_KLINE_USERNET] =
    SettingPtr(new BoolSetting("AUTO_KLINE_USERNET",
    DEFAULT_AUTO_KLINE_USERNET));
  this->vec[VAR_AUTO_KLINE_USERNET_REASON] =
    SettingPtr(new StrSetting("AUTO_KLINE_USERNET_REASON",
    DEFAULT_AUTO_KLINE_USERNET_REASON));
  this->vec[VAR_AUTO_KLINE_USERNET_TIME] =
    SettingPtr(new IntSetting("AUTO_KLINE_USERNET_TIME",
    DEFAULT_AUTO_KLINE_USERNET_TIME, 0));
  this->vec[VAR_AUTO_PILOT] =
    SettingPtr(new BoolSetting("AUTO_PILOT", DEFAULT_AUTO_PILOT));
  this->vec[VAR_AUTO_SAVE] =
    SettingPtr(new BoolSetting("AUTO_SAVE", DEFAULT_AUTO_SAVE));
  this->vec[VAR_BROKEN_HOSTNAME_MUNGING] =
    SettingPtr(new BoolSetting("BROKEN_HOSTNAME_MUNGING",
    DEFAULT_BROKEN_HOSTNAME_MUNGING));
  this->vec[VAR_CHECK_FOR_SPOOFS] =
    SettingPtr(new BoolSetting("CHECK_FOR_SPOOFS", DEFAULT_CHECK_FOR_SPOOFS));
  this->vec[VAR_CLONE_MAX_TIME] = SettingPtr(new IntSetting("CLONE_MAX_TIME",
    DEFAULT_CLONE_MAX_TIME, 1));
  this->vec[VAR_CLONE_MIN_COUNT] = SettingPtr(new IntSetting("CLONE_MIN_COUNT",
    DEFAULT_CLONE_MIN_COUNT, 2));
  this->vec[VAR_CLONE_REPORT_FORMAT] =
    SettingPtr(new StrSetting("CLONE_REPORT_FORMAT",
    DEFAULT_CLONE_REPORT_FORMAT));
  this->vec[VAR_CLONE_REPORT_INTERVAL] =
    SettingPtr(new IntSetting("CLONE_REPORT_INTERVAL",
    DEFAULT_CLONE_REPORT_INTERVAL, 1));
  this->vec[VAR_CONNECT_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("CONNECT_FLOOD_ACTION",
    DEFAULT_CONNECT_FLOOD_ACTION, DEFAULT_CONNECT_FLOOD_ACTION_TIME));
  this->vec[VAR_CONNECT_FLOOD_MAX_COUNT] =
    SettingPtr(new IntSetting("CONNECT_FLOOD_MAX_COUNT",
    DEFAULT_CONNECT_FLOOD_MAX_COUNT, 0));
  this->vec[VAR_CONNECT_FLOOD_MAX_TIME] =
    SettingPtr(new IntSetting("CONNECT_FLOOD_MAX_TIME",
    DEFAULT_CONNECT_FLOOD_MAX_TIME, 1));
  this->vec[VAR_CONNECT_FLOOD_REASON] =
    SettingPtr(new StrSetting("CONNECT_FLOOD_REASON",
    DEFAULT_CONNECT_FLOOD_REASON));
  this->vec[VAR_CTCPVERSION_ENABLE] =
    SettingPtr(new BoolSetting("CTCPVERSION_ENABLE",
    DEFAULT_CTCPVERSION_ENABLE));
  this->vec[VAR_CTCPVERSION_TIMEOUT] =
    SettingPtr(new IntSetting("CTCPVERSION_TIMEOUT",
    DEFAULT_CTCPVERSION_TIMEOUT, 0));
  this->vec[VAR_CTCPVERSION_TIMEOUT_ACTION] =
    SettingPtr(new ActionSetting("CTCPVERSION_TIMEOUT_ACTION",
    DEFAULT_CTCPVERSION_TIMEOUT_ACTION,
    DEFAULT_CTCPVERSION_TIMEOUT_ACTION_TIME));
  this->vec[VAR_CTCPVERSION_TIMEOUT_REASON] =
    SettingPtr(new StrSetting("CTCPVERSION_TIMEOUT_REASON",
    DEFAULT_CTCPVERSION_TIMEOUT_REASON));
  this->vec[VAR_DEFAULT_DLINE_TIMEOUT] =
    SettingPtr(new IntSetting("DEFAULT_DLINE_TIMEOUT",
    DEFAULT_DEFAULT_DLINE_TIMEOUT, 0));
  this->vec[VAR_DEFAULT_KLINE_TIMEOUT] =
    SettingPtr(new IntSetting("DEFAULT_KLINE_TIMEOUT",
    DEFAULT_DEFAULT_KLINE_TIMEOUT, 0));
  this->vec[VAR_DNSBL_PROXY_ACTION] =
    SettingPtr(new ActionSetting("DNSBL_PROXY_ACTION",
    DEFAULT_DNSBL_PROXY_ACTION));
  this->vec[VAR_DNSBL_PROXY_ENABLE] =
    SettingPtr(new BoolSetting("DNSBL_PROXY_ENABLE",
    DEFAULT_DNSBL_PROXY_ENABLE));
  this->vec[VAR_DNSBL_PROXY_REASON] =
    SettingPtr(new StrSetting("DNSBL_PROXY_REASON",
    DEFAULT_DNSBL_PROXY_REASON));
  this->vec[VAR_DNSBL_PROXY_ZONE] =
    SettingPtr(new StrSetting("DNSBL_PROXY_ZONE", DEFAULT_DNSBL_PROXY_ZONE));
  this->vec[VAR_EXTRA_KLINE_INFO] =
    SettingPtr(new BoolSetting("EXTRA_KLINE_INFO", DEFAULT_EXTRA_KLINE_INFO));
  this->vec[VAR_FAKE_IP_SPOOF_ACTION] =
    SettingPtr(new ActionSetting("FAKE_IP_SPOOF_ACTION",
    DEFAULT_FAKE_IP_SPOOF_ACTION));
  this->vec[VAR_FAKE_IP_SPOOF_REASON] =
    SettingPtr(new StrSetting("FAKE_IP_SPOOF_REASON",
    DEFAULT_FAKE_IP_SPOOF_REASON));
  this->vec[VAR_FLOODER_ACTION] =
    SettingPtr(new ActionSetting("FLOODER_ACTION", DEFAULT_FLOODER_ACTION));
  this->vec[VAR_FLOODER_MAX_COUNT] =
    SettingPtr(new IntSetting("FLOODER_MAX_COUNT", DEFAULT_FLOODER_MAX_COUNT,
    0));
  this->vec[VAR_FLOODER_MAX_TIME] =
    SettingPtr(new IntSetting("FLOODER_MAX_TIME", DEFAULT_FLOODER_MAX_TIME, 1));
  this->vec[VAR_FLOODER_REASON] =
    SettingPtr(new StrSetting("FLOODER_REASON", DEFAULT_FLOODER_REASON));
  this->vec[VAR_GLIST_FORMAT] =
    SettingPtr(new StrSetting("GLIST_FORMAT", DEFAULT_GLIST_FORMAT));
  this->vec[VAR_IGNORE_UNKNOWN_COMMAND] =
    SettingPtr(new BoolSetting("IGNORE_UNKNOWN_COMMAND",
    DEFAULT_IGNORE_UNKNOWN_COMMAND));
  this->vec[VAR_ILLEGAL_CHAR_SPOOF_ACTION] =
    SettingPtr(new ActionSetting("ILLEGAL_CHAR_SPOOF_ACTION",
    DEFAULT_ILLEGAL_CHAR_SPOOF_ACTION));
  this->vec[VAR_ILLEGAL_CHAR_SPOOF_REASON] =
    SettingPtr(new StrSetting("ILLEGAL_CHAR_SPOOF_REASON",
    DEFAULT_ILLEGAL_CHAR_SPOOF_REASON));
  this->vec[VAR_ILLEGAL_TLD_SPOOF_ACTION] =
    SettingPtr(new ActionSetting("ILLEGAL_TLD_SPOOF_ACTION",
    DEFAULT_ILLEGAL_TLD_SPOOF_ACTION));
  this->vec[VAR_ILLEGAL_TLD_SPOOF_REASON] =
    SettingPtr(new StrSetting("ILLEGAL_TLD_SPOOF_REASON",
    DEFAULT_ILLEGAL_TLD_SPOOF_REASON));
  this->vec[VAR_INFO_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("INFO_FLOOD_ACTION",
    DEFAULT_INFO_FLOOD_ACTION));
  this->vec[VAR_INFO_FLOOD_MAX_COUNT] =
    SettingPtr(new IntSetting("INFO_FLOOD_MAX_COUNT",
    DEFAULT_INFO_FLOOD_MAX_COUNT, 1));
  this->vec[VAR_INFO_FLOOD_MAX_TIME] =
    SettingPtr(new IntSetting("INFO_FLOOD_MAX_TIME",
    DEFAULT_INFO_FLOOD_MAX_TIME, 1));
  this->vec[VAR_INFO_FLOOD_REASON] =
    SettingPtr(new StrSetting("INFO_FLOOD_REASON", DEFAULT_INFO_FLOOD_REASON));
  this->vec[VAR_INVALID_USERNAME_ACTION] =
    SettingPtr(new ActionSetting("INVALID_USERNAME_ACTION",
    DEFAULT_INVALID_USERNAME_ACTION));
  this->vec[VAR_INVALID_USERNAME_REASON] =
    SettingPtr(new StrSetting("INVALID_USERNAME_REASON",
    DEFAULT_INVALID_USERNAME_REASON));
  this->vec[VAR_JUPE_JOIN_ACTION] =
    SettingPtr(new ActionSetting("JUPE_JOIN_ACTION", DEFAULT_JUPE_JOIN_ACTION,
    DEFAULT_JUPE_JOIN_ACTION_TIME));
  this->vec[VAR_JUPE_JOIN_IGNORE_CHANNEL] =
    SettingPtr(new BoolSetting("JUPE_JOIN_IGNORE_CHANNEL",
    DEFAULT_JUPE_JOIN_IGNORE_CHANNEL));
  this->vec[VAR_JUPE_JOIN_MAX_COUNT] =
    SettingPtr(new IntSetting("JUPE_JOIN_MAX_COUNT",
    DEFAULT_JUPE_JOIN_MAX_COUNT, 0));
  this->vec[VAR_JUPE_JOIN_MAX_TIME] =
    SettingPtr(new IntSetting("JUPE_JOIN_MAX_TIME", DEFAULT_JUPE_JOIN_MAX_TIME,
    0));
  this->vec[VAR_JUPE_JOIN_REASON] =
    SettingPtr(new StrSetting("JUPE_JOIN_REASON", DEFAULT_JUPE_JOIN_REASON));
  this->vec[VAR_KILLLIST_REASON] =
    SettingPtr(new StrSetting("KILLLIST_REASON", DEFAULT_KILLLIST_REASON));
  this->vec[VAR_KILLNFIND_REASON] =
    SettingPtr(new StrSetting("KILLNFIND_REASON", DEFAULT_KILLNFIND_REASON));
  this->vec[VAR_LINKS_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("LINKS_FLOOD_ACTION",
    DEFAULT_LINKS_FLOOD_ACTION));
  this->vec[VAR_LINKS_FLOOD_MAX_COUNT] =
    SettingPtr(new IntSetting("LINKS_FLOOD_MAX_COUNT",
    DEFAULT_LINKS_FLOOD_MAX_COUNT, 1));
  this->vec[VAR_LINKS_FLOOD_MAX_TIME] =
    SettingPtr(new IntSetting("LINKS_FLOOD_MAX_TIME",
    DEFAULT_LINKS_FLOOD_MAX_TIME, 1));
  this->vec[VAR_LINKS_FLOOD_REASON] =
    SettingPtr(new StrSetting("LINKS_FLOOD_REASON",
    DEFAULT_LINKS_FLOOD_REASON));
  this->vec[VAR_LIST_FORMAT] =
    SettingPtr(new StrSetting("LIST_FORMAT", DEFAULT_LIST_FORMAT));
  this->vec[VAR_MOTD_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("MOTD_FLOOD_ACTION",
    DEFAULT_MOTD_FLOOD_ACTION));
  this->vec[VAR_MOTD_FLOOD_MAX_COUNT] =
    SettingPtr(new IntSetting("MOTD_FLOOD_MAX_COUNT",
    DEFAULT_MOTD_FLOOD_MAX_COUNT, 1));
  this->vec[VAR_MOTD_FLOOD_MAX_TIME] =
    SettingPtr(new IntSetting("MOTD_FLOOD_MAX_TIME",
    DEFAULT_MOTD_FLOOD_MAX_TIME, 1));
  this->vec[VAR_MOTD_FLOOD_REASON] =
    SettingPtr(new StrSetting("MOTD_FLOOD_REASON", DEFAULT_MOTD_FLOOD_REASON));
  this->vec[VAR_MULTI_MIN] = SettingPtr(new IntSetting("MULTI_MIN",
    DEFAULT_MULTI_MIN, 1));
  this->vec[VAR_NFIND_FORMAT] =
    SettingPtr(new StrSetting("NFIND_FORMAT", DEFAULT_NFIND_FORMAT));
  this->vec[VAR_NICK_CHANGE_MAX_COUNT] =
    SettingPtr(new IntSetting("NICK_CHANGE_MAX_COUNT",
    DEFAULT_NICK_CHANGE_MAX_COUNT, 1));
  this->vec[VAR_NICK_CHANGE_T1_TIME] =
    SettingPtr(new IntSetting("NICK_CHANGE_T1_TIME",
    DEFAULT_NICK_CHANGE_T1_TIME, 1));
  this->vec[VAR_NICK_CHANGE_T2_TIME] =
    SettingPtr(new IntSetting("NICK_CHANGE_T2_TIME",
    DEFAULT_NICK_CHANGE_T2_TIME, 1));
  this->vec[VAR_NICK_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("NICK_FLOOD_ACTION",
    DEFAULT_NICK_FLOOD_ACTION));
  this->vec[VAR_NICK_FLOOD_REASON] =
    SettingPtr(new StrSetting("NICK_FLOOD_REASON", DEFAULT_NICK_FLOOD_REASON));
  this->vec[VAR_OPER_IN_MULTI] =
    SettingPtr(new BoolSetting("OPER_IN_MULTI", DEFAULT_OPER_IN_MULTI));
  this->vec[VAR_OPER_NICK_IN_REASON] =
    SettingPtr(new BoolSetting("OPER_NICK_IN_REASON",
    DEFAULT_OPER_NICK_IN_REASON));
  this->vec[VAR_OPER_ONLY_DCC] =
    SettingPtr(new BoolSetting("OPER_ONLY_DCC", DEFAULT_OPER_ONLY_DCC));
  this->vec[VAR_OPERFAIL_ACTION] =
    SettingPtr(new ActionSetting("OPERFAIL_ACTION", DEFAULT_OPERFAIL_ACTION));
  this->vec[VAR_OPERFAIL_MAX_COUNT] =
    SettingPtr(new IntSetting("OPERFAIL_MAX_COUNT", DEFAULT_OPERFAIL_MAX_COUNT,
    0));
  this->vec[VAR_OPERFAIL_MAX_TIME] =
    SettingPtr(new IntSetting("OPERFAIL_MAX_TIME", DEFAULT_OPERFAIL_MAX_TIME,
    1));
  this->vec[VAR_OPERFAIL_REASON] =
    SettingPtr(new StrSetting("OPERFAIL_REASON", DEFAULT_OPERFAIL_REASON));
  this->vec[VAR_RELAY_MSGS_TO_LOCOPS] =
    SettingPtr(new BoolSetting("RELAY_MSGS_TO_LOCOPS",
    DEFAULT_RELAY_MSGS_TO_LOCOPS));
  this->vec[VAR_SCAN_FOR_PROXIES] =
    SettingPtr(new BoolSetting("SCAN_FOR_PROXIES", DEFAULT_SCAN_FOR_PROXIES));
  this->vec[VAR_SCAN_HTTP_CONNECT_PORTS] =
    SettingPtr(new StrSetting("SCAN_HTTP_CONNECT_PORTS",
    DEFAULT_SCAN_HTTP_CONNECT_PORTS));
  this->vec[VAR_SCAN_PROXY_ACTION] =
    SettingPtr(new ActionSetting("SCAN_PROXY_ACTION",
    DEFAULT_SCAN_PROXY_ACTION));
  this->vec[VAR_SCAN_PROXY_REASON] =
    SettingPtr(new StrSetting("SCAN_PROXY_REASON", DEFAULT_SCAN_PROXY_REASON));
  this->vec[VAR_SCAN_SOCKS4_PORTS] =
    SettingPtr(new StrSetting("SCAN_SOCKS4_PORTS", DEFAULT_SCAN_SOCKS4_PORTS));
  this->vec[VAR_SCAN_SOCKS5_PORTS] =
    SettingPtr(new StrSetting("SCAN_SOCKS5_PORTS", DEFAULT_SCAN_SOCKS5_PORTS));
  this->vec[VAR_SCAN_WINGATE_PORTS] =
    SettingPtr(new StrSetting("SCAN_WINGATE_PORTS",
    DEFAULT_SCAN_WINGATE_PORTS));
  this->vec[VAR_SEEDRAND_ACTION] =
    SettingPtr(new ActionSetting("SEEDRAND_ACTION", DEFAULT_SEEDRAND_ACTION));
  this->vec[VAR_SEEDRAND_COMMAND_MIN] =
    SettingPtr(new IntSetting("SEEDRAND_COMMAND_MIN",
    DEFAULT_SEEDRAND_COMMAND_MIN));
  this->vec[VAR_SEEDRAND_FORMAT] =
    SettingPtr(new StrSetting("SEEDRAND_FORMAT", DEFAULT_SEEDRAND_FORMAT));
  this->vec[VAR_SEEDRAND_REASON] =
    SettingPtr(new StrSetting("SEEDRAND_REASON", DEFAULT_SEEDRAND_REASON));
  this->vec[VAR_SEEDRAND_REPORT_MIN] =
    SettingPtr(new IntSetting("SEEDRAND_REPORT_MIN",
    DEFAULT_SEEDRAND_REPORT_MIN));
  this->vec[VAR_SERVER_TIMEOUT] =
    SettingPtr(new ServerTimeoutSetting("SERVER_TIMEOUT",
    DEFAULT_SERVER_TIMEOUT));
  this->vec[VAR_SERVICES_CHECK_INTERVAL] =
    SettingPtr(new IntSetting("SERVICES_CHECK_INTERVAL",
    DEFAULT_SERVICES_CHECK_INTERVAL, 1));
  this->vec[VAR_SERVICES_CLONE_LIMIT] =
    SettingPtr(new IntSetting("SERVICES_CLONE_LIMIT",
    DEFAULT_SERVICES_CLONE_LIMIT, 2));
  this->vec[VAR_SPAMBOT_ACTION] =
    SettingPtr(new ActionSetting("SPAMBOT_ACTION", DEFAULT_SPAMBOT_ACTION));
  this->vec[VAR_SPAMBOT_MAX_COUNT] =
    SettingPtr(new IntSetting("SPAMBOT_MAX_COUNT", DEFAULT_SPAMBOT_MAX_COUNT,
    0));
  this->vec[VAR_SPAMBOT_MAX_TIME] =
    SettingPtr(new IntSetting("SPAMBOT_MAX_TIME", DEFAULT_SPAMBOT_MAX_TIME, 1));
  this->vec[VAR_SPAMBOT_REASON] =
    SettingPtr(new StrSetting("SPAMBOT_REASON", DEFAULT_SPAMBOT_REASON));
  this->vec[VAR_SPAMTRAP_ACTION] =
    SettingPtr(new ActionSetting("SPAMTRAP_ACTION", DEFAULT_SPAMTRAP_ACTION));
  this->vec[VAR_SPAMTRAP_DEFAULT_REASON] =
    SettingPtr(new StrSetting("SPAMTRAP_DEFAULT_REASON",
    DEFAULT_SPAMTRAP_DEFAULT_REASON));
  this->vec[VAR_SPAMTRAP_ENABLE] =
    SettingPtr(new BoolSetting("SPAMTRAP_ENABLE", DEFAULT_SPAMTRAP_ENABLE));
  this->vec[VAR_SPAMTRAP_MIN_SCORE] =
    SettingPtr(new IntSetting("SPAMTRAP_MIN_SCORE", DEFAULT_SPAMTRAP_MIN_SCORE,
    0));
  this->vec[VAR_SPAMTRAP_NICK] =
    SettingPtr(new StrSetting("SPAMTRAP_NICK", DEFAULT_SPAMTRAP_NICK));
  this->vec[VAR_SPAMTRAP_USERHOST] =
    SettingPtr(new StrSetting("SPAMTRAP_USERHOST", DEFAULT_SPAMTRAP_USERHOST));
  this->vec[VAR_STATS_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("STATS_FLOOD_ACTION",
    DEFAULT_STATS_FLOOD_ACTION));
  this->vec[VAR_STATS_FLOOD_MAX_COUNT] =
    SettingPtr(new IntSetting("STATS_FLOOD_MAX_COUNT",
    DEFAULT_STATS_FLOOD_MAX_COUNT, 1));
  this->vec[VAR_STATS_FLOOD_MAX_TIME] =
    SettingPtr(new IntSetting("STATS_FLOOD_MAX_TIME",
    DEFAULT_STATS_FLOOD_MAX_TIME, 1));
  this->vec[VAR_STATS_FLOOD_REASON] =
    SettingPtr(new StrSetting("STATS_FLOOD_REASON",
    DEFAULT_STATS_FLOOD_REASON));
  this->vec[VAR_STATSP_CASE_INSENSITIVE] =
    SettingPtr(new BoolSetting("STATSP_CASE_INSENSITIVE",
    DEFAULT_STATSP_CASE_INSENSITIVE));
  this->vec[VAR_STATSP_MESSAGE] =
    SettingPtr(new StrSetting("STATSP_MESSAGE", DEFAULT_STATSP_MESSAGE));
  this->vec[VAR_STATSP_REPLY] =
    SettingPtr(new BoolSetting("STATSP_REPLY", DEFAULT_STATSP_REPLY));
  this->vec[VAR_STATSP_SHOW_IDLE] =
    SettingPtr(new BoolSetting("STATSP_SHOW_IDLE", DEFAULT_STATSP_SHOW_IDLE));
  this->vec[VAR_STATSP_SHOW_USERHOST] =
    SettingPtr(new BoolSetting("STATSP_SHOW_USERHOST",
    DEFAULT_STATSP_SHOW_USERHOST));
  this->vec[VAR_TOOMANY_ACTION] =
    SettingPtr(new ActionSetting("TOOMANY_ACTION", DEFAULT_TOOMANY_ACTION,
    DEFAULT_TOOMANY_ACTION_TIME));
  this->vec[VAR_TOOMANY_IGNORE_USERNAME] =
    SettingPtr(new BoolSetting("TOOMANY_IGNORE_USERNAME",
    DEFAULT_TOOMANY_IGNORE_USERNAME));
  this->vec[VAR_TOOMANY_MAX_COUNT] =
    SettingPtr(new IntSetting("TOOMANY_MAX_COUNT", DEFAULT_TOOMANY_MAX_COUNT,
    1));
  this->vec[VAR_TOOMANY_MAX_TIME] =
    SettingPtr(new IntSetting("TOOMANY_MAX_TIME", DEFAULT_TOOMANY_MAX_TIME, 1));
  this->vec[VAR_TOOMANY_REASON] =
    SettingPtr(new StrSetting("TOOMANY_REASON", DEFAULT_TOOMANY_REASON));
  this->vec[VAR_TRACE_FLOOD_ACTION] =
    SettingPtr(new ActionSetting("TRACE_FLOOD_ACTION",
    DEFAULT_TRACE_FLOOD_ACTION));
  this->vec[VAR_TRACE_FLOOD_MAX_COUNT] =
    SettingPtr(new IntSetting("TRACE_FLOOD_MAX_COUNT",
    DEFAULT_TRACE_FLOOD_MAX_COUNT, 1));
  this->vec[VAR_TRACE_FLOOD_MAX_TIME] =
    SettingPtr(new IntSetting("TRACE_FLOOD_MAX_TIME",
    DEFAULT_TRACE_FLOOD_MAX_TIME, 1));
  this->vec[VAR_TRACE_FLOOD_REASON] =
    SettingPtr(new StrSetting("TRACE_FLOOD_REASON",
    DEFAULT_TRACE_FLOOD_REASON));
  this->vec[VAR_TRACK_PERM_DLINES] =
    SettingPtr(new BoolSetting("TRACK_PERM_DLINES", DEFAULT_TRACK_PERM_DLINES));
  this->vec[VAR_TRACK_PERM_KLINES] =
    SettingPtr(new BoolSetting("TRACK_PERM_KLINES", DEFAULT_TRACK_PERM_KLINES));
  this->vec[VAR_TRACK_TEMP_DLINES] =
    SettingPtr(new BoolSetting("TRACK_TEMP_DLINES", DEFAULT_TRACK_TEMP_DLINES));
  this->vec[VAR_TRACK_TEMP_KLINES] =
    SettingPtr(new BoolSetting("TRACK_TEMP_KLINES", DEFAULT_TRACK_TEMP_KLINES));
  this->vec[VAR_TRAP_CONNECTS] =
    SettingPtr(new BoolSetting("TRAP_CONNECTS", DEFAULT_TRAP_CONNECTS));
  this->vec[VAR_TRAP_CTCP_VERSIONS] =
    SettingPtr(new BoolSetting("TRAP_CTCP_VERSIONS",
	DEFAULT_TRAP_CTCP_VERSIONS));
  this->vec[VAR_TRAP_NICK_CHANGES] =
    SettingPtr(new BoolSetting("TRAP_NICK_CHANGES", DEFAULT_TRAP_NICK_CHANGES));
  this->vec[VAR_TRAP_NOTICES] =
    SettingPtr(new BoolSetting("TRAP_NOTICES", DEFAULT_TRAP_NOTICES));
  this->vec[VAR_TRAP_PRIVMSGS] =
    SettingPtr(new BoolSetting("TRAP_PRIVMSGS", DEFAULT_TRAP_PRIVMSGS));
  this->vec[VAR_UMODE] = SettingPtr(new UmodeSetting("UMODE", DEFAULT_UMODE));
  this->vec[VAR_UNAUTHED_MAY_CHAT] =
    SettingPtr(new BoolSetting("UNAUTHED_MAY_CHAT", DEFAULT_UNAUTHED_MAY_CHAT));
  this->vec[VAR_USER_COUNT_DELTA_MAX] =
    SettingPtr(new IntSetting("USER_COUNT_DELTA_MAX",
    DEFAULT_USER_COUNT_DELTA_MAX, 1));
  this->vec[VAR_WATCH_FLOODER_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_FLOODER_NOTICES",
    DEFAULT_WATCH_FLOODER_NOTICES));
  this->vec[VAR_WATCH_INFO_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_INFO_NOTICES",
    DEFAULT_WATCH_INFO_NOTICES));
  this->vec[VAR_WATCH_JUPE_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_JUPE_NOTICES",
    DEFAULT_WATCH_JUPE_NOTICES));
  this->vec[VAR_WATCH_LINKS_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_LINKS_NOTICES",
    DEFAULT_WATCH_LINKS_NOTICES));
  this->vec[VAR_WATCH_MOTD_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_MOTD_NOTICES",
    DEFAULT_WATCH_MOTD_NOTICES));
  this->vec[VAR_WATCH_OPERFAIL_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_OPERFAIL_NOTICES",
    DEFAULT_WATCH_OPERFAIL_NOTICES));
  this->vec[VAR_WATCH_SPAMBOT_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_SPAMBOT_NOTICES",
    DEFAULT_WATCH_SPAMBOT_NOTICES));
  this->vec[VAR_WATCH_STATS_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_STATS_NOTICES",
    DEFAULT_WATCH_STATS_NOTICES));
  this->vec[VAR_WATCH_TOOMANY_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_TOOMANY_NOTICES",
    DEFAULT_WATCH_TOOMANY_NOTICES));
  this->vec[VAR_WATCH_TRACE_NOTICES] =
    SettingPtr(new BoolSetting("WATCH_TRACE_NOTICES",
    DEFAULT_WATCH_TRACE_NOTICES));
  this->vec[VAR_XO_SERVICES_CLONE_ACTION] =
    SettingPtr(new ActionSetting("XO_SERVICES_CLONE_ACTION",
    DEFAULT_XO_SERVICES_CLONE_ACTION, DEFAULT_XO_SERVICES_CLONE_ACTION_TIME));
  this->vec[VAR_XO_SERVICES_CLONE_REASON] =
    SettingPtr(new StrSetting("XO_SERVICES_CLONE_REASON",
    DEFAULT_XO_SERVICES_CLONE_REASON));
  this->vec[VAR_XO_SERVICES_ENABLE] =
    SettingPtr(new BoolSetting("XO_SERVICES_ENABLE",
    DEFAULT_XO_SERVICES_ENABLE));
  this->vec[VAR_XO_SERVICES_REQUEST] =
    SettingPtr(new StrSetting("XO_SERVICES_REQUEST",
    DEFAULT_XO_SERVICES_REQUEST));
  this->vec[VAR_XO_SERVICES_RESPONSE] =
    SettingPtr(new StrSetting("XO_SERVICES_RESPONSE",
    DEFAULT_XO_SERVICES_RESPONSE));
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
    if (error.empty() && !handle.empty())
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
      ::SendAll(notice, UserFlags::OPER);
      
      if (vars[VAR_AUTO_SAVE]->getBool())
      {
        Config::saveSettings();
      }
    }
    return error;
  }
}


int
Vars::get(BotClient * client, const std::string & name) const
{
  int count = 0;

  for (int i = 0; i < VAR_COUNT; ++i)
  {
    if (Same(this->vec[i]->getName(), name, name.length()))
    {
      std::string value = this->vec[i]->get();
      if (value.length() > 0)
      {
        client->send("*** " + this->vec[i]->getName() + " = " + value);
      }
      else
      {
        client->send("*** " + this->vec[i]->getName() + " is empty.");
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

