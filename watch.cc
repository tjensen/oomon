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
#include <bitset>
#include <algorithm>

// OOMon Headers
#include "strtype"
#include "watch.h"
#include "util.h"
#include "botexcept.h"
#include "botclient.h"


#ifdef DEBUG
# define WATCH_DEBUG
#endif


Watch
WatchSet::getWatchValue(const std::string & watch)
{
  if (watch == "CHAT")
    return WATCH_CHAT;
  else if (watch == "CONNECTS")
    return WATCH_CONNECTS;
  else if (watch == "CONNFLOOD")
    return WATCH_CONNFLOOD;
  else if (watch == "CTCPVERSIONS")
    return WATCH_CTCPVERSIONS;
  else if (watch == "DISCONNECTS")
    return WATCH_DISCONNECTS;
  else if (watch == "DLINES")
    return WATCH_DLINES;
  else if (watch == "DLINE_MATCHES")
    return WATCH_DLINE_MATCHES;
  else if (watch == "GLINES")
    return WATCH_GLINES;
  else if (watch == "GLINE_MATCHES")
    return WATCH_GLINE_MATCHES;
  else if (watch == "FLOODERS")
    return WATCH_FLOODERS;
  else if (watch == "INFOS")
    return WATCH_INFOS;
  else if (watch == "JUPES")
    return WATCH_JUPES;
  else if (watch == "KILLS")
    return WATCH_KILLS;
  else if (watch == "KLINES")
    return WATCH_KLINES;
  else if (watch == "KLINE_MATCHES")
    return WATCH_KLINE_MATCHES;
  else if (watch == "LINKS")
    return WATCH_LINKS;
  else if (watch == "MOTDS")
    return WATCH_MOTDS;
  else if (watch == "MSGS")
    return WATCH_MSGS;
  else if (watch == "NICK_CHANGES")
    return WATCH_NICK_CHANGES;
  else if (watch == "OPERFAILS")
    return WATCH_OPERFAILS;
  else if (watch == "SEEDRAND")
    return WATCH_SEEDRAND;
  else if (watch == "SPAMBOTS")
    return WATCH_SPAMBOTS;
  else if (watch == "SPAMTRAP")
    return WATCH_SPAMTRAP;
  else if (watch == "STATS")
    return WATCH_STATS;
  else if (watch == "TOOMANYS")
    return WATCH_TOOMANYS;
  else if (watch == "TRACES")
    return WATCH_TRACES;
  else if (watch == "TRAPS")
    return WATCH_TRAPS;
  else if (watch == "WALLOPS")
    return WATCH_WALLOPS;
  else
    throw OOMon::invalid_watch_name(watch);
}


void
WatchSet::set(BotClient * client, std::string line, const bool noisy)
{
  std::string word;

  while ("" != (word = UpCase(FirstWord(line))))
  {
    bool plus = false;
    bool minus = false;

    // Look for a + or - before the watch type
    if (word.length() >= 1)
    {
      if (word[0] == '+')
      {
	plus = true;
	word = word.substr(1);
      }
      else if (word[0] == '-')
      {
	minus = true;
	word = word.substr(1);
      }
    }

    if ((!minus && !plus && (word == "NONE")) || (minus && (word == "ALL")))
    {
      this->clear();
      if (noisy) client->send("*** Removed ALL watches.");
    }
    else if ((word == "DEFAULT") || (word == "DEFAULTS"))
    {
      if (minus)
      {
        this->remove(WatchSet::defaults());
        if (noisy) client->send("*** Removed DEFAULT watches.");
      }
      else if (plus)
      {
        this->add(WatchSet::defaults());
        if (noisy) client->send("*** Added DEFAULT watches.");
      }
      else
      {
        this->clear();
        this->add(WatchSet::defaults());
        if (noisy) client->send("*** Set DEFAULT watches.");
      }
    }
    else if (word == "ALL")
    {
      this->add(WatchSet::all());
      if (noisy) client->send("*** Added ALL watches.");
    }
    else
    {
      try
      {
        Watch type = WatchSet::getWatchValue(word);

        if (minus)
	{
          this->remove(type);
          if (noisy) client->send("*** Removed watch: " +
	    WatchSet::getWatchName(type));
	}
        else
	{
          this->add(type);
          if (noisy) client->send("*** Added watch: " + 
	    WatchSet::getWatchName(type));
	}
      }
      catch (OOMon::invalid_watch_name & e)
      {
        if (noisy) client->send(std::string("*** Invalid WATCH type: ") +
	  e.what());
      }
    }
  }
}


WatchSet
WatchSet::all(void)
{
  WatchSet result;

  result.contents.set();

  return result;
}


