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
#include <string>
#include <ctime>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// Std C Headers
#include <stdio.h>

// OOMon Headers
#include "oomon.h"
#include "services.h"
#include "userhash.h"
#include "engine.h"
#include "irc.h"
#include "util.h"
#include "main.h"
#include "log.h"
#include "vars.h"
#include "format.h"
#include "config.h"
#include "defaults.h"


Services services;


int Services::servicesCheckInterval_(DEFAULT_SERVICES_CHECK_INTERVAL);
int Services::servicesCloneLimit_(DEFAULT_SERVICES_CLONE_LIMIT);
AutoAction Services::spamtrapAction_(DEFAULT_SPAMTRAP_ACTION,
    DEFAULT_SPAMTRAP_ACTION_TIME);
std::string Services::spamtrapDefaultReason_(DEFAULT_SPAMTRAP_DEFAULT_REASON);
bool Services::spamtrapEnable_(DEFAULT_SPAMTRAP_ENABLE);
int Services::spamtrapMinScore_(DEFAULT_SPAMTRAP_MIN_SCORE);
std::string Services::spamtrapNick_(DEFAULT_SPAMTRAP_NICK);
std::string Services::spamtrapUserhost_(DEFAULT_SPAMTRAP_USERHOST);
AutoAction Services::xoServicesCloneAction_(DEFAULT_XO_SERVICES_CLONE_ACTION,
    DEFAULT_XO_SERVICES_CLONE_ACTION_TIME);
std::string Services::xoServicesCloneReason_(DEFAULT_XO_SERVICES_CLONE_REASON);
bool Services::xoServicesEnable_(DEFAULT_XO_SERVICES_ENABLE);
std::string Services::xoServicesRequest_(DEFAULT_XO_SERVICES_REQUEST);
std::string Services::xoServicesResponse_(DEFAULT_XO_SERVICES_RESPONSE);


void
Services::check()
{
  std::time_t now = std::time(NULL);

  if ((this->lastCheckedTime + (Services::servicesCheckInterval_ * 60)) < now)
  {
    this->lastCheckedTime = now;

    if (Services::xoServicesEnable_)
    {
      server.msg(Services::xoServicesRequest_, "clones " +
          boost::lexical_cast<std::string>(Services::servicesCloneLimit_));
    }

    if (Services::spamtrapEnable_)
    {
      server.whois(Services::spamtrapNick_);
    }

#ifdef CA_SERVICES
    /* check if services.ca just signed on or off */
    server.isOn(CA_SERVICES_REQUEST);
#endif
  }
}


void
Services::onXoNotice(std::string text)
{
  if (!Services::xoServicesEnable_)
  {
    return;
  }

  StrVector parms;
  StrSplit(parms, text, " ", true);

  if ((parms.size() > 3) && server.same("users", parms[3]))
  {
    this->cloningUserhost = parms[0];
    try
    {
      this->cloneCount = boost::lexical_cast<int>(parms[2]);
    }
    catch (boost::bad_lexical_cast)
    {
      this->cloneCount = 0;
    }
    this->suggestKline = true;
  }
  else if ((parms.size() > 2) && server.same("on", parms[1]) &&
    server.same(server.getServerName(), parms[2]))
  {
    std::string nick = parms[0];
    std::string count(boost::lexical_cast<std::string>(this->cloneCount));

    std::string notice(Services::xoServicesResponse_);
    notice += " reports ";
    notice += count;
    notice += " users cloning ";
    notice += this->cloningUserhost;
    notice += " nick ";
    notice += nick;

    ::SendAll(notice, UserFlags::OPER);
    Log::Write(notice);

    bool exempt(false);
    BotSock::Address ip(INADDR_NONE);
    UserEntryPtr find(users.findUser(nick, this->cloningUserhost));
    if (find)
    {
      ip = find->getIP();
      exempt = find->getOper() || config.isExempt(find, Config::EXEMPT_CLONE) ||
        config.isOper(find);
    }
    else
    {
      // If we can't find the user in our userlist, we won't be able to find
      // its IP address either
      exempt = config.isExempt(this->cloningUserhost, Config::EXEMPT_CLONE) ||
        config.isOper(this->cloningUserhost);
    }

    Format reason;
    reason.setStringToken('n', nick);
    reason.setStringToken('u', this->cloningUserhost);
    reason.setStringToken('i', BotSock::inet_ntoa(ip));
    reason.setStringToken('c', count);
    reason.setStringToken('s', Services::xoServicesResponse_);

    if (!exempt)
    {
      doAction(nick, this->cloningUserhost, ip,
          Services::xoServicesCloneAction_,
          reason.format(Services::xoServicesCloneReason_), this->suggestKline);
    }

    this->suggestKline = false;
  }
}


