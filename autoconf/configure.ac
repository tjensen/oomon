# configure.in for oomon
# thanks to the ircii configure.in for giving me examples to
# work from and steal ;)
#
# $Id$

AC_REVISION($Revision$)
AC_INIT
AC_CONFIG_HEADER(defs.h)
AC_CONFIG_AUX_DIR(autoconf)
AC_LANG_CPLUSPLUS
AC_PREFIX_DEFAULT(/usr/local/oomon)
AC_CANONICAL_HOST

#
# grok the version number - from oomon.h
#
VERSION=`sed -n -e 's/"$//' -e '/#define OOMON_VERSION	*"/s///p' -e '/#define OOMON_VERSION/q' < oomon.h` 
echo this is OOMon version $VERSION
echo
AC_SUBST(VERSION)

#
# initial stuff
#
AC_PROG_CXX
AC_PROG_CXXCPP
AC_ISC_POSIX
AC_PROG_MAKE_SET
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(LN, ln)
AC_PATH_PROG(MKDIR, mkdir)
AC_PATH_PROG(TOUCH, touch)
AC_PROG_INSTALL

# use directory structure of cached as default (hack)
if test "$libexecdir" = '${exec_prefix}/libexec' &&
   test "$localstatedir" = '${prefix}/var'; then
        libexecdir='${bindir}'
        localstatedir='${prefix}'
fi


#
# If we're running GNU C++, add helpful command line arguments, including
# -ftemplate-depth-20 if it's an old version.
#
if test "$GXX" = yes; then
	# Grab G++ version
	gxx_version=`${CXX} -dumpversion`

	CXXFLAGS="$CXXFLAGS -Wall"

	case $gxx_version in
	  2.9*)	CXXFLAGS="$CXXFLAGS -ftemplate-depth-20";;
	esac
fi


# AC_PROG_LEX
# AC_PROG_GCC_TRADITIONAL
AC_CHECK_LIB(sun, getpwnam)


AC_MSG_CHECKING(for solaris)
if /bin/sun 2> /dev/null
then
  uname=`uname -r`
  case "$uname" in
    4.*)
      AC_MSG_RESULT(no) 
      sunos4=1
      ;;
    5.*)
      AC_MSG_RESULT(yes)
      AC_DEFINE(__solaris__, 1, [define this if you are on solaris 2.x])
      svr4=1
      ;;
  esac
else
  AC_MSG_RESULT(no)
  AC_MSG_CHECKING(for SVR4)
  AC_EGREP_CPP(yes,
[#if defined(SVR4) || defined(__svr4__)
  yes
#endif
],svr4=1)
fi

if test -n "$svr4"
then
  AC_MSG_RESULT(yes)
  LDFLAGS="-L/usr/ccs/lib $LDFLAGS"
  AC_DEFINE(SVR4, 1, [define this if you are on a svr4 derivative])
else
  if test ! -n "$sunos4"
  then
    AC_MSG_RESULT(no)
  fi
fi

#
# testing for svr3/wollengong tcp's /usr/netinclude
#
AC_MSG_CHECKING(for -I/usr/netinclude)
if test -d /usr/netinclude; then
  AC_MSG_RESULT(yes)
  CFLAGS="$CFLAGS -I/usr/netinclude"
else
  AC_MSG_RESULT(no)
fi

