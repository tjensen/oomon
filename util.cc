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

// C headers
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

// C++ headers
#include <string>
#include <cctype>

#include "strtype"
#include "oomon.h"
#include "util.h"
#include "remote.h"

// Originally I used strsep() to do this, but quickly learned how
// unportable it makes my code. *sigh* oh well. This works too.
int
StrSplit(StrVector & temp, std::string input, const std::string & tokens,
  bool ignoreEmpties)
{
  int count = 0;

  temp.clear();

  while (input != "")
  {
    std::string::size_type pos = input.find_first_of(tokens);

    if (std::string::npos == pos)
    {
      temp.push_back(input);
      input = "";
      count++;
    }
    else
    {
      std::string unit = input.substr(0, pos);
      std::string rest = input.substr(pos + 1);

      if (!ignoreEmpties || (unit != ""))
      {
        temp.push_back(unit);
        count++;
      }
      input = rest;
    }
  }
  return count;
}


int
ReadLine(register int fd, std::string & text)
{
  int n, rc;
  char c;

  text = "";

  for (n = 1; n < MAX_BUFF; n++) {
    if ((rc = read(fd, &c, 1)) == 1) {
      if ((c != '\n') && (c != '\r'))
	text = text + c;
      else {
	//n--;
	break;
      }
    } else if (rc == 0) {
      if (n == 1)
	return 0;
      else
	break;
    } else
      return -1;
  }
  return n;
}


int
SplitIRC(StrVector & temp, std::string text)
{
  int count = 0;

  temp.clear();
  while (text.length() > 0)
  {
    if (text.at(0) == ':')
    {
      text.erase((std::string::size_type) 0, 1);
      temp.push_back(text);
      text = std::string("");
      count++;
    }
    else
    {
      std::string::size_type i = text.find(' ');
      if (std::string::npos != i)
      {
	if (i == 0)
	{
	  text.erase((std::string::size_type) 0, 1);
	}
	else
	{
	  std::string chunk = text.substr(0, i);
          temp.push_back(chunk);
          text.erase(0, i + 1);
          count++;
	}
      }
      else
      {
        temp.push_back(text);
        text = "";
        count++;
      }
    }
  }
  return count;
}


void
SplitFrom(std::string input, std::string & Nick, std::string & UserHost)
{
  std::string::size_type i = input.find('!');
  if (std::string::npos == i)
  {
    // This is a server, not a nick!user@host
    Nick = input;
    UserHost = "";
  }
  else
  {
    // This is a nick!user@host
    Nick = input.substr(0, i);
    UserHost = input.substr(i + 1);
  }
}


std::string
getNick(const std::string & text)
{
  std::string::size_type bang = text.find('!');

  if (std::string::npos == bang)
  {
    return text;
  }
  else
  {
    return text.substr(0, bang);
  }
}


IRCCommand
getIRCCommand(const std::string & text)
{
  if (text == std::string("PING"))
    return IRC_PING;
  else if (text == std::string("NICK"))
    return IRC_NICK;
  else if (text == std::string("JOIN"))
    return IRC_JOIN;
  else if (text == std::string("PART"))
    return IRC_PART;
  else if (text == std::string("KICK"))
    return IRC_KICK;
  else if (text == std::string("INVITE"))
    return IRC_INVITE;
  else if (text == std::string("NOTICE"))
    return IRC_NOTICE;
  else if (text == std::string("PRIVMSG"))
    return IRC_PRIVMSG;
  else if (text == std::string("WALLOPS"))
    return IRC_WALLOPS;
  else if (text == std::string("ERROR"))
    return IRC_ERROR;
  else
    return IRC_UNKNOWN;
}


unsigned long
atoul(const char *text)
{
  unsigned long temp = 0;
  while ((*text >= '0') && (*text <= '9'))
    temp = temp * 10 + (*(text++) - '0');
  return temp;
}


std::string
FirstWord(std::string & Text)
{
  std::string temp;
  std::string::size_type i = Text.find(' ');
  if (std::string::npos != i)
  {
    temp = Text.substr(0, i);
    Text = Text.substr(i + 1);
  }
  else
  {
    temp = Text;
    Text = "";
  }
  while ((Text.length() > 0) && (Text[0] == ' '))
    Text.erase((std::string::size_type) 0, 1);
  return temp;
}


char
UpCase(const char c)
{
	return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}


