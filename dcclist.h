#ifndef __DCCLIST_H__
#define __DCCLIST_H__
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

// Std C++ headers
#include <string>
#include <list>

// OOMon headers
#include "strtype"
#include "oomon.h"
#include "config.h"
#include "dcc.h"
#include "watch.h"


class DCCList
{
public:
  bool connect(const DCC::Address address, const int port,
    const std::string & nick, const std::string & userhost);
  bool listen(const std::string & nick, const std::string & userhost);
  void setAllFD(fd_set & readset, fd_set & writeset);
  void processAll(const fd_set & readset, const fd_set & writeset);
  void sendAll(const std::string & message, const int flags = UF_NONE,
    const WatchSet & watches = WatchSet(), const DCC *exception = NULL);
  void sendAll(const std::string & message, const int flags,
    const Watch watch, const DCC *exception = NULL);
  bool sendTo(const std::string & handle, const std::string & message,
    const int flags = UF_NONE, const WatchSet & watches = WatchSet());
  bool sendTo(const std::string & handle, const std::string & message,
    const int flags, const Watch watch);
  bool sendChat(const std::string & from, std::string message,
    const DCC *exception = (DCC *) 0);
  void who(StrList &);
  void statsP(StrList &);

  void status(StrList & output) const;

  DCCList() { };
  virtual ~DCCList();

private:
  typedef std::list<DCC *> SockList;
  SockList connections;
  SockList listeners;

  DCC *find(const std::string &);

  class SendFilter
  {
  public:
    SendFilter(const std::string & message, const int flags, 
      const WatchSet & watches, const DCC *exception)
      : _message(message), _flags(flags), _watches(watches),
      _exception(exception) { }

    void operator()(DCC *dccClient)
    {
      if (this->_exception != dccClient)
      {
        dccClient->send(_message, _flags, _watches);
      }
    }

  private:
    const std::string _message;
    const int _flags;
    const WatchSet _watches;
    const DCC *_exception;
  };
};


extern DCCList clients;


#endif /* __DCCLIST_H__ */

