#ifndef __SERVICES_H__
#define __SERVICES_H__
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
#include <ctime>

// OOMon Headers
#include "autoaction.h"


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

  static bool spamtrapEnable(void) { return Services::spamtrapEnable_; }
  static int spamtrapMinScore(void) { return Services::spamtrapMinScore_; }
  static const std::string & spamtrapNick(void)
  {
    return Services::spamtrapNick_;
  }
  static const std::string & spamtrapUserhost(void)
  {
    return Services::spamtrapUserhost_;
  }
  static const std::string & xoServicesResponse(void)
  {
    return Services::xoServicesResponse_;
  }

  static void init(void);

private:
  // XO services parameters
  std::string cloningUserhost;
  int cloneCount;
  bool suggestKline;

  std::time_t lastCheckedTime;
  bool gettingCaReports;

  static int servicesCheckInterval_;
  static int servicesCloneLimit_;
  static AutoAction spamtrapAction_;
  static std::string spamtrapDefaultReason_;
  static bool spamtrapEnable_;
  static int spamtrapMinScore_;
  static std::string spamtrapNick_;
  static std::string spamtrapUserhost_;
  static AutoAction xoServicesCloneAction_;
  static std::string xoServicesCloneReason_;
  static bool xoServicesEnable_;
  static std::string xoServicesRequest_;
  static std::string xoServicesResponse_;
};


extern Services services;


#endif /* __SERVICES_H__ */

