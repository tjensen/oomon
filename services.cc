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


Services services;


void
Services::check()
{
  std::time_t now = std::time(NULL);

  if ((this->lastCheckedTime +
    (vars[VAR_SERVICES_CHECK_INTERVAL]->getInt() * 60)) < now)
  {
    this->lastCheckedTime = now;

    if (vars[VAR_XO_SERVICES_ENABLE]->getBool())
    {
      server.msg(vars[VAR_XO_SERVICES_REQUEST]->getString(), "clones " +
        IntToStr(vars[VAR_SERVICES_CLONE_LIMIT]->getInt()));
    }

    if (vars[VAR_SPAMTRAP_ENABLE]->getBool())
    {
      server.whois(vars[VAR_SPAMTRAP_NICK]->getString());
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
  if (!vars[VAR_XO_SERVICES_ENABLE]->getBool())
  {
    return;
  }

  StrVector parms;
  StrSplit(parms, text, " ", true);

  if ((parms.size() > 3) && server.same("users", parms[3]))
  {
    this->cloningUserhost = parms[0];
    this->cloneCount = atoi(parms[2].c_str());
    this->suggestKline = true;
  }
  else if ((parms.size() > 2) && server.same("on", parms[1]) &&
    server.same(server.getServerName(), parms[2]))
  {
    std::string nick = parms[0];
    std::string count(IntToStr(this->cloneCount));

    std::string notice(vars[VAR_XO_SERVICES_RESPONSE]->getString());
    notice += " reports ";
    notice += count;
    notice += " users cloning ";
    notice += this->cloningUserhost;
    notice += " nick ";
    notice += nick;

    ::SendAll(notice, UserFlags::OPER);
    Log::Write(notice);

    BotSock::Address ip = users.getIP(nick, this->cloningUserhost);

    Format reason;
    reason.setStringToken('n', nick);
    reason.setStringToken('u', this->cloningUserhost);
    reason.setStringToken('i', BotSock::inet_ntoa(ip));
    reason.setStringToken('c', count);
    reason.setStringToken('s', vars[VAR_XO_SERVICES_RESPONSE]->getString());

    doAction(nick, this->cloningUserhost, ip,
      vars[VAR_XO_SERVICES_CLONE_ACTION]->getAction(),
      vars[VAR_XO_SERVICES_CLONE_ACTION]->getInt(),
      reason.format(vars[VAR_XO_SERVICES_CLONE_REASON]->getString()),
      this->suggestKline);

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

      int score = atoi(FirstWord(copy).c_str());

      if (score >= vars[VAR_SPAMTRAP_MIN_SCORE]->getInt())
      {
        std::string klineType = FirstWord(copy);

        std::string notice("*** SpamTrap report: " + nick + " (" + userhost +
          ") Score: " + IntToStr(score) + " [" + copy + "]");

        ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
        Log::Write(notice);

        doAction(nick, userhost, users.getIP(nick, userhost),
	  vars[VAR_SPAMTRAP_ACTION]->getAction(),
	  vars[VAR_SPAMTRAP_ACTION]->getInt(), copy, false);
      }
    }
    return;
  }

  std::string notice("*** SpamTrap message: " + text);

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
	  int score = atoi(scoreText.substr(0, scoreText.length() - 1).c_str());

	  if (0 == server.downCase(FirstWord(copy)).compare("on"))
	  {
	    std::string serverName = FirstWord(copy);

	    if ((score >= vars[VAR_SPAMTRAP_MIN_SCORE]->getInt()) &&
	      server.same(serverName, server.getServerName()))
	    {
              std::string notice("*** SpamTrap report: " + nick + " (" +
	        userhost + ") Score: " + IntToStr(score));

              ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
              Log::Write(notice);

              doAction(nick, userhost, users.getIP(nick, userhost),
	        vars[VAR_SPAMTRAP_ACTION]->getAction(),
	        vars[VAR_SPAMTRAP_ACTION]->getInt(),
	        vars[VAR_SPAMTRAP_DEFAULT_REASON]->getString(), false);

	    }
            return;
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

  std::string notice("*** SpamTrap notice: " + text);

  ::SendAll(notice, UserFlags::OPER, WATCH_SPAMTRAP);
  Log::Write(notice);
}


void
Services::pollSpamtrap()
{
  if (vars[VAR_SPAMTRAP_ENABLE]->getBool())
  {
    server.msg(vars[VAR_SPAMTRAP_NICK]->getString(), ".server " +
      server.getServerName());
  }
}