void
Services::onCaNotice(std::string Text)
{
#ifdef CA_SERVICES
  char *text[MAX_BUFF];
  char *body;
  char *parm1, *parm2, *parm3, *parm4;
  char *user, *host, *tmp;
  char dccbuff[MAX_BUFF];
  char userathost[MAX_HOST];
  
        while(*body == ' ')
                body++;

        parm1 = strtok(body, " ");
        if (parm1 == NULL)
                return;

        parm2 = strtok(NULL, " ");
        if(parm2 == NULL)
                return;

        parm3 = strtok(NULL, " ");
        if(parm3 == NULL)
                return;

        parm4 = strtok(NULL, " ");
        if(parm4 == NULL)
                return;
  
        /*
         * If this is services.ca suggesting a kline, we'll do it
         * ourselves, thanks very much.  Just return.
         */     
        if (strcasecmp("kline",parm2) == 0)
                return;
        
        if (strcasecmp("Nickflood",parm1) == 0)
        {
//#if defined(AUTOKSERVICES) && defined(AUTO_KILL_NICK_FLOODERS)
//          if (vars[VAR_AUTO_PILOT]->getBool())
//          {
//            server.kill("Auto-Kill", parm3, "CaSvcs-Reported Nick Flooding");
//            sprintf(dccbuff,
//              "AutoKilled %s %s for CaSvcs-Reported Nick Flooding", parm3,
//              parm4);
//            ::SendAll(dccbuff, UserFlags::OPER, WATCH_KILLS, NULL);
//            Log::Write(dccbuff);
//          }
//          else
//#endif   
//          { 
//            sprintf(dccbuff, "CaSvcs-Reported Nick flooding %s %s", parm3,
//              parm4);
//	      ::SendAll(dccbuff, UserFlags::OPER, WATCH_KILLS, NULL);
//	      Log::Write(dccbuff);
//          }
        }
            
        sprintf(dccbuff,"%s: %s %s %s %s", CA_SERVICES_RESPONSE, parm1, parm2,
	  parm3, strtok(NULL, ""));
        ::SendAll(dccbuff, UserFlags::OPER); 
	Log::Write(dccbuff);
        
        /*              
         * Nope, services.ca is reporting clones.  Determine the
         * user@host.
         */ 

        tmp = strtok(dccbuff, "(");	/* Chop off up to the '(' */
        tmp = strtok(NULL, ")"); 	/* Chop off up to the ')' */
                
        user = strtok(tmp, "@");	/* Chop off up to the ')' */
        if (user == NULL)
                return;			/* Hrm, strtok couldn't parse. */
              
        host = strtok(NULL, "");
        if (host == NULL)
                return;			/* Hrm, strtok couldn't parse. */

        if (strcasecmp("user-clones", parm2) == 0)
	{
                /*      
                 * Is this services.ca reporting multiple user@hosts?
                 */
                suggestKline(false,user,host,false,(*user != '~'),MK_CLONES);
        }
	else if (strcasecmp("site-clones",parm2) == 0)
	{
                /*
                 * Multiple site clones?
                 */
                suggestKline(false,user,host,true,(*user != '~'),MK_CLONES);
        }
	else if (strcasecmp("Nickflood",parm1) == 0)
	{
                /*
                 * Nick flooding twit.
                 */
                suggestKline(false,user,host,false,(*user != '~'),MK_FLOODING);
        }
#endif /* CA_SERVICES */
}

void
Services::onIson(const std::string & text)
{
#ifdef CA_SERVICES
  StrVector nicks;

  StrSplit(nicks, " ", server.upCase(text), true);

  if (nicks.end() == nicks.find(server.upCase(CA_SERVICES_REQUEST)))
  {
    // CA services disconnected - stop receiving reports
    this->gettingCaReports = false;
  }
  else
  {
    // CA services is connected 
    if (!this->gettingCaReports)
    {
      // Start receiving reports from CA services
      Msg(CA_SERVICES_REQUEST, "ADDNOTIFY HERE");
      this->gettingCaReports = true;
    }
  }
#endif
}


void
Services::onSpamtrapMessage(const std::string & text)
{
  std::string copy = text;

  if (0 == server.upCase(FirstWord(copy)).compare("SPAM"))
  {
    if (server.same(server.getServerName(), FirstWord(copy)))
    {
      std::string nick = FirstWord(copy);

      std::string user = FirstWord(copy);
      std::string host = FirstWord(copy);
      std::string userhost = user + '@' + host;

      try
      {
        int score = boost::lexical_cast<int>(FirstWord(copy));

        if (score >= Services::spamtrapMinScore_)
        {
          std::string klineType = FirstWord(copy);

          std::string notice("*** SpamTrap report: ");
	  notice += nick;
	  notice += " (";
	  notice += userhost;
	  notice += ") Score: ";
	  notice += boost::lexical_cast<std::string>(score);
	  notice += " [";
	  notice += copy;
	  notice += ']';

          ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
          Log::Write(notice);

          doAction(nick, userhost, users.getIP(nick, userhost),
              Services::spamtrapAction_, copy, false);
        }
      }
      catch (boost::bad_lexical_cast)
      {
	// just ignore the error
      }
    }
    return;
  }

  std::string notice("*** SpamTrap message: ");
  notice += text;

  ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
  Log::Write(notice);
}


