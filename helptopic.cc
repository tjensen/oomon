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

#include <fstream>
#include <string>
#include <list>
#include <algorithm>

#include "helptopic.h"
#include "config.h"
#include "util.h"

HelpTopic::HelpTopic(void)
{
  topics.clear();
  syntax.clear();
  description.clear();
  example.clear();
  helpLink.clear();
  subTopic.clear();
  flags = UF_NONE;
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


StrList
HelpTopic::getHelp(void)
{
  StrList result;

  result.clear();

  if (topics.size() <= 0)
  {
    return result;
  }

  if (syntax.size() > 0)
  {
    result.push_back("\002Syntax:\002");
    for (StrList::iterator pos = syntax.begin();
      pos != syntax.end(); ++pos)
    {
      result.push_back(std::string("  .") + *pos);
    }
  }

  if (description.size() > 0)
  {
    result.push_back("\002Description:\002");
    for (StrList::iterator pos = description.begin();
      pos != description.end(); ++pos)
    {
      result.push_back(std::string("  ") + *pos);
    }
  }

  if (example.size() > 0)
  {
    result.push_back("\002Example:\002");
    for (StrList::iterator pos = example.begin();
      pos != example.end(); ++pos)
    {
      result.push_back(std::string("  ") + *pos);
    }
  }

  result.push_back("\002Required Flags:\002");
  if (flags == UF_NONE)
  {
    result.push_back("  None");
  }
  else
  {
    if (flags & UF_CHANOP)
    {
      result.push_back("  C - Chanop");
    }
    if (flags & UF_DLINE)
    {
      result.push_back("  D - Dline");
    }
    if (flags & UF_GLINE)
    {
      result.push_back("  G - Gline");
    }
    if (flags & UF_KLINE)
    {
      result.push_back("  K - Kline");
    }
    if (flags & UF_MASTER)
    {
      result.push_back("  M - Master");
    }
    if (flags & UF_OPER)
    {
      result.push_back("  O - Oper");
    }
    if (flags & UF_REMOTE)
    {
      result.push_back("  R - Remote");
    }
    if (flags & UF_WALLOPS)
    {
      result.push_back("  W - Wallops");
    }
  }

  if (helpLink.size() > 0)
  {
    std::string row = std::string("");

    result.push_back("\002See Also:\002");
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
          result.push_back(std::string("  ") + row);
	  row = DownCase(*pos);
	}
	else
	{
	  row += std::string("  ") + DownCase(*pos);
	}
      }
    }
    if (row.length() > 0)
    {
      result.push_back(std::string("  ") + row);
    }
  }

  if (subTopic.size() > 0)
  {
    std::string row = std::string("");

    std::string major = topics[0];
    major = FirstWord(major);

    if (Same("info", major))
    {
      result.push_back("\002Topics:\002 (.info <topic>)");
    }
    else
    {
      result.push_back("\002Sub-Topics:\002 (.help " + major + " <sub-topic>)");
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
          result.push_back(std::string("  ") + row);
	  row = DownCase(*pos);
	}
	else
	{
	  row += std::string("  ") + DownCase(*pos);
	}
      }
    }
    if (row.length() > 0)
    {
      result.push_back(std::string("  ") + row);
    }
  }

  return result;
}


StrList
HelpTopic::getHelp(const std::string & topic)
{
  std::string	line;
  bool		inTopic = false;

  std::ifstream	helpFile(Config::getHelpFilename().c_str());
  if (!helpFile.is_open())
  {
    error = true;
    return StrList();
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
	    flags |= UF_CHANOP;
	  }
	  if (value.find('d') != std::string::npos)
	  {
	    flags |= UF_DLINE;
	  }
	  if (value.find('g') != std::string::npos)
	  {
	    flags |= UF_GLINE;
	  }
	  if (value.find('k') != std::string::npos)
	  {
	    flags |= UF_KLINE;
	  }
	  if (value.find('m') != std::string::npos)
	  {
	    flags |= UF_MASTER;
	  }
	  if (value.find('o') != std::string::npos)
	  {
	    flags |= UF_OPER;
	  }
	  if (value.find('r') != std::string::npos)
	  {
	    flags |= UF_REMOTE;
	  }
	  if (value.find('w') != std::string::npos)
	  {
	    flags |= UF_WALLOPS;
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

  return getHelp();
}