WatchSet
WatchSet::defaults(void)
{
  WatchSet result;

  result.add(WATCH_CHAT);
  //result.add(WATCH_CONNECTS);
  result.add(WATCH_CONNFLOOD);
  result.add(WATCH_CTCPVERSIONS);
  //result.add(WATCH_DISCONNECTS);
  result.add(WATCH_DLINES);
  //result.add(WATCH_DLINE_MATCHES);
  //result.add(WATCH_GLINES);
  //result.add(WATCH_GLINE_MATCHES);
  result.add(WATCH_FLOODERS);
  result.add(WATCH_INFOS);
  result.add(WATCH_JUPES);
  result.add(WATCH_KILLS);
  result.add(WATCH_KLINES);
  //result.add(WATCH_KLINE_MATCHES);
  result.add(WATCH_LINKS);
  result.add(WATCH_MOTDS);
  //result.add(WATCH_MSGS);
  //result.add(WATCH_NICK_CHANGES);
  //result.add(WATCH_OPERFAILS);
  result.add(WATCH_SEEDRAND);
  result.add(WATCH_SPAMBOTS);
  result.add(WATCH_SPAMTRAP);
  result.add(WATCH_STATS);
  result.add(WATCH_TOOMANYS);
  result.add(WATCH_TRACES);
  result.add(WATCH_TRAPS);
  result.add(WATCH_WALLOPS);

  return result;
}


std::string
WatchSet::getWatchName(const Watch watch)
{
  if (watch == WATCH_CHAT)
    return "CHAT";
  else if (watch == WATCH_CONNECTS)
    return "CONNECTS";
  else if (watch == WATCH_CONNFLOOD)
    return "CONNFLOOD";
  else if (watch == WATCH_CTCPVERSIONS)
    return "CTCPVERSIONS";
  else if (watch == WATCH_DISCONNECTS)
    return "DISCONNECTS";
  else if (watch == WATCH_DLINES)
    return "DLINES";
  else if (watch == WATCH_DLINE_MATCHES)
    return "DLINE_MATCHES";
  else if (watch == WATCH_GLINES)
    return "GLINES";
  else if (watch == WATCH_GLINE_MATCHES)
    return "GLINE_MATCHES";
  else if (watch == WATCH_FLOODERS)
    return "FLOODERS";
  else if (watch == WATCH_INFOS)
    return "INFOS";
  else if (watch == WATCH_JUPES)
    return "JUPES";
  else if (watch == WATCH_KILLS)
    return "KILLS";
  else if (watch == WATCH_KLINES)
    return "KLINES";
  else if (watch == WATCH_KLINE_MATCHES)
    return "KLINE_MATCHES";
  else if (watch == WATCH_LINKS)
    return "LINKS";
  else if (watch == WATCH_MOTDS)
    return "MOTDS";
  else if (watch == WATCH_MSGS)
    return "MSGS";
  else if (watch == WATCH_NICK_CHANGES)
    return "NICK_CHANGES";
  else if (watch == WATCH_OPERFAILS)
    return "OPERFAILS";
  else if (watch == WATCH_SEEDRAND)
    return "SEEDRAND";
  else if (watch == WATCH_SPAMBOTS)
    return "SPAMBOTS";
  else if (watch == WATCH_SPAMTRAP)
    return "SPAMTRAP";
  else if (watch == WATCH_STATS)
    return "STATS";
  else if (watch == WATCH_TOOMANYS)
    return "TOOMANYS";
  else if (watch == WATCH_TRACES)
    return "TRACES";
  else if (watch == WATCH_TRAPS)
    return "TRAPS";
  else if (watch == WATCH_WALLOPS)
    return "WALLOPS";
  else
    throw OOMon::invalid_watch_value("Unknown WATCH type");
}


std::string
WatchSet::getWatchNames(const WatchSet & watches, const bool distinct,
  const char separator)
{
  std::string result;

  if (distinct)
  {
    for (int i = WATCH_MIN; i <= WATCH_MAX; ++i)
    {
      std::string name = WatchSet::getWatchName((Watch) i);

      if (!result.empty()) result += separator;

      if (watches.has((Watch) i))
      {
        result += '+';
      }
      else
      {
        result += '-';
      }
      result += name;
    }
  }
  else
  {
    if (watches.none())
    {
      result = "NONE";
    }
    else
    {
      for (size_t bit = 0; bit < watches.contents.size(); ++bit)
      {
	if (watches.contents.test(bit))
	{
	  if (!result.empty()) result += separator;
	  result += WatchSet::getWatchName(static_cast<Watch>(bit));
	}
      }
    }
  }

  return result;
}


WatchSet
WatchSet::getWatchValues(StrVector & watches)
{
  WatchSet result;

  for (StrVector::iterator pos = watches.begin(); pos != watches.end(); ++pos)
  {
    if (0 != UpCase(*pos).compare("NONE"))
    {
      result.add(WatchSet::getWatchValue(*pos));
    }
  }

  return result;
}


WatchSet
WatchSet::getWatchValues(const std::string & watches, const char separator)
{
  StrVector vector;

  StrSplit(vector, watches, std::string(1, separator), true);

  return WatchSet::getWatchValues(vector);
}

