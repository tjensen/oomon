#ifndef __LINKS_H__
#define __LINKS_H__
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
#include <list>


struct BotLink
{
  BotLink(const std::string & nodeA_, const std::string & nodeB_)
    : nodeA(nodeA_), nodeB(nodeB_) { };

  std::string nodeA;
  std::string nodeB;
};

typedef std::list<BotLink> BotLinkList;

class Links
{
private:
  std::string Name;		// "Root" server's name
  std::list<Links> Children;	// Linked children
public:
  Links();			// Constructor
  Links(const std::string &);	// Constructor
  virtual ~Links();		// Destructor
  void Clear();			// Clear everything
  bool exists(const std::string &) const;	// Is server somewhere
					// in the tree?
  bool Link(const std::string &, const std::string &);	// Connect a
					// server somewhere in
					// a tree
  bool Unlink(const std::string &);	// Disconnect a server from
				// the tree
  void setName(const std::string &);	// Set the root's name

  std::string getName() const	// Get the root's name
  {
    return Name;
  }

  void getLinks(class BotClient * client, const std::string &) const;
  void getBotList(BotLinkList &) const;
};

#endif /* __LINKS_H__ */

