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

#include <string>

#include "oomon.h"
#include "action.h"
#include "botclient.h"
#include "userentry.h"
#include "format.h"
#include "botsock.h"
#include "irc.h"


Action::Nothing::Nothing(BotClient * client) : client_(client)
{
}


Action::Nothing::~Nothing(void)
{
}


void
Action::Nothing::operator()(const UserEntryPtr user)
{
  // do nothing!
}


Action::List::List(BotClient * client, const FormatSet & formats)
  : Nothing(client), formats_(formats)
{
}


Action::List::~List(void)
{
}


void
Action::List::operator()(const UserEntryPtr user)
{
  std::string text(" ");
  if (this->formats_.test(FORMAT_NICK))
  {
    text += ' ';
    text += user->getNick();
  }
  if (this->formats_.test(FORMAT_USERHOST))
  {
    text += " (";
    text += user->getUserHost();
    text += ')';
  }
  if (this->formats_.test(FORMAT_IP) && (INADDR_NONE != user->getIP()))
  {
    text += " [";
    text += user->getTextIP();
    text += ']';
  }
  if (this->formats_.test(FORMAT_CLASS))
  {
    text += " {";
    text += user->getClass();
    text += '}';
  }
  if (this->formats_.test(FORMAT_GECOS))
  {
    text += " <";
    text += user->getGecos();
    text += '>';
  }

  this->client_->send(text);
}


Action::Kill::Kill(BotClient * client, const std::string & reason)
  : Nothing(client), reason_(reason)
{
}


Action::Kill::~Kill(void)
{
}


void
Action::Kill::operator()(const UserEntryPtr user)
{
  server.kill(this->client_->handleAndBot(), user->getNick(), this->reason_);
}


boost::shared_ptr<Action::Nothing>
Action::parse(BotClient * client, std::string text, const FormatSet & formats)
{
  boost::shared_ptr<Action::Nothing> result;

  text = trimLeft(text);

  std::string cmd(UpCase(FirstWord(text)));

  if (partialCompare(cmd, "COUNT", 1))
  {
    result.reset(new Action::Nothing(client));
  }
  else if (partialCompare(cmd, "KILL", 1))
  {
    result.reset(new Action::Kill(client, text));
  }
  else if (cmd.empty() || partialCompare(cmd, "LIST", 1))
  {
    result.reset(new Action::List(client, formats));
  }
  else
  {
    throw Action::bad_action(cmd);
  }

  return result;
}

