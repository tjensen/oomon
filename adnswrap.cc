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

#ifdef HAVE_LIBADNS
# include <adns.h>
#endif /* HAVE_LIBADNS */

#include "adnswrap.h"


Adns adns;


Adns::Adns(void)
{
#ifdef HAVE_LIBADNS
  if (adns_init(&this->_state, adns_if_nosigpipe, 0))
  {
    throw std::string("Unable to initialize adns");
  }
#endif /* HAVE_LIBADNS */
}


Adns::~Adns(void)
{
#ifdef HAVE_LIBADNS
  adns_finish(this->_state);
#endif /* HAVE_LIBADNS */
}


#ifdef HAVE_LIBADNS
int
Adns::submit(const std::string & name, adns_query & query)
{
  return adns_submit(this->_state, name.c_str(), adns_r_addr,
    static_cast<adns_queryflags>(adns_qf_owner | adns_qf_search), 0, &query);
}


int
Adns::submit_reverse(const BotSock::Address & addr, adns_query & query)
{
  struct sockaddr_in saddr;

  memset(&saddr, 0, sizeof(saddr));
  memcpy(&saddr.sin_addr, &addr, sizeof(addr));
  saddr.sin_family = AF_INET;

  return adns_submit_reverse(this->_state,
    reinterpret_cast<struct sockaddr *>(&saddr), adns_r_ptr,
    static_cast<adns_queryflags>(adns_qf_owner), 0, &query);
}


int
Adns::submit_rbl(const BotSock::Address & addr, const std::string & zone,
  adns_query & query)
{
  struct sockaddr_in saddr;

  memset(&saddr, 0, sizeof(saddr));
  memcpy(&saddr.sin_addr, &addr, sizeof(addr));
  saddr.sin_family = AF_INET;

  return adns_submit_reverse_any(this->_state,
    reinterpret_cast<struct sockaddr *>(&saddr), zone.c_str(), adns_r_addr,
    static_cast<adns_queryflags>(adns_qf_owner), 0, &query);
}


int
Adns::check(adns_query & query, adns_answer * & answer)
{
  void *context;

  return adns_check(this->_state, &query, &answer, &context);
}
#endif /* HAVE_LIBADNS */


void
Adns::beforeselect(int & maxfd, fd_set & readfds, fd_set & writefds,
  fd_set & exceptfds, struct timeval * & tv_mod)
{
#ifdef HAVE_LIBADNS
  struct timeval now;

  gettimeofday(&now, 0);

  adns_beforeselect(this->_state, &maxfd, &readfds, &writefds, &exceptfds,
    &tv_mod, 0, &now);
#endif /* HAVE_LIBADNS */
}


void
Adns::afterselect(int maxfd, const fd_set & readfds, const fd_set & writefds,
  const fd_set & exceptfds)
{
#ifdef HAVE_LIBADNS
  struct timeval now;

  gettimeofday(&now, 0);

  adns_afterselect(this->_state, maxfd, &readfds, &writefds, &exceptfds, &now);
#endif /* HAVE_LIBADNS */
}