# sys/ ones
AC_CHECK_HEADERS(sys/un.h sys/select.h sys/fcntl.h sys/file.h sys/time.h sys/wait.h sys/twg_config.h sys/ptem.h)
# non sys/ ones
AC_CHECK_HEADERS(stdarg.h unistd.h fcntl.h string.h memory.h netdb.h limits.h crypt.h)
AC_CHECK_HEADERS(termbits.h)
AC_CHECK_FUNC(setsid, AC_DEFINE(HAVE_SETSID, 1, [Define if your system has a setsid() function.]),)
AC_CHECK_FUNC(getpgid, AC_DEFINE(HAVE_GETPGID, 1, [Define if your system has a getpgid() function.]),)
AC_CHECK_FUNC(getsid, AC_DEFINE(HAVE_GETSID, 1, [Define if your system has a getsid() function.]),)
AC_CHECK_FUNC(scandir, AC_DEFINE(HAVE_SCANDIR, 1, [Define if your system has a scandir() function.]),)
AC_CHECK_FUNC(memmove, AC_DEFINE(HAVE_MEMMOVE, 1, [Define if your system has a memmove() function.]),)
AC_CHECK_FUNC(uname, AC_DEFINE(HAVE_UNAME, 1, [Define if your system has a uname() function.]),)
AC_CHECK_FUNC(getcwd, AC_DEFINE(HAVE_GETCWD, 1, [Define if your system has a getcwd() function.]),)
AC_CHECK_FUNC(fchmod, AC_DEFINE(HAVE_FCHMOD, 1, [Define if your system has a fchmod() function.]),)
AC_CHECK_FUNC(strerror, AC_DEFINE(HAVE_STRERROR, 1, [Define if your system has a strerror() function.]),)
AC_CHECK_FUNC(regcomp, AC_DEFINE(HAVE_POSIX_REGEX, 1, [Define if using POSIX regular expressions.]),)
AC_CHECK_FUNC(snprintf, AC_DEFINE(HAVE_SNPRINTF, 1, [Define if your system has a snprintf() function.]),)
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_TYPE_SIGNAL
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T

# PCRE support
AC_MSG_CHECKING(for PCRE)
AC_ARG_ENABLE(pcre,
[  --enable-pcre[=DIR]     Enable PCRE support (DIR optional).
  --disable-pcre          Disable PCRE support.],
[cf_enable_pcre=$enableval],
[cf_enable_pcre="auto"])

if test "X$cf_enable_pcre" != "Xno" ; then
  cf_pcre_basedir=""
  if test "X$cf_enable_pcre" != "Xauto" &&
     test "X$cf_enable_pcre" != "Xyes" ; then
     # Support for --enable-pcre=/some/place
     cf_pcre_basedir="${cf_enable_pcre}"
  else
     # Do the auto-probe here.  Check some common directory paths.
     for dirs in /usr/local /usr/local/pcre /usr/pkg ; do
       if test -f "${dirs}/include/pcre.h" ; then
	 cf_pcre_basedir="$dirs"
	 break
       fi
     done
     unset dirs
  fi
  # Now check cf_pcre_basedir to see if we found anything.
  if test "X$cf_pcre_basedir" != "X" ; then
    if test -f "${cf_pcre_basedir}/include/pcre.h" ; then
      echo "${CXXFLAGS}" | grep -e "-I${cf_pcre_basedir}/include" > /dev/null 2>&1
      if test $? -ne 0; then
        PCRE_INCLUDES="-I${cf_pcre_basedir}/include"
      fi
      PCRE_LIBS="-lpcre"
      echo "${LIBS}" | grep -e "-L${cf_pcre_basedir}/lib" > /dev/null 2>&1
      if test $? -ne 0; then
        PCRE_LIBS="-L${cf_pcre_basedir}/lib $PCRE_LIBS"
      fi
    else
      # PCRE wasn't found in the directory specified.
      cf_pcre_basedir=""
    fi
  else
    # Check for install in /usr/include and /usr/lib.
    if test -f "/usr/include/pcre.h" ; then
      cf_pcre_basedir="/usr"
      PCRE_LIBS="-lpcre"
    fi
  fi
  # If we have a basedir defined, then everything is okay.  Otherwise,
  # we have a problem.
  if test "X$cf_pcre_basedir" != "X" ; then
    AC_MSG_RESULT(yes ($cf_pcre_basedir))
    cf_enable_pcre="yes"
  else
    AC_MSG_RESULT(not found.  Please check your path.)
    cf_enable_pcre="no"
  fi
  unset cf_pcre_basedir
else
  # If --disable-pcre was specified
  AC_MSG_RESULT(disabled)
fi

if test "X$cf_enable_pcre" != "Xno" ; then
  if test ! -z "$PCRE_INCLUDES"; then
    CXXFLAGS="$CXXFLAGS $PCRE_INCLUDES"
  fi
  if test ! -z "$PCRE_LIBS"; then
    LIBS="$PCRE_LIBS $LIBS"
  fi
  AC_DEFINE(HAVE_LIBPCRE, 1, [Define if using PCRE for regex.])
  PCRE_MSG="yes"
else
  PCRE_MSG="no"
fi

