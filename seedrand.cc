// $Id$
//
// Ported to C++ by Tim Jensen
//
// Wow, this is really nifty stuff!  -Tim
//

//
// seedrand 0.1
//
// Copyright (c) 2002-2003 Bill Jonus
//
// tcm-hybrid:(seedrand.c,v 1.5 2002/12/29 09:41:20 bill Exp)
//

// Std C++ Headers
#include <iostream>
#include <string>
#include <list>

// Boost C++ Headers
#include <boost/lexical_cast.hpp>

// OOMon Headers
#include "strtype"
#include "oomon.h"
#include "seedrand.h"
#include "util.h"


#ifdef DEBUG
# define SEEDRAND_DEBUG
#endif


const static std::string allChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890\\|^_-{[}]`");
const static std::string consonants("bcdfghjklmnpqrstvwxyzBCDFGHJKLMNPQRSTVWXYZ");
const static std::string leftBrackets("{[");
const static std::string lower("abcdefghijklmnopqrstuvwxyz");
const static std::string numerics("0123456789");
const static std::string rightBrackets("}]");
const static std::string sylableEnd("bdgjkpqxzBDGJKPQXZ");
const static std::string upper("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
const static std::string vowels("aeiouAEIOU");


static bool
isConsonant(char asc)
{
  return (std::string::npos != consonants.find(asc));
}


static bool
isVowel(char asc)
{
  return (std::string::npos != vowels.find(asc));
}


static bool
isLeftBracket(char asc)
{
  return (std::string::npos != leftBrackets.find(asc));
}


static bool
isLower(char asc)
{
  return (std::string::npos != lower.find(asc));
}


static bool
isNumeric(char asc)
{
  return (std::string::npos != numerics.find(asc));
}


static bool
isRightBracket(char asc)
{
  return (std::string::npos != rightBrackets.find(asc));
}


static bool
isSylableEnd(char asc)
{
  return (std::string::npos != sylableEnd.find(asc));
}


static bool
isUpper(char asc)
{
  return (std::string::npos != upper.find(asc));
}


int
seedrandScore(const std::string & text)
{
  std::string::size_type consonantCount = 0;
  std::string::size_type hyphenCount = 0;
  std::string::size_type leftBracketCount = 0;
  std::string::size_type lowerCount = 0;
  std::string::size_type numericCount = 0;
  std::string::size_type rightBracketCount = 0;
  std::string::size_type upperCount= 0;
  std::string::size_type vowelCount = 0;
  std::string::size_type uniqueCount = 0;
  int retval = 0;

  // initialize "uses" vector
  std::vector<int> uses(allChars.length());

  std::string::size_type length = text.length();

  if (length <= 3)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "Cannot perform seeding on such short strings.  Skipping..." <<
      std::endl;
#endif
    return 0;
  }

#ifdef SEEDRAND_DEBUG
  std::cout << "Computing score for " << text << " ..." << std::endl;
#endif
  for (std::string::size_type w = 0; w < length; ++w)
  {
    char ch = text[w];
    char next = ((w + 1) < length) ? text[w + 1] : 0;

    std::string::size_type offset = allChars.find(ch);
    if (std::string::npos == offset)
    {
      std::cerr << "Invalid character '" << ch << "' (asc:" <<
	static_cast<int>(ch) << ") in '" << text << "'.  Skipping..." <<
	std::endl;
      return 0;
    }

    if (++uses[offset] == 1)
      ++uniqueCount;

    if (isConsonant(ch))
    {
      ++consonantCount;
      if (next && isConsonant(next) && isSylableEnd(ch) && (ch != next))
      {
#ifdef SEEDRAND_DEBUG
        std::cout << ch << " and then " << next << "!" << std::endl;
#endif
        if (isLower(ch) && isUpper(next))
        {
#ifdef SEEDRAND_DEBUG
          std::cout << "+400\tsylable ended followed by uppercase consonant" <<
	    std::endl;
#endif
          retval += 400;
        }
        else
        {
#ifdef SEEDRAND_DEBUG
          std::cout << "+800\tsylable ended followed by lowercase consonant" <<
	    std::endl;
#endif
          retval += 800;
        }
      }
    }

    if (isVowel(ch))
      ++vowelCount;

    if (isLeftBracket(ch))
      ++leftBracketCount;

    if (isNumeric(ch))
      ++numericCount;

    if (isRightBracket(ch))
      ++rightBracketCount;

    if (isUpper(ch))
      ++upperCount;

    if (isLower(ch))
      ++lowerCount;

    if (ch == '-')
      ++hyphenCount;

    if ((ch == 'Q' || ch == 'q') && (next != 'u' && next != 'U'))
    {
#ifdef SEEDRAND_DEBUG
      std::cout << "+400\tq without proceeding u" << std::endl;
#endif
      retval += 400;
    }
  }

  if (std::string::npos != text.find("DCC"))
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "-1300\tdcc bot" << std::endl;
#endif
    retval -= 1300;
  }

  if (((float) uniqueCount / length) <= 0.5)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "-750\tsmall range of characters" << std::endl;
#endif
    retval -= 750;
  }

  if (consonantCount == length)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+1000\tall chars consonants" << std::endl;
#endif
    retval += 1000;
  }

  if ((upperCount + numericCount) == length)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+900\tall uppercase and numerics" << std::endl;
#endif
    retval += 900;
  }

  if (numericCount >= (length - 2))
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+750\tall but 1 or 2 numerics" << std::endl;
#endif
    retval += 750;
  }

  if ((hyphenCount > 0) && (numericCount >= 2))
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+700\t1 or more hyphens and 2 or more numerics" << std::endl;
#endif
    retval += 700;
  }

  if (upperCount > lowerCount)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+600\tmore upper than lower case" << std::endl;
#endif
    retval += 600;
  }

  if ((numericCount * 2) >= length)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+600\tmostly numerics" << std::endl;
#endif
    retval += 600;
  }

  if ((vowelCount * 3) < consonantCount)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+550\t3 times as many consonants as vowels" << std::endl;
#endif
    retval += 550;
  }

  if (leftBracketCount != rightBracketCount)
  {
#ifdef SEEDRAND_DEBUG
    std::cout << "+500\tunmatched brackets" << std::endl;
#endif
    retval += 500;
  }

#ifdef SEEDRAND_DEBUG
    std::cout << "Total score: " << retval << std::endl;
#endif
  return retval;
}

