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

// C++ headers
#include <iostream>
#include <string>
#include <list>

#include "oomon.h"
#include "links.h"
#include "util.h"


#ifdef DEBUG
# define LINKS_DEBUG
#endif


Links::Links()
{
  Name = "";
}

Links::Links(const std::string & name)
{
  #ifdef LINKS_DEBUG
    std::cout << "Links(" << name << ')' << std::endl;
  #endif
  Name = name;
}

Links::~Links()
{
  Clear();
}

void Links::Clear()
{
  Name = "";
  Children.clear();
}

bool
Links::exists(const std::string & name) const
{
  if (Same(Name, name))
  {
    return true;
  }
  for (std::list<Links>::const_iterator pos = Children.begin();
    pos != Children.end(); ++pos)
  {
    if (pos->exists(name))
    {
      return true;
    }
  }
  return false;
}

bool Links::Link(const std::string & parent, const std::string & name)
{
  if (Same(Name, parent))
  {
    #ifdef LINKS_DEBUG
      std::cout << "Linking " << name << " to " << Name << std::endl;
    #endif
    Children.push_back(Links(name));
    return true;
  }
  else
  {
    for (std::list<Links>::iterator pos = Children.begin();
      pos != Children.end(); ++pos)
    {
      if (pos->Link(parent, name))
      {
        return true;
      }
    }
  }
  return false;
}

bool Links::Unlink(const std::string & name)
{
  for (std::list<Links>::iterator pos = Children.begin();
    pos != Children.end(); ++pos)
  {
    if (Same(pos->getName(), name))
    {
      #ifdef LINKS_DEBUG
	std::cout << "Unlinking " << pos->getName() << " from " << Name <<
	  std::endl;
      #endif
      Children.erase(pos);
      return true;
    }
  }
  for (std::list<Links>::iterator pos = Children.begin();
    pos != Children.end(); ++pos)
  {
    if (pos->Unlink(name))
    {
      return true;
    }
  }
  return false;
}

void Links::setName(const std::string & name)
{
  #ifdef LINKS_DEBUG
    std::cout << "Changing link name to " << name << std::endl;
  #endif
  Name = name;
}

void
Links::getLinks(StrList & Output, const std::string & prefix) const
{
  Output.push_back(prefix + Name);
  if (Children.size() > 0)
  {
    std::list<Links>::size_type i = 0;
    for (std::list<Links>::const_iterator pos = Children.begin();
      pos != Children.end(); ++pos)
    {
      if (i < Children.size() - 1)
      {
	Output.push_back(prefix + "|\\");
	pos->getLinks(Output, prefix + "| ");
      }
      else
      {
	Output.push_back(prefix + " \\");
	pos->getLinks(Output, prefix + "  ");
      }
      i++;
    }
  }
}


void
Links::getBotList(BotLinkList & list) const
{
  for (std::list<Links>::const_iterator pos = Children.begin();
    pos != Children.end(); ++pos)
  {
    list.push_back(BotLink(this->getName(), pos->getName()));
    pos->getBotList(list);
  }
}