# ADNS support
AC_MSG_CHECKING(for ADNS)
AC_ARG_ENABLE(adns,
[  --enable-adns[=DIR]     Enable ADNS support (DIR optional).
  --disable-adns          Disable ADNS support.],
[cf_enable_adns=$enableval],
[cf_enable_adns="auto"])

if test "X$cf_enable_adns" != "Xno" ; then
  cf_adns_basedir=""
  if test "X$cf_enable_adns" != "Xauto" &&
     test "X$cf_enable_adns" != "Xyes" ; then
     # Support for --enable-adns=/some/place
     cf_adns_basedir="${cf_enable_adns}"
  else
     # Do the auto-probe here.  Check some common directory paths.
     for dirs in /usr/local /usr/local/adns /usr/pkg ; do
       if test -f "${dirs}/include/adns.h" ; then
	 cf_adns_basedir="$dirs"
	 break
       fi
     done
     unset dirs
  fi
  # Now check cf_adns_basedir to see if we found anything.
  if test "X$cf_adns_basedir" != "X" ; then
    if test -f "${cf_adns_basedir}/include/adns.h" ; then
      echo "${CXXFLAGS}" | grep -e "-I${cf_adns_basedir}/include" > /dev/null 2>&1
      if test $? -ne 0; then
        ADNS_INCLUDES="-I${cf_adns_basedir}/include"
      fi
      ADNS_LIBS="-ladns"
      echo "${LIBS}" | grep -e "-L${cf_adns_basedir}/lib" > /dev/null 2>&1
      if test $? -ne 0; then
        ADNS_LIBS="-L${cf_adns_basedir}/lib $ADNS_LIBS"
      fi
    else
      # ADNS wasn't found in the directory specified.
      cf_adns_basedir=""
    fi
  else
    # Check for install in /usr/include and /usr/lib.
    if test -f "/usr/include/adns.h" ; then
      cf_adns_basedir="/usr"
      ADNS_LIBS="-ladns"
    fi
  fi
  # If we have a basedir defined, then everything is okay.  Otherwise,
  # we have a problem.
  if test "X$cf_adns_basedir" != "X" ; then
    AC_MSG_RESULT(yes ($cf_adns_basedir))
    cf_enable_adns="yes"
  else
    AC_MSG_RESULT(not found.  Please check your path.)
    cf_enable_adns="no"
  fi
  unset cf_adns_basedir
else
  # If --disable-adns was specified
  AC_MSG_RESULT(disabled)
fi

if test "X$cf_enable_adns" != "Xno" ; then
  if test ! -z "$ADNS_INCLUDES"; then
    CXXFLAGS="$CXXFLAGS $ADNS_INCLUDES"
  fi
  if test ! -z "$ADNS_LIBS"; then
    LIBS="$LIBS $ADNS_LIBS"
  fi
  AC_DEFINE(HAVE_LIBADNS, 1, [Define if using Asynchronous DNS library.])
  ADNS_MSG="yes"
else
  ADNS_MSG="no"
fi

# GDBM support
AC_MSG_CHECKING(for GDBM)
AC_ARG_ENABLE(gdbm,
[  --enable-gdbm[=DIR]     Enable GDBM support (DIR optional).
  --disable-gdbm          Disable GDBM support.],
[cf_enable_gdbm=$enableval],
[cf_enable_gdbm="auto"])

