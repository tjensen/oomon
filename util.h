#ifndef __UTIL_H__
#define __UTIL_H__
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
#include <ctime>

// OOMon Headers
#include "strtype"


enum TimeStampFormat
{
  TIMESTAMP_CLIENT, TIMESTAMP_LOG, TIMESTAMP_KLINE
};


// Split tokenized (char *) into a StrVector
int StrSplit(StrVector &, std::string, const std::string &,
  bool ignoreEmpties = false);

// Join StrVector into a std::string
std::string StrJoin(char, const StrVector &);

// Splits data from an IRC server into its pieces
int SplitIRC(StrVector &, std::string);

// Splits a nick!user@host into a nick and a user@host
void SplitFrom(std::string, std::string &, std::string &);

// Returns the nick in a nick!user@host thingy
std::string getNick(const std::string &);

// Retrieves the first word in a string
std::string FirstWord(std::string &);

// Converts a string to uppercase/lowercase
char UpCase(const char);
std::string UpCase(const std::string &);
char DownCase(const char);
std::string DownCase(const std::string &);

// Compares two strings (case-insensitive)
bool Same(const std::string & Text1, const std::string & Text2);
bool Same(const std::string & Text1, const std::string & Text2,
  const std::string::size_type length);

// Compares two passwords
bool ChkPass(std::string, std::string);

// Returns a representation of the date and time
std::string timeStamp(const TimeStampFormat format = TIMESTAMP_LOG,
  std::time_t when = std::time(NULL));

// Returns a string representation of an integer
std::string IntToStr(int i, std::string::size_type len = 0);

std::string GetBotHB(std::string);
std::string GetHandleHB(std::string);

std::string::size_type CountChars(const std::string & text,
  const std::string::value_type ch);

bool isNumericIPv4(const std::string & host);
bool isNumericIPv6(const std::string & host);
bool isNumericIP(const std::string & host);
std::string getDomain(std::string host, bool withDot = false);
std::string klineMask(const std::string & userhost);
std::string classCMask(const std::string & ip);

bool isNumeric(const std::string & text);
bool isDynamic(const std::string & user, const std::string & host);

std::string timeDiff(std::time_t diff);

std::string trimLeft(const std::string & text);
std::string padLeft(const std::string & text,
  const std::string::size_type width,
  const std::string::value_type padding = ' ');
std::string padRight(const std::string & text,
  const std::string::size_type width,
  const std::string::value_type padding = ' ');

std::string hexDump(const void *buffer, const int size);

std::string chopUserhost(std::string NUH);
std::string chopNick(std::string NUH);

std::string expandPath(const std::string & filename, const std::string & cwd);

#endif /* __UTIL_H__ */

