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
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

// Std C Headers
#include <unistd.h>

// OOMon Headers
#include "oomon.h"


static char saltChars[] =
       "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


static	char	*
makeSalt(char *salt, int size)
{
  std::srand(std::time(NULL));

  for(int i = 0; i < size; ++i)
  {
    salt[i] = saltChars[std::rand() % 64];
  }
  return(salt);
}


int
main(void)
{
#ifdef HAVE_CRYPT
	std::cout << "Plaintext: ";

	std::string plaintext;
	std::getline(std::cin, plaintext);

	char salt[2];
	makeSalt(salt, 2);

	std::cout << "Encrypted: " << ::crypt(plaintext.c_str(), salt) <<
	  std::endl;

	return 0;
#else
	std::cout << "No crypt(3) supported detected!" << std::endl;

	return -1;
#endif
}