std::string
UpCase(const std::string & Text)
{
  std::string result = Text;
  std::string::size_type len = result.length();

  for (std::string::size_type index = 0; index < len; index++)
  {
	result[index] = UpCase(result[index]);
  }

  return result;
}


char
DownCase(const char c)
{
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}


std::string
DownCase(const std::string & Text)
{
  std::string result = Text;
  std::string::size_type len = result.length();

  for (std::string::size_type index = 0; index < len; index++)
  {
	result[index] = DownCase(result[index]);
  }

  return result;
}


bool
Same(const std::string & Text1, const std::string & Text2)
{
  return (UpCase(Text1) == UpCase(Text2));
}


bool
Same(const std::string & Text1, const std::string & Text2,
  const std::string::size_type length)
{
  if ((length > Text1.length()) || (length > Text2.length()))
  {
    return false;
  }

  return (UpCase(Text1.substr(0, length)) == UpCase(Text2.substr(0, length)));
}


bool
ChkPass(std::string CORRECT, std::string TEST)
{
  if (CORRECT == "*")
    return false;
  if ((CORRECT == "") && (TEST == ""))
    return true;
#ifdef USE_CRYPT
  if (CORRECT == crypt(TEST.c_str(), CORRECT.c_str()))
    return true;
#else
  return (CORRECT == TEST);
#endif
  return false;
}


