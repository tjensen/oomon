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

#include "help.h"
#include "config.h"
#include "util.h"


std::list<HelpTopic> Help::topics;
StrList Help::topicList;


bool
Help::haveTopic(const std::string & topic)
{
  return (topics.end() != std::find(topics.begin(), topics.end(), topic));

  return false;
}


StrList
Help::getIndex(void)
{
  StrList	result;
  std::string	row = std::string("");
  char 		line[MAX_BUFF];
  char		*topic;

  if (topicList.size() <= 0)
  {
    std::ifstream helpFile(Config::getHelpFilename().c_str());
    while (helpFile.getline(line, sizeof(line) - 1))
    {
      if ((line[0] != '.') && (line[0] != '#'))
      {
        topic = strtok(line, ".\r\n");
        while (topic != NULL)
        {
	  if (*topic && (topicList.end() == find(topicList.begin(),
	    topicList.end(), std::string(topic))))
	  {
	    if ((topicList.size() == 0) ||
	      (std::string(topic) > topicList.back()))
	    {
	      topicList.push_back(DownCase(std::string(topic)));
	    }
	    else
	    {
	      StrList::iterator pos;
	      for (pos = topicList.begin();
		(pos != topicList.end()) && (*pos < std::string(topic)); ++pos);
              if (pos != topicList.end())
	      {
	        topicList.insert(pos, DownCase(std::string(topic)));
	      }
	    }
	  }
	  topic = strtok(NULL, ".\r\n");
        }
      }
    }
    helpFile.close();
  }

  result.clear();
  result.push_back("\002Help Index:\002");

  for (StrList::iterator pos = topicList.begin();
    pos != topicList.end(); ++pos)
  {
    if (std::string::npos == pos->find(' '))
    {
      if (row.length() == 0)
      {
        row = std::string("  ") + DownCase(*pos);
      }
      else
      {
        if ((row.length() + pos->length() + 1) >= 40)
        {
          result.push_back(row);
          row = std::string("  ") + DownCase(*pos);
        }
        else
        {
          row += std::string(" ") + DownCase(*pos);
        }
      }
    }
  }
  if (row.length() > 0)
  {
    result.push_back(row);
  }

  return result;
}


StrList
Help::getHelp(const std::string & topic)
{
  if (topic == std::string(""))
  {
    return getIndex();
  }
  else if (haveTopic(topic))
  {
    std::list<HelpTopic>::iterator pos = std::find(topics.begin(), topics.end(),
      topic);

    if (pos != topics.end())
    {
      return pos->getHelp();
    }
    else
    {
      StrList result;
      result.push_back(std::string("No help available for: ") + topic);
      return result;
    }
  }
  else
  {
    HelpTopic newTopic;
    StrList result;

    result = newTopic.getHelp(topic);

    if (result.size() > 0)
    {
      topics.push_back(newTopic);
    }
    else
    {
      if (newTopic.hadError())
      {
        result.push_back(std::string("Error reading help file!"));
        result.push_back(
	  std::string("Try http://oomon.sourceforge.net/ for help."));
      }
      else
      {
        result.push_back(std::string("No help available for: ") + topic);
      }
    }
    return result;
  }
}


void
Help::flush(void)
{
  topics.clear();
  topicList.clear();
}
