#ifndef __PATTERN_H__
#define __PATTERN_H__
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
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

// Std C Headers
#include <sys/types.h>

// OOMon Headers
#include "oomon.h"

#if defined(HAVE_LIBPCRE)
# include <pcre.h>
#elif defined(HAVE_POSIX_REGEX)
# include <regex.h>
#endif


typedef boost::shared_ptr<class Pattern> PatternPtr;


bool MatchesMask(std::string TEST, std::string MASK, bool special = false);


class Pattern : boost::noncopyable
{
public:
  explicit Pattern(const std::string & text = "");
  virtual ~Pattern(void);

  std::string get(void) const { return this->pattern_; };

  virtual bool match(const std::string & text) const = 0;

protected:
  const std::string pattern_;
};


class ClusterPattern : public Pattern
{
public:
  explicit ClusterPattern(const std::string & text) : Pattern(text) { }
  virtual ~ClusterPattern(void) { }

  virtual bool match(const std::string & text) const
  {
    return MatchesMask(text, this->get());
  }
};


class NickClusterPattern : public Pattern
{
public:
  explicit NickClusterPattern(const std::string & text) : Pattern(text) { }
  virtual ~NickClusterPattern(void) { }

  virtual bool match(const std::string & text) const
  {
    return MatchesMask(text, this->get(), true);
  }
};


class RegExPattern : public Pattern
{
public:
  explicit RegExPattern(const std::string & text);
  virtual ~RegExPattern(void);

  virtual bool match(const std::string & text) const;

private:
#if defined(HAVE_LIBPCRE)
  pcre * regex;
  pcre_extra * extra;
#elif defined(HAVE_POSIX_REGEX)
  regex_t regex;
#endif
};


PatternPtr smartPattern(const std::string & text, const bool nick);

std::string grabPattern(std::string & input,
  const std::string & delimiters = " ");


#endif /* __PATTERN_H__ */

