#ifndef __SERVICES_H__
#define __SERVICES_H__
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

// Std C++ Headers
#include <string>

// Std C Headers
#include <time.h>

// OOMon Headers


class Services
{
public:
  Services() { };

  void check();
  void pollSpamtrap();
  void onXoNotice(std::string Text);
  void onCaNotice(std::string Text);
  void onSpamtrapMessage(const std::string & text);
  void onSpamtrapNotice(const std::string & text);
  void onIson(const std::string & text);

private:
  // XO services parameters
  std::string cloningUserhost;
  int cloneCount;
  bool suggestKline;

  time_t lastCheckedTime;
  bool gettingCaReports;
};


extern Services services;


#endif /* __SERVICES_H__ */