void
Services::onSpamtrapNotice(const std::string & text)
{
// [14-Feb/09:45] -spamtrap!spamtrap@spam.trap- cocol (aan@ip57-12.cbn.net.id) (score 59) on irc2.secsup.org
  std::string copy = text;

  if (0 == text.find("No tracked users on servers matching"))
  {
    // Just ignore this reply
    return;
  }

  std::string nick = FirstWord(copy);
  
  if ((nick.length() > 2) && (nick[0] = BOLD_TOGGLE) &&
    (nick[nick.length() - 1] == BOLD_TOGGLE))
  {
    nick = nick.substr(1, nick.length() - 2);

    if (0 == copy.find("still being processed."))
    {
      // Just ignore this reply
      return;
    }

    std::string userhost = FirstWord(copy);

    if ((userhost.length() > 2) && (userhost[0] = '(') &&
      (userhost[userhost.length() - 1] == ')'))
    {
      userhost = userhost.substr(1, userhost.length() - 2);

      if (0 == server.downCase(FirstWord(copy)).compare("(score"))
      {
	std::string scoreText = FirstWord(copy);

	if ((scoreText.length() > 1) &&
	  (scoreText[scoreText.length() - 1] == ')'))
	{
	  try
	  {
	    int score = boost::lexical_cast<int>(scoreText.substr(0,
	      scoreText.length() - 1));

	    if (0 == server.downCase(FirstWord(copy)).compare("on"))
	    {
	      std::string serverName = FirstWord(copy);

	      if ((score >= Services::spamtrapMinScore_) &&
                  server.same(serverName, server.getServerName()))
	      {
                std::string notice("*** SpamTrap report: ");
	        notice += nick;
	        notice += " (";
	        notice += userhost;
	        notice += ") Score: ";
	        notice += boost::lexical_cast<std::string>(score);

                ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
                Log::Write(notice);

                doAction(nick, userhost, users.getIP(nick, userhost),
                    Services::spamtrapAction_, Services::spamtrapDefaultReason_,
                    false);
	      }
              return;
	    }
	  }
	  catch (boost::bad_lexical_cast)
	  {
	    // just ignore the error
	  }
	}
      }
    }
  }
  else if ((0 == copy.find("tracked users on")) ||
    (0 == copy.find("tracked user on")))
  {
    // Just ignore this header information
    return;
  }

  std::string notice("*** SpamTrap notice: ");
  notice += text;

  ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
  Log::Write(notice);
}


void
Services::pollSpamtrap()
{
  if (Services::spamtrapEnable_)
  {
    server.msg(Services::spamtrapNick_, ".server " + server.getServerName());
  }
}


void
Services::init(void)
{
  vars.insert("SERVICES_CHECK_INTERVAL",
      Setting::IntegerSetting(Services::servicesCheckInterval_, 1));
  vars.insert("SERVICES_CLONE_LIMIT",
      Setting::IntegerSetting(Services::servicesCloneLimit_, 2));
  vars.insert("SPAMTRAP_ACTION",
      AutoAction::Setting(Services::spamtrapAction_));
  vars.insert("SPAMTRAP_DEFAULT_REASON",
      Setting::StringSetting(Services::spamtrapDefaultReason_));
  vars.insert("SPAMTRAP_ENABLE",
      Setting::BooleanSetting(Services::spamtrapEnable_));
  vars.insert("SPAMTRAP_MIN_SCORE",
      Setting::IntegerSetting(Services::spamtrapMinScore_, 0));
  vars.insert("SPAMTRAP_NICK",
      Setting::StringSetting(Services::spamtrapNick_));
  vars.insert("SPAMTRAP_USERHOST",
      Setting::StringSetting(Services::spamtrapUserhost_));
  vars.insert("XO_SERVICES_CLONE_ACTION",
      AutoAction::Setting(Services::xoServicesCloneAction_));
  vars.insert("XO_SERVICES_CLONE_REASON",
      Setting::StringSetting(Services::xoServicesCloneReason_));
  vars.insert("XO_SERVICES_ENABLE",
      Setting::BooleanSetting(Services::xoServicesEnable_));
  vars.insert("XO_SERVICES_REQUEST",
      Setting::StringSetting(Services::xoServicesRequest_));
  vars.insert("XO_SERVICES_RESPONSE",
      Setting::StringSetting(Services::xoServicesResponse_));
}

