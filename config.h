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
#include <map>
#include <bitset>

// Boost C++ Headers
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "botsock.h"
#include "pattern.h"
#include "userentry.h"
#include "botexcept.h"

class Config
{
  public:
    static void init(void);

    Config(void);
    Config(const std::string & filename);
    virtual ~Config(void);

    bool authUser(const std::string & handle, const std::string & username,
        const std::string & password, class UserFlags & flags,
        std::string & newhandle) const;

    bool linkable(const std::string & address) const;
    bool connect(std::string & handle, std::string & address,
        BotSock::Port & port, std::string & password) const;
    bool authBot(const std::string &, const std::string &, const std::string &)
      const;

    std::string hostname(void) const { return this->hostname_; }
    std::string username(void) const;
    std::string nickname(void) const { return this->nick_; }
    std::string realName(void) const { return this->realName_; }
    std::string operName(void) const { return this->operName_; }
    std::string operPassword(void) const { return this->operPassword_; }

    std::string serverAddress(void) const { return this->serverAddress_; }
    BotSock::Port serverPort(void) const { return this->serverPort_; }
    std::string serverPassword(void) const { return this->serverPassword_; }
    std::string channels(void) const { return this->channels_; }

    bool haveChannel(const std::string & channel) const;

    std::string logFilename(void) const { return this->logFilename_; }
    std::string motdFilename(void) const { return this->motdFilename_; }
    std::string helpFilename(void) const { return this->helpFilename_; }
    std::string userDBFilename(void) const { return this->userDBFilename_; }
    std::string settingsFilename(void) const { return this->settingsFilename_; }

    enum ExemptFlag
    {
      EXEMPT_CLONE, EXEMPT_FLOOD, EXEMPT_SPOOF, EXEMPT_TRAP, EXEMPT_PROXY,
      EXEMPT_JUPE, EXEMPT_SEEDRAND, EXEMPT_VERSION, EXEMPT_INVALID,

      MAX_EXEMPT
    };

    bool mayChat(const std::string & userhost) const;
    bool mayChat(const std::string & userhost, const BotSock::Address & ip)
      const;

    bool isExempt(const std::string & userhost, const Config::ExemptFlag flag)
      const;
    bool isExempt(const std::string & userhost, const std::string & ip,
        const Config::ExemptFlag flag) const;
    bool isExempt(const std::string & userhost, const BotSock::Address & ip,
        const Config::ExemptFlag flag) const;
    bool isExempt(const UserEntryPtr & user, const Config::ExemptFlag flag)
      const;
    bool isExemptClass(const std::string & name, const Config::ExemptFlag flag)
      const;
    bool isOper(const std::string & userhost) const;
    bool isOper(const std::string & userhost, const std::string & ip) const;
    bool isOper(const std::string & userhost, const BotSock::Address & ip)
      const;
    bool isOper(const UserEntryPtr & user) const;

    bool spoofer(const std::string & ip) const;

    std::string classDescription(const std::string & name) const;
    UserFlags remoteFlags(const std::string & client) const;

    bool isOpenProxyLine(const std::string & text) const;
    StrVector proxySendLines(void) const;
    BotSock::Address proxyTargetAddress(void) const;
    BotSock::Address proxyTargetPort(void) const;
    std::string proxyVhost(void) const;

    BotSock::Port remotePort(void) const { return this->remotePort_; }
    BotSock::Port dccPort(void) const { return this->dccPort_; }

    bool saveSettings(void) const;
    bool loadSettings(void);

    static bool autoSave(void) { return Config::autoSave_; }

    struct parse_failed : public OOMon::oomon_error
    {
      parse_failed(const std::string & arg) : oomon_error(arg) { };
    };

  private:
    typedef std::bitset<Config::MAX_EXEMPT> ExemptFlags;

    static UserFlags userFlags(const std::string & text,
        const bool remote = false);
    static Config::ExemptFlags exemptFlags(const std::string & text);

    typedef boost::function<void(const StrVector &)>
      ParserFunction;

    void initialize(void);
    void addParser(const std::string & key, const StrVector::size_type args,
        const Config::ParserFunction function);

    void parse(const std::string & filename);

    void parseBLine(const StrVector & fields);
    void parseCLine(const StrVector & fields);
    void parseDLine(const StrVector & fields);
    void parseELine(const StrVector & fields);
    void parseEcLine(const StrVector & fields);
    void parseFLine(const StrVector & fields);
    void parseGLine(const StrVector & fields);
    void parseHLine(const StrVector & fields);
    void parseILine(const StrVector & fields);
    void parseLLine(const StrVector & fields);
    void parseMLine(const StrVector & fields);
    void parseOLine(const StrVector & fields);
    void parsePLine(const StrVector & fields);
    void parseProxyMatchLine(const StrVector & fields);
    void parseProxySendLine(const StrVector & fields);
    void parseProxyTargetLine(const StrVector & fields);
    void parseProxyVhostLine(const StrVector & fields);
    void parseRLine(const StrVector & fields);
    void parseSLine(const StrVector & fields);
    void parseTLine(const StrVector & fields);
    void parseULine(const StrVector & fields);
    void parseYLine(const StrVector & fields);

    struct Oper;
    struct Link;
    struct Connect;
    struct Remote;
    struct Exempt;
    struct Parser;

    typedef std::multimap<std::string, boost::shared_ptr<Config::Oper> >
      OperMap;
    typedef std::multimap<std::string, boost::shared_ptr<Config::Link> >
      LinkMap;
    typedef std::map<std::string, boost::shared_ptr<Config::Connect> >
      ConnectMap;
    typedef std::map<std::string, std::string> YLineMap;
    typedef std::list<boost::shared_ptr<Config::Remote> > RemoteList;
    typedef std::list<boost::shared_ptr<Config::Exempt> > ExemptList;
    typedef std::list<PatternPtr> PatternList;
    typedef std::map<std::string, boost::shared_ptr<Config::Parser> > ParserMap;

    class syntax_error;
    class bad_port_number;

    Config::ParserMap parser_;
    Config::OperMap opers_;
    Config::RemoteList remotes_;
    Config::ConnectMap connects_;
    Config::LinkMap links_;
    Config::YLineMap ylines_;
    Config::ExemptList exempts_;
    Config::ExemptList classExempts_;
    Config::PatternList spoofers_;
    std::string nick_;
    std::string username_;
    std::string hostname_;
    std::string realName_;
    std::string operName_;
    std::string operPassword_;
    std::string serverAddress_;
    BotSock::Port serverPort_;
    std::string serverPassword_;
    std::string channels_;
    Config::PatternList proxyMatches_;
    StrVector proxySendLines_;
    BotSock::Address proxyTargetAddress_;
    BotSock::Port proxyTargetPort_;
    std::string proxyVhost_;
    std::string logFilename_;
    std::string motdFilename_;
    std::string helpFilename_;
    std::string userDBFilename_;
    std::string settingsFilename_;
    BotSock::Port remotePort_;
    BotSock::Port dccPort_;

    static bool operOnlyDcc_;
    static bool autoSave_;
};


extern Config config;


#endif /* __CONFIG_H__ */