std::string
timeStamp(const TimeStampFormat format)
{
  char ts_holder[MAX_BUFF];
  struct tm *time_val;
  time_t t;
  
  t = time(NULL);
  time_val = localtime(&t);

  switch (format)
  {
    case TIMESTAMP_KLINE:
#ifdef HAVE_SNPRINTF
      snprintf(ts_holder, sizeof(ts_holder),
#else
      sprintf(ts_holder,
#endif
	"%d/%d/%d %2.2d.%2.2d", time_val->tm_year + 1900, time_val->tm_mon + 1,
	time_val->tm_mday, time_val->tm_hour, time_val->tm_min);
	break;
    case TIMESTAMP_LOG:
    default:
#ifdef HAVE_SNPRINTF
      snprintf(ts_holder, sizeof(ts_holder),
#else
      sprintf(ts_holder,
#endif
	"%d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", time_val->tm_year + 1900,
	time_val->tm_mon + 1, time_val->tm_mday, time_val->tm_hour,
	time_val->tm_min, time_val->tm_sec);
      break;
  }

  return std::string(ts_holder);
}


std::string
IntToStr(int i, std::string::size_type len)
{
  if (i < 0)
  {
    return std::string("-") + IntToStr(-i, (len > 1) ? (len - 1) : 0);
  }
  else
  {
    std::string result;

    if (i == 0)
    {
      result = "0";
    }
    else
    {
      while (i > 0)
      {
        result.insert(result.begin(), 1,
	  (std::string::value_type)((i % 10) + '0'));
        i /= 10;
      }
    }

    std::string::size_type l = result.length();
    if (l < len)
    {
      result.insert(result.begin(), len - l, ' ');
    }
    return result;
  }
}


std::string
ULongToStr(unsigned long i, std::string::size_type len)
{
  std::string result;

  if (i == 0)
  {
    result = "0";
  }
  else
  {
    while (i != 0)
    {
      result.insert(result.begin(), 1,
	(std::string::value_type)((i % 10) + '0'));
      i /= 10;
    }
  }

  std::string::size_type l = result.length();
  if (l < len)
  {
    result.insert(result.begin(), len - l, ' ');
  }
  return result;
}


std::string
GetBotHB(std::string text)
{
  if (text == "*")
    return text;

  std::string::size_type i = text.find('@');
  if (i != std::string::npos)
    return text.substr(i + 1);
  else
    return "";
}


std::string
GetHandleHB(std::string text)
{
  if (text == "*")
    return text;

  std::string::size_type i = text.find('@');
  if (i != std::string::npos)
    return text.substr(0, i);
  else
    return "";
}


std::string::size_type
CountChars(const std::string & text, const std::string::value_type ch)
{
  std::string::size_type length = text.length();
  std::string::size_type count = 0;

  for (std::string::size_type index = 0; index < length; index++)
  {
    if (text[index] == ch)
    {
      count++;
    }
  }

  return count;
}


bool
isIP(const std::string & host)
{
  return ((std::string::npos == host.find_first_not_of(".0123456789")) &&
    (3 == CountChars(host, '.')));
}


std::string
getDomain(std::string host, bool withDot)
{
  if (isIP(host))
  {
    // 1.2.3.4
    std::string::size_type lastDot = host.rfind('.');
    if (std::string::npos != lastDot)
    {
      return (host.substr(0, lastDot + (withDot ? 1 : 0)));
    }
    else
    {
      // This should never happen
      return host;
    }
  }
  else
  {
    std::string::size_type lastDot = host.rfind('.');
    if (std::string::npos == lastDot)
    {
      // domain
      return host;
    }
    else
    {
      std::string::size_type prevDot = host.rfind('.', lastDot - 1);
      if (std::string::npos == prevDot)
      {
        // domain.tld
        return host;
      }
      else
      {
        // *.domain.tld
        std::string tld = host.substr(lastDot + 1, std::string::npos);
        if (tld.length() == 2)
        {
          // *.domain.xx
          std::string::size_type triDot = host.rfind('.', prevDot - 1);
          if (std::string::npos == triDot)
          {
            // user@foo.domain.xx
            if (std::string::npos == host.substr(0,
              prevDot).find_first_of("0123456789"))
            {
              // alpha.domain.xx
	      return (host);
	    }
	    else
	    {
              // al55pha.domain.xx
	      return (host.substr(prevDot + (withDot ? 0 : 1),
		std::string::npos));
	    }
	  }
	  else
	  {
	    // *.foo.domain.xx
	    if (std::string::npos == host.substr(triDot + 1,
              prevDot - triDot - 1).find_first_of("0123456789"))
            {
              // *.alpha.domain.xx
	      return (host.substr(triDot + (withDot ? 0 : 1),
		std::string::npos));
	    }
	    else
	    {
              // *.al55pha.domain.xx
	      return (host.substr(prevDot + (withDot ? 0 : 1),
		std::string::npos));
	    }
	  }
	}
	else
	{
	  // *.domain.tld
	  return (host.substr(prevDot + (withDot ? 0 : 1), std::string::npos));
	}
      }
    }
  }
}


std::string klineMask(const std::string & userhost)
{
  std::string::size_type at = userhost.find('@');
  if (at != std::string::npos)
  {
    std::string user = userhost.substr(0, at);
    std::string host = userhost.substr(at + 1, std::string::npos);

    if ((user.length() > 0) && (user[0] == '~'))
    {
      user = "~*";
    }
    else if (user.length() > 8)
    {
      user = "*" + user.substr(user.length() - 8);
    }
    else
    {
      user = "*" + user;
    }

    std::string domain = getDomain(host, true);

    if (isIP(host))
    {
      return (user + "@" + domain + "*");
    }
    else
    {
      return (user + "@*" + domain);
    }
  }
  else
  {
    return userhost;
  }
}

std::string classCMask(const std::string & ip)
{
  std::string::size_type lastDot = ip.rfind('.');

  return (ip.substr(0, lastDot) + ".*");
}

std::string
StrJoin(char token, const StrVector & items)
{
  std::string result;

  for (StrVector::size_type index = 0; index < items.size(); index++)
  {
    if (index == 0)
      result = items[index];
    else
      result += " " + items[index];
  }

  return result;
}


bool
isNumeric(const std::string & text)
{
  for (std::string::size_type pos = 0; pos < text.length(); ++pos)
  {
    if (!isdigit(text[pos]))
    {
      return false;
    }
  }
  return true;
}


//////////////////////////////////////////////////////////////////////
// isDynamic(user, host)
//
// Description:
//  Given a user and hostname, this function will determine whether
//  the client is connecting from a dynamic host.  For now, this uses
//  a VERY simple algorithm.  If the client is not running identd or
//  the hostname contains numeric characters, it is considered
//  dynamic.  This works about 90% of the time.  There are problems,
//  however, as AOL dialup hostnames use the form foo.ipt.aol.com,
//  where "foo" is a hexadecimal representation of the IP address.
//  Since hex includes alpha characters, an AOL dialup hostname can
//  be completely without numeric characters (I've seen it).
//
//  Eventually, I'll rewrite this so that users can specify patterns
//  to match against that determine whether a hostname is dynamic.
//
// Parameters:
//  user - a client's username
//  host - a client's hostname
//
// Return value:
//  true - the client is connecting from a dynamic host
//  false - the client is NOT connecting from a dynamic host
//////////////////////////////////////////////////////////////////////
bool
isDynamic(const std::string & user, const std::string & host)
{
  bool result = false;

  if (((user.length() > 0) && (user[0] == '~')) ||
    (std::string::npos != host.find_first_of("0123456789")))
  {
    result = true;
  }

  return result;
}


std::string
timeDiff(time_t diff)
{
  if (diff == 0)
 {
    	return "none";
  }

  std::string result;
  int amount;

  // Seconds
  amount = diff % 60;
  if (amount != 0)
  {
    result = IntToStr(amount) + "s";
  }
  diff /= 60;

  // Minutes
  amount = diff % 60;
  if (amount != 0)
  {
    result = IntToStr(amount) + "m" + result;
  }
  diff /= 60;

  // Hours
  amount = diff % 24;
  if (amount != 0)
  {
    result = IntToStr(amount) + "h" + result;
  }
  diff /= 24;

  // Days
  amount = diff % 7;
  if (amount != 0)
  {
    result = IntToStr(amount) + "d" + result;
  }
  diff /= 7;

  // Weeks
  if (diff != 0)
  {
    result = IntToStr(diff) + "w" + result;
  }

  return result;
}


std::string
trimLeft(const std::string & text)
{
  std::string::size_type pos = text.find_first_not_of(" \t");

  if (pos == std::string::npos)
  {
    return "";
  }

  return text.substr(pos);
}


std::string
hexDump(const void *buffer, const int size)
{
  const unsigned char *copy = reinterpret_cast<const unsigned char *>(buffer);
  std::string result;

  if (copy != NULL)
  {
    for (int i = 0; i < size; ++i)
    {
      char *nibble = "0123456789ABCDEF";

      if (result.length() > 0)
      {
        result += " ";
      }
      result += nibble[copy[i] >> 4];
      result += nibble[copy[i] & 15];
    }
  }

  return result;
}


// chopUserhost(NUH)
//
// Takes the user@host out of a nick[user@host]
// Sorry, I'm too lazy to code the other formats :P
//
std::string
chopUserhost(std::string NUH)
{
  if (NUH != "")
  {
    std::string::size_type pos = NUH.find('[');
    if (pos != std::string::npos)
    {
      if (CountChars(NUH, '[') > 1)
      {
	// Moron has '[' characters in his nick or username
	// Look for '~' to mark the start of an non-ident'ed
	// username...
	std::string::size_type tilde = NUH.find("[~");
	if (tilde != std::string::npos)
	{
	  NUH = NUH.substr(tilde + 1);
	}
	else
	{
	  // Blah. Loser is running identd with a screwy
	  // username or nick.
	  // Find the last '[' in the string and assume it is
	  // the divider, unless this produces an illegal
	  // length nick :P
	  int nicklen = 0;
	  while ((nicklen <= 9) && ((pos = NUH.find('[')) != std::string::npos))
	  {
	    NUH = NUH.substr(1);
	    nicklen++;
	  }
	}
      }
      else
      {
        NUH = NUH.substr(pos + 1);
      }
      // Remove the trailing ']'
      NUH.erase(NUH.length() - 1, 1);
      return NUH;
    }
  }
  return "";
}


// chopNick(NUH)
//
// Takes the nick out of a nick[user@host]
//
std::string
chopNick(std::string NUH)
{
  std::string::size_type pos;
  if (CountChars(NUH, '[') == 1)
  {
    pos = NUH.find('[');
    if (pos != std::string::npos)
    {
      return NUH.substr(0, pos);
    }
  }
  if (CountChars(NUH, '[') > 1)
  {
    if ((pos = NUH.find("[~")) != std::string::npos)
    {
      return NUH.substr(0, pos);
    }
    int nicklen = 0;
    while (nicklen < 9)
    {
      if ((NUH[nicklen] == '[') && (CountChars(NUH.substr(nicklen), '[') == 1))
      {
	break;
      }
      nicklen++;
    }
    return NUH.substr(0, nicklen);
  }
  return "";
}


//////////////////////////////////////////////////////////////////////
// expandPath(filename, cwd)
//
// Description:
//  This function determines whether a filename contains an absolute
//  path or if it is located relative to a working directory.
//
// Parameters:
//  filename - a filename
//  cwd      - the current working directory
//
// Return value:
//  The function returns the full pathname of the desired file.
//////////////////////////////////////////////////////////////////////
std::string
expandPath(const std::string & filename, const std::string & cwd)
{
  std::string result;

  if ((filename.length() > 0) && (filename[0] != '/'))
  {
    result = cwd + '/' + filename;
  }
  else
  {
    result = filename;
  }

  return result;
}