if test "X$cf_enable_gdbm" != "Xno" ; then
  cf_gdbm_basedir=""
  if test "X$cf_enable_gdbm" != "Xauto" &&
     test "X$cf_enable_gdbm" != "Xyes" ; then
     # Support for --enable-gdbm=/some/place
     cf_gdbm_basedir="${cf_enable_gdbm}"
  else
     # Do the auto-probe here.  Check some common directory paths.
     for dirs in /usr/local /usr/local/gdbm /usr/pkg ; do
       if test -f "${dirs}/include/gdbm.h" ; then
	 cf_gdbm_basedir="$dirs"
	 break
       fi
     done
     unset dirs
  fi
  # Now check cf_gdbm_basedir to see if we found anything.
  if test "X$cf_gdbm_basedir" != "X" ; then
    if test -f "${cf_gdbm_basedir}/include/gdbm.h" ; then
      echo "${CXXFLAGS}" | grep -e "-I${cf_gdbm_basedir}/include" > /dev/null 2>&1
      if test $? -ne 0; then
        GDBM_INCLUDES="-I${cf_gdbm_basedir}/include"
      fi
      GDBM_LIBS="-lgdbm"
      echo "${LIBS}" | grep -e "-L${cf_gdbm_basedir}/lib" > /dev/null 2>&1
      if test $? -ne 0; then
        GDBM_LIBS="-L${cf_gdbm_basedir}/lib $GDBM_LIBS"
      fi
    else
      # GDBM wasn't found in the directory specified.
      cf_gdbm_basedir=""
    fi
  else
    # Check for install in /usr/include and /usr/lib.
    if test -f "/usr/include/gdbm.h" ; then
      cf_gdbm_basedir="/usr"
      GDBM_LIBS="-lgdbm"
    fi
  fi
  # If we have a basedir defined, then everything is okay.  Otherwise,
  # we have a problem.
  if test "X$cf_gdbm_basedir" != "X" ; then
    AC_MSG_RESULT(yes ($cf_gdbm_basedir))
    cf_enable_gdbm="yes"
  else
    AC_MSG_RESULT(not found.  Please check your path.)
    cf_enable_gdbm="no"
  fi
  unset cf_gdbm_basedir
else
  # If --disable-gdbm was specified
  AC_MSG_RESULT(disabled)
fi

if test "X$cf_enable_gdbm" != "Xno" ; then
  if test ! -z "$GDBM_INCLUDES"; then
    CXXFLAGS="$CXXFLAGS $GDBM_INCLUDES"
  fi
  if test ! -z "$GDBM_LIBS"; then
    LIBS="$LIBS $GDBM_LIBS"
  fi
  AC_DEFINE(HAVE_LIBGDBM, 1, [Define if using GDBM style DB.])
  GDBM_MSG="yes"
else
  GDBM_MSG="no"
fi

