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

#include <fstream>
#include <string>
#include <list>
#include <algorithm>

#include "helptopic.h"
#include "config.h"
#include "util.h"
#include "botclient.h"

HelpTopic::HelpTopic(void)
{
  topics.clear();
  syntax.clear();
  description.clear();
  example.clear();
  helpLink.clear();
  subTopic.clear();
  flags = UserFlags::NONE();
  error = 0;
}


HelpTopic::HelpTopic(const HelpTopic & copy)
{
  topics = copy.topics;
  syntax = copy.syntax;
  description = copy.description;
  example = copy.example;
  helpLink = copy.helpLink;
  subTopic = copy.subTopic;
  flags = copy.flags;
  error = 0;
}


HelpTopic::~HelpTopic(void)
{
}


bool
HelpTopic::operator==(const std::string & topic) const
{
  StrVector words;
  int count = StrSplit(words, topic, " ", true);

  std::string topicMaj, topicMin;

  if (count > 0)
  {
    topicMaj = words[0];
    if (count > 1)
    {
      topicMin = words[1];
    }
  }

  for (StrVector::const_iterator pos = topics.begin();
    pos != topics.end(); ++pos)
  {
    std::string current = *pos;

    if ((topicMaj == FirstWord(current)) && (topicMin == FirstWord(current)))
    {
      return true;
    }
  }

  return false;
}


bool
HelpTopic::getHelp(BotClient * client)
{
  bool result = false;
  if (topics.size() > 0)
  {
    result = true;

    if (syntax.size() > 0)
    {
      client->send("\002Syntax:\002");
      for (StrList::iterator pos = syntax.begin();
        pos != syntax.end(); ++pos)
      {
        client->send("  ." + *pos);
      }
    }

    if (description.size() > 0)
    {
      client->send("\002Description:\002");
      for (StrList::iterator pos = description.begin();
        pos != description.end(); ++pos)
      {
        client->send("  " + *pos);
      }
    }

    if (example.size() > 0)
    {
      client->send("\002Example:\002");
      for (StrList::iterator pos = example.begin();
        pos != example.end(); ++pos)
      {
        client->send("  " + *pos);
      }
    }

    client->send("\002Required Flags:\002");
    if (flags == UserFlags::NONE())
    {
      client->send("  None");
    }
    else
    {
      if (flags.has(UserFlags::CHANOP))
      {
        client->send("  C - Chanop");
      }
      if (flags.has(UserFlags::DLINE))
      {
        client->send("  D - Dline");
      }
      if (flags.has(UserFlags::GLINE))
      {
        client->send("  G - Gline");
      }
      if (flags.has(UserFlags::KLINE))
      {
        client->send("  K - Kline");
      }
      if (flags.has(UserFlags::MASTER))
      {
        client->send("  M - Master");
      }
      if (flags.has(UserFlags::OPER))
      {
        client->send("  O - Oper");
      }
      if (flags.has(UserFlags::REMOTE))
      {
        client->send("  R - Remote");
      }
      if (flags.has(UserFlags::WALLOPS))
      {
        client->send("  W - Wallops");
      }
    }

    if (helpLink.size() > 0)
    {
      std::string row = std::string("");

      client->send("\002See Also:\002");
      for (StrList::iterator pos = helpLink.begin();
        pos != helpLink.end(); ++pos)
      {
        if (row.length() == 0)
        {
          row = DownCase(*pos);
        }
        else
        {
          if ((row.length() + pos->length() + 2) > 40)
          {
            client->send("  " + row);
	    row = DownCase(*pos);
	  }
	  else
	  {
	    row += "  ";
	    row += DownCase(*pos);
	  }
        }
      }
      if (row.length() > 0)
      {
        client->send("  " + row);
      }
    }

    if (subTopic.size() > 0)
    {
      std::string row = std::string("");

      std::string major = topics[0];
      major = FirstWord(major);

      if (Same("info", major))
      {
        client->send("\002Topics:\002 (.info <topic>)");
      }
      else
      {
        client->send("\002Sub-Topics:\002 (.help " + major + " <sub-topic>)");
      }
      for (StrList::iterator pos = subTopic.begin();
        pos != subTopic.end(); ++pos)
      {
        if (row.length() == 0)
        {
          row = DownCase(*pos);
        }
        else
        {
          if ((row.length() + pos->length() + 2) > 40)
	  {
            client->send("  " + row);
	    row = DownCase(*pos);
	  }
	  else
	  {
	    row += "  ";
	    row += DownCase(*pos);
	  }
        }
      }
      if (row.length() > 0)
      {
        client->send("  " + row);
      }
    }
  }

  return result;
}


bool
HelpTopic::getHelp(BotClient * client, const std::string & topic)
{
  std::string	line;
  bool		inTopic = false;

  std::ifstream	helpFile(Config::getHelpFilename().c_str());
  if (!helpFile.is_open())
  {
    error = true;
    return false;
  }

  while (std::getline(helpFile, line))
  {
    std::string::size_type crlf = line.find_first_of("\r\n");
    if (crlf != std::string::npos)
    {
      line = line.substr(0, crlf);
    }

    if ((line.length() >= 2) && (line[0] == '.'))
    {
      if (!inTopic)
      {
	continue;
      }

      char code = line[1];
      std::string::size_type dot = line.find('.', 1);
      if (dot != std::string::npos)
      {
        std::string value = line.substr(dot + 1);

	if (code == 's')
	{
	  syntax.push_back(value);
	}

	if (code == 'd')
	{
	  description.push_back(value);
	}

	if (code == 'e')
	{
	  example.push_back(value);
	}

	if (code == 'f')
	{
	  if (value.find('c') != std::string::npos)
	  {
	    flags |= UserFlags::CHANOP;
	  }
	  if (value.find('d') != std::string::npos)
	  {
	    flags |= UserFlags::DLINE;
	  }
	  if (value.find('g') != std::string::npos)
	  {
	    flags |= UserFlags::GLINE;
	  }
	  if (value.find('k') != std::string::npos)
	  {
	    flags |= UserFlags::KLINE;
	  }
	  if (value.find('m') != std::string::npos)
	  {
	    flags |= UserFlags::MASTER;
	  }
	  if (value.find('o') != std::string::npos)
	  {
	    flags |= UserFlags::OPER;
	  }
	  if (value.find('r') != std::string::npos)
	  {
	    flags |= UserFlags::REMOTE;
	  }
	  if (value.find('w') != std::string::npos)
	  {
	    flags |= UserFlags::WALLOPS;
	  }
	}

	if (code == 'l')
	{
	  helpLink.push_back(value);
	}

	if (code == 't')
	{
	  subTopic.push_back(value);
	}
      }
    }
    else if ((line.length() >= 1) && (line[0] != '#'))
    {
      if (topics.size() > 0)
      {
	break;
      }

      StrSplit(topics, line, ".", true);
      inTopic = (topics.end() != std::find(topics.begin(), topics.end(),
	DownCase(topic)));
      if (!inTopic)
      {
	topics.clear();
      }
    }
  }
  helpFile.close();

  return getHelp(client);
}
