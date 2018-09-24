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

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// Std C Headers
#include <sys/types.h>

// OOMon Headers
#include "oomon.h"
#include "pattern.h"
#include "botexcept.h"
#include "util.h"
#include "irc.h"

#if defined(HAVE_LIBPCRE)
# include <pcre.h>
#elif defined(HAVE_POSIX_REGEX)
# include <regex.h>
#endif


#ifdef PATTERN_DEBUG
static unsigned long patternCount = 0;
#endif


Pattern::Pattern(const std::string & text) : pattern_(text)
{
#ifdef PATTERN_DEBUG
  ++patternCount;
#endif
}


Pattern::~Pattern(void)
{
#ifdef PATTERN_DEBUG
  --patternCount;
#endif
}


RegExPattern::RegExPattern(const std::string & text) : Pattern(text)
{
  std::string realPattern;
  bool insensitive = false;

  if ((text.length() > 0) && (text[0] == '/'))
  {
    std::string::size_type pos = 1;
    bool foundEnd = false;

    while (pos < text.length())
    {
      if (text[pos] == '/')
      {
	foundEnd = true;
	realPattern = text.substr(1, pos - 1);
	pos++;
	break;
      }
      else if (text[pos] == '\\')
      {
	pos += 2;
      }
      else
      {
	pos++;
      }
    }

    if (foundEnd)
    {
      while (pos < text.length())
      {
	switch (text[pos])
	{
	  case 'i':
	  case 'I':
	    insensitive = true;
	    break;
	  default:
	    throw OOMon::regex_error(std::string("Invalid flag '") +
	      text[pos] + "'");
	}
	pos++;
      }
    }
    else
    {
      throw OOMon::regex_error("No closing slash");
    }
  }
  else
  {
    realPattern = text;
  }

#if defined(HAVE_LIBPCRE)
  const char *errstr = NULL;
  int erroffset;

  this->regex = pcre_compile(realPattern.c_str(),
    insensitive ? PCRE_CASELESS : 0, &errstr, &erroffset, NULL);
  if (NULL == this->regex)
  {
    std::string error((errstr != NULL) ? errstr : "unknown");
    throw OOMon::regex_error(error);
  }

  this->extra = pcre_study(this->regex, 0, &errstr);
#elif defined(HAVE_POSIX_REGEX)
  if (int result = regcomp(&this->regex, realPattern.c_str(),
    REG_EXTENDED | REG_NOSUB | (insensitive ? REG_ICASE : 0)))
  {
    char buffer[128];
    regerror(result, &this->regex, buffer, sizeof(buffer));
    throw OOMon::regex_error(std::string(buffer));
  }
#else
  throw OOMon::regex_error("No regular expression support!");
#endif
}


RegExPattern::~RegExPattern()
{
#if defined(HAVE_LIBPCRE)
  pcre_free(this->regex);
  if (NULL != this->extra)
  {
    pcre_free(this->extra);
  }
#elif defined(HAVE_POSIX_REGEX)
  regfree(&this->regex);
#endif
}


bool
RegExPattern::match(const std::string & text) const
{
#if defined(HAVE_LIBPCRE)
  int result = pcre_exec(this->regex, this->extra, text.c_str(), text.length(),
    0, 0, NULL, 0);
  if (result >= 0)
  {
    return true;
  }
  else if (result == PCRE_ERROR_NOMATCH)
  {
    return false;
  }
  else
  {
    throw OOMon::regex_error(std::string("pcre_exec returned ") +
      boost::lexical_cast<std::string>(result));
  }
#elif defined(HAVE_POSIX_REGEX)
  int result = regexec(&this->regex, text.c_str(), 0, NULL, 0);
  if (result == 0)
  {
    return true;
  }
  else if (result == REG_NOMATCH)
  {
    return false;
  }
  else
  {
    char buffer[128];
    regerror(result, &this->regex, buffer, sizeof(buffer));
    throw OOMon::regex_error(buffer);
  }
#else
  return false;
#endif
}


PatternPtr
smartPattern(const std::string & text, const bool nick)
{
  if ((text.length() > 0) && (text[0] == '/'))
  {
    PatternPtr tmp(new RegExPattern(text));
    return tmp;
  }
  else if (nick)
  {
    PatternPtr tmp(new NickClusterPattern(text));
    return tmp;
  }
  else
  {
    PatternPtr tmp(new ClusterPattern(text));
    return tmp;
  }
}


static bool
matchesChar(char test, char mask)
{
  return ((mask == '?') || (test == mask));
}