AC_MSG_CHECKING(for BSD style DB)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <db.h>],
[DB *foo = dbopen("foo.db", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, DB_HASH, NULL);],
AC_DEFINE(HAVE_BSDDB, 1, [Define if using BSD style DB.]) AC_MSG_RESULT(yes),
AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether socklen_t is defined)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
  [socklen_t foo = 0;], AC_MSG_RESULT(yes), AC_DEFINE(socklen_t, int,
  [Define as 'int' if the socklen_t type doesn't exist.])
  AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether in_addr_t is defined)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
  [in_addr_t foo = 0;], AC_MSG_RESULT(yes), AC_DEFINE(in_addr_t, unsigned int,
  [Define as 'unsigned int' if the in_addr_t type doesn't exist.])
  AC_MSG_RESULT(no))

AC_MSG_CHECKING(whether in_port_t is defined)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
  [in_port_t foo = 0;], AC_MSG_RESULT(yes), AC_DEFINE(in_port_t, unsigned short,
  [Define as 'unsigned short' if the in_port_t type doesn't exist.])
  AC_MSG_RESULT(no))


#
# check for pthreads
#
# ACX_PTHREAD(
#	[
#		LIBS="${PTHREAD_LIBS} $LIBS"
#		CXXFLAGS="$CXXFLAGS ${PTHREAD_CFLAGS}"
#	], AC_MSG_ERROR([No pthreads? Uh-oh!]))


#
# checking for select()
#
olibs="$LIBS"
AC_CHECK_FUNC(select, ,
  AC_CHECK_LIB(socket, select, LIBS="$LIBS -lsocket",
    AC_CHECK_LIB(nsl, select, LIBS="$LIBS -lnsl",
      AC_CHECK_LIB(inet, select, LIBS="$LIBS -linet",
        AC_CHECK_LIB(cposix, select, LIBS="$LIBS -lcposix",
          AC_CHECK_LIB(net, select, LIBS="$LIBS -lnet",
            AC_MSG_WARN(i can not find select.  you might need to help me)))))))

#
# need this for dcc stuff
#
AC_MSG_CHECKING(for a 32 bit integer)
AC_TRY_RUN([
main()
{
  if (4 == sizeof(unsigned long))
    exit(0);
  exit(1);
}
],
  AC_MSG_RESULT(unsigned long)
  AC_DEFINE(UNSIGNED_LONG32, 1, [define this if an unsigned long is 32 bits]),
  AC_TRY_RUN([
main()
{
  if (4 == sizeof(unsigned int))
    exit(0);
  exit(1);
}
],
  AC_MSG_RESULT(unsigned int),
  AC_DEFINE(UNSIGNED_INT32, 1, [define this if an unsigned int is 32 bits]),
  AC_MSG_RESULT(none, using unsigned long)
  AC_DEFINE(UNKNOWN_32INT, 1, [define this if you are unsure what is is 32 bits])), :)

#
# libraries
#

# Standard C++ library
AC_CHECK_LIB(stdc++, main)

# encryption is a good thing
have_crypt=0
AC_CHECK_LIB(c, crypt, [have_crypt=1],
	AC_CHECK_LIB(crypt, crypt, [have_crypt=1; LIBS="-lcrypt $LIBS"],
		AC_CHECK_LIB(descrypt, crypt, [have_crypt=1; LIBS="-ldescrypt $LIBS"],
			AC_MSG_WARN(Bummer.  You're stuck with plaintext passwords.  Loser.)
		)
	)
)
if test "$have_crypt" = "1"; then
	AC_DEFINE(HAVE_CRYPT, 1, [define this if you have crypt(3)])
fi

# i think this was only needed for solaris with -lucb.
# AC_HAVE_LIBRARY(elf, LIBS="$LIBS -lelf",)
# AC_HAVE_LIBRARY(nls, libnls=1;AC_DEFINE(HAVE_LIB_NLS, 1, [define this if -lnls exists]),)
if test ! -n "$libsocket"; then
	AC_CHECK_LIB(socket, socket, LIBS="$LIBS -lsocket",)
fi
# this is for isc.  need the nsl_s library as well.
if test ! -n "$libinet"; then
	AC_CHECK_LIB(inet, socket, libnsl=1; LIBS="$LIBS -linet -lnsl_s",)
fi
if test ! -n "$libnsl"; then
	AC_CHECK_LIB(nsl, gethostname, LIBS="$LIBS -lnsl",)
fi
AC_CHECK_LIB(dgc, inet_addr, LIBS="$LIBS -ldgc",)
AC_CHECK_LIB(resolv, gethostbyname,
  olibs=$LIBS
  LIBS="-lresolv $LIBS"
  AC_TRY_RUN([
#include <netdb.h>
main()
{
  struct hostent *hp;

  hp = gethostbyname("foo");
  exit(0);
}
],,LIBS=$olibs, :))
# AC_HAVE_LIBRARY(PW, LIBS="$LIBS -lPW",)

if test -z "$CFLAGS"; then CFLAGS=-O ; fi
if test -z "$LDFLAGS"; then LDFLAGS= ; fi
RM="rm -f"

AC_SUBST(RM)
AC_OUTPUT(makefile)

A=`eval echo ${prefix}` ; A=`eval echo ${A}`
B=`eval echo ${bindir}` ; B=`eval echo ${B}`
C=`eval echo ${sbindir}` ; C=`eval echo ${C}`
D=`eval echo ${sysconfdir}` ; D=`eval echo ${D}`
E=`eval echo ${libexecdir}/ssh-askpass` ; E=`eval echo ${E}`
F=`eval echo ${mandir}/${mansubdir}X` ; F=`eval echo ${F}`
G=`eval echo ${piddir}` ; G=`eval echo ${G}`
H=`eval echo ${PRIVSEP_PATH}` ; H=`eval echo ${H}`
I=`eval echo ${user_path}` ; I=`eval echo ${I}`
J=`eval echo ${superuser_path}` ; J=`eval echo ${J}`
K=`eval echo ${localstatedir}/logs` ; K=`eval echo ${K}`

echo ""
echo "OOMon has been configured with the following options:"
echo ""
echo "                                    Binaries: $B"
echo "                         Configuration files: $D"
echo "                                   Log files: $K"
echo "  Perl-Compatible Regular Expressions (PCRE): ${PCRE_MSG}"
echo "                 GNU Database Manager (GDBM): ${GDBM_MSG}"
echo "             Asynchronous DNS Library (ADNS): ${ADNS_MSG}"
echo ""
echo "            Compiler: ${CXX}"
echo "      Compiler flags: ${CXXFLAGS}"
echo "        Linker flags: ${LDFLAGS}"
echo "           Libraries: ${LIBS}"

echo ""
