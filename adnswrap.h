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

#ifndef __ADNSWRAP_H__
#define __ADNSWRAP_H__

// Std C++ Headers
#include <string>

// OOMon Headers
#include "botsock.h"

#ifdef HAVE_LIBADNS
# include <adns.h>
#endif /* HAVE_LIBADNS */


class Adns
{
public:
  Adns(void);
  ~Adns(void);

#ifdef HAVE_LIBADNS
  int submit(const std::string & name, adns_query & query);
  int submit_reverse(const BotSock::Address & addr, adns_query & query);
  int submit_rbl(const BotSock::Address & addr, const std::string & zone,
    adns_query & query);

  int check(adns_query & query, adns_answer * & answer);
#endif /* HAVE_LIBADNS */

  void beforeselect(int & maxfd, fd_set & readfds, fd_set & writefds,
    fd_set & exceptfds, struct timeval * & tv_mod);
  void afterselect(int maxfd, const fd_set & readfds, const fd_set & writefds,
    const fd_set & exceptfds);

private:
#ifdef HAVE_LIBADNS
  adns_state _state;
#endif /* HAVE_LIBADNS */
};


extern Adns adns;


#endif /* __ADNSWRAP_H__ */