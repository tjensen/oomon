#ifndef __OOMON_H__
#define __OOMON_H__
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

// ======================================================================
//
//                               PLEASE READ
//
//   You may have noticed this file has changed significantly since
//   OOMon-1.*.  That's because OOMon now has a ".set" command, which
//   allows you to change the bot's settings on the fly.
//
//   Remember to use the ".save" command to save your custom settings!
//
// ======================================================================


#include "defs.h"

#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif


// This is the configuration file
#define DEFAULT_CFGFILE		ETCDIR "/oomon.cf"

// Where to store the process id number
#define DEFAULT_PIDFILE		ETCDIR "/oomon.pid"

// Default help filename
#define DEFAULT_HELPFILE	"oomon.help"

// Default MOTD filename
#define DEFAULT_MOTDFILE	"oomon-motd.txt"

// Default log filename
#define DEFAULT_LOGFILE		"oomon.log"

// Default user settings filename
#define DEFAULT_USERDBFILE	"oomon-users.db"

// Default settings filename
#define DEFAULT_SETTINGSFILE	"oomon.settings"


// OOMon will listen on this port for bot linking
#define DEFAULT_PORT		4000


// I like to fork.  Do you like to fork?  Everybody likes to fork!
// Not recommended for use when debugging.
#ifndef DEBUG
# define FORK_OOMON
#endif

// Define this if you want to use encrypted passwords
#define USE_CRYPT

// Define this if you are using comstud's new ircd
#undef USE_FLAGS


// Define this to detect vhost clones
#define DETECT_VHOST_CLONES

// Netmask used for detecting vhosts (warning: do not change!)
#define VHOST_NETMASK "255.255.255.0"

#define MAX_IP_CONNECTS 5
#define MAX_IP_TIME 10


// Use services.ca style services (BROKEN -- DO NOT USE)
#undef CA_SERVICES
#define CA_SERVICES_REQUEST 	"CA-SVCS"
#define CA_SERVICES_RESPONSE 	"services.ca"









//////////////////////////////////////////////////////////////////////
//  Do not modify anything below this point
//////////////////////////////////////////////////////////////////////

// This is the version string :P
#define OOMON_VERSION		"2.1-DEV"

// Maximum buffer size
#define MAX_BUFF		1024

// Maximum hostname length
#define MAX_HOST		256

// Number of seconds to wait until timeout
#define SERVER_TIME_OUT		300

// Number of seconds to wait before disconnecting an idle, unauthorized DCC
// connection
#define DCC_IDLE_MAX		60

// Number of seconds to wait before timing out a proxy connection
#define PROXY_IDLE_MAX		300

#define INVALID			(-1)

#define BOLD_TOGGLE		'\002'

#ifdef HAVE_WHOIP
#undef HAVE_USRIP
#endif

#ifndef HAVE_USRIP
 #ifndef HAVE_WHOIP
  #define HAVE_STATSL
 #endif
#endif

#if !defined(HAVE_CRYPT) && defined(USE_CRYPT)
# undef USE_CRYPT
# warning "Couldn't find crypt()!  Using plaintext passwords instead."
#endif

#endif /* __OOMON_H__ */