static bool
matchesSpecialChar(char test, char mask)
{
  if ((mask == '?') || (test == mask))
  {
    return true;
  }
  else if ((mask == '#') && isdigit(test))
  {
    return true;
  }
  else if ((mask == '&') && isalpha(test))
  {
    return true;
  }
  else if ((mask == '%') && isalnum(test))
  {
    return true;
  }

  return false;
}


static bool
FixedMatch(std::string TEST, std::string MASK, bool special)
{
  if (0 == TEST.compare(MASK))
  {
    return true;
  }
  else
  {
    if (TEST.length() == MASK.length())
    {
      if (special)
      {
        for (std::string::size_type i = 0; i < MASK.length(); i++)
        {
	  if (!matchesSpecialChar(TEST[i], MASK[i]))
	  {
	    return false;
	  }
	}
      }
      else
      {
        for (std::string::size_type i = 0; i < MASK.length(); i++)
        {
	  if (!matchesChar(TEST[i], MASK[i]))
	  {
	    return false;
	  }
	}
      }
    }
    else
    {
      return false;
    }
  }
  return true;
}


static bool
SMatch(std::string TEST, std::string MASK, bool special)
{
  if ((0 == MASK.compare("*")) || (0 == TEST.compare(MASK)))
    return true;
  if (MASK.empty() || TEST.empty())
    return false;
  if ((MASK[0] == '*') && (std::string::npos != MASK.substr(1).find('*')))
  {
    MASK.erase((std::string::size_type) 0, 1);
    for (std::string::size_type i = 0; i < TEST.length(); i++)
    {
      std::string temps = TEST.substr(i);
      std::string::size_type p = MASK.find('*');
      if ((std::string::npos != p) &&
	FixedMatch(temps.substr(0, p), MASK.substr(0, p), special))
      {
	temps = temps.substr(p);
	std::string tempm = MASK.substr(p);
	if (SMatch(temps, tempm, special))
	  return true;
      }
    }
    return false;
  }
  else if (MASK[0] == '*')
  {
    MASK.erase((std::string::size_type) 0, 1);
    TEST.erase(0, TEST.length() - MASK.length());
    return FixedMatch(TEST, MASK, special);
  }
  else
  {
    std::string::size_type p = MASK.find('*');
    if (std::string::npos != p)
    {
      std::string temps = TEST.substr(0, p);
      std::string tempm = MASK.substr(0, p);
      if (FixedMatch(temps, tempm, special))
      {
        TEST.erase(0, p);
        MASK.erase(0, p);
        return SMatch(TEST, MASK, special);
      }
      else
      {
        return false;
      }
    }
    else
    {
      return FixedMatch(TEST, MASK, special);
    }
  }
}


bool
MatchesMask(std::string TEST, std::string MASK, bool special)
{
  return SMatch(server.upCase(TEST), server.upCase(MASK), special);
}


std::string
grabPattern(std::string & input, const std::string & delimiters)
{
  std::string result;

  std::string::size_type len = input.length();
  std::string::size_type pos = 0;

  if ((pos < len) && (input[pos] == '/'))
  {
    // Regular expression, surrounded by slashes.  Find the trailing slash
    // and options, if any, to close the pattern.
    ++pos;
    while (pos < len)
    {
      if (input[pos] == '/')
      {
	++pos;
	break;
      }
      else if (input[pos] == '\\')
      {
	++pos;
      }
      ++pos;
    }
    while (pos < len)
    {
      if (input[pos] == 'i')
      {
	++pos;
      }
      else
      {
	break;
      }
    }
  }
#ifdef SUPPORT_QUOTES_IN_CLUSTER_PATTERNS
  else if ((len > 0) && (input[pos] == '"'))
  {
    // Glob-style pattern, surrounded by double quotes.  Find the trailing
    // double quote to close the pattern.
    ++pos;
    while (pos < len)
    {
      if (input[pos] == '"')
      {
	++pos;
	break;
      }
      ++pos;
    }
  }
#endif
  else
  {
    // Ambiguous pattern type.  Just find the next space to close the pattern.
    ++pos;
    while (pos < len)
    {
      if (std::string::npos != delimiters.find(input[pos]))
      {
	break;
      }
      ++pos;
    }
  }
  if ((pos > 0) && (len > 0))
  {
    result = input.substr(0, pos);
    input = input.substr(pos);
  }
  return result;
}


void
#ifdef PATTERN_DEBUG
patternStatus(BotClient * client)
{
  client->send("Patterns: " + boost::lexical_cast<std::string>(patternCount));
#else
patternStatus(BotClient *)
{
#endif
}

