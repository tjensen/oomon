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

// OOMon Headers
#include "oomon.h"
#include "botdb.h"
#include "botexcept.h"

// Conditional Headers
#if defined(HAVE_LIBGDBM)
# include <gdbm.h>
#elif defined(HAVE_BSDDB)
# include <sys/types.h>
# include <limits.h>
# include <db.h>
#else
# include <map>
#endif
#if defined(HAVE_FLOCK)
# include <sys/file.h>
#elif defined(HAVE_LOCKF)
# include <unistd.h>
#endif



#ifdef DEBUG
# define BOTDB_DEBUG
#endif


#if defined(HAVE_LIBGDBM)
#elif defined(HAVE_BSDDB)
int BotDB::errno_ = 0;
#else
#endif


class BotDB::Lock
{
  public:
    Lock(const int fd, const int operation) : fd_(fd)
    {
      if (this->fd_ != -1)
      {
#if defined(HAVE_FLOCK)
        flock(this->fd_, operation);
#elif defined(HAVE_LOCKF)
        lseek(this->fd_, 0, SEEK_SET);
        lockf(this->fd_, operation, 0);
#endif
      }
    }
    ~Lock(void)
    {
      if (this->fd_ != -1)
      {
#if defined(HAVE_FLOCK)
        flock(this->fd_, Lock::UNLOCK);
#elif defined(HAVE_LOCKF)
        lseek(this->fd_, 0, SEEK_SET);
        lockf(this->fd_, Lock::UNLOCK, 0);
#endif
      }
    }
#if defined(HAVE_FLOCK)
    static const int UNLOCK = LOCK_UN;
    static const int SHARED = LOCK_SH;
    static const int EXCLUSIVE = LOCK_EX;
#elif defined(HAVE_LOCKF)
    static const int UNLOCK = F_ULOCK;
    static const int SHARED = F_TLOCK;
    static const int EXCLUSIVE = F_LOCK;
#else
    static const int UNLOCK = 0;
    static const int SHARED = 0;
    static const int EXCLUSIVE = 0;
#endif
  private:
    const int fd_;
};


BotDB::BotDB(const std::string & file, int mode)
{
#if defined(HAVE_LIBGDBM)
  db = gdbm_open(const_cast<char *>(file.c_str()), 0,
      GDBM_WRCREAT | GDBM_NOLOCK | GDBM_SYNC, mode, NULL);
#elif defined(HAVE_BSDDB)
  db = dbopen(file.c_str(), O_CREAT | O_RDWR, mode, DB_HASH, NULL);
  if (NULL == this->db)
  {
    BotDB::errno_ = errno;
  }
#endif

#if defined(HAVE_LIBGDBM) || defined(HAVE_BSDDB)
  if (NULL == this->db)
  {
    throw OOMon::botdb_error("Could not open database: " + file);
  }
#endif
}


BotDB::~BotDB()
{
#if defined(HAVE_LIBGDBM)
  gdbm_close(this->db);
#elif defined(HAVE_BSDDB)
  this->db->close(this->db);
#endif
}


int
BotDB::del(const std::string & key)
{
  BotDB::Lock lock(this->fd(), BotDB::Lock::EXCLUSIVE);

#if defined(HAVE_LIBGDBM)
  datum k = { dptr: const_cast<char *>(key.c_str()), dsize: key.length() + 1 };

  return gdbm_delete(this->db, k);
#elif defined(HAVE_BSDDB)
  DBT k = { data: const_cast<char *>(key.c_str()), size: key.length() + 1 };

  int result = this->db->del(this->db, &k, 0);

  if (0 == result)
  {
    this->sync();
  }
  else if (result < 0)
  {
    BotDB::errno_ = errno;
  }

  return result;
#else
  DBMap::iterator pos = db.find(key);

  if (pos != db.end())
  {
    db.erase(pos);
    return 0;
  }

  return -1;
#endif
}


int
BotDB::fd()
{
#if defined(HAVE_LIBGDBM)
  return gdbm_fdesc(this->db);
#elif defined(HAVE_BSDDB)
  int result = this->db->fd(this->db);

  if (result < 0)
  {
    BotDB::errno_ = errno;
  }

  return result;
#else
  return -1;
#endif
}


int
BotDB::get(const std::string & key, std::string & data)
{
  BotDB::Lock lock(this->fd(), BotDB::Lock::SHARED);

#if defined(HAVE_LIBGDBM)
  datum k = { dptr: const_cast<char *>(key.c_str()), dsize: key.length() + 1 };

  datum d = gdbm_fetch(this->db, k);

  if (d.dptr != NULL)
  {
    data = const_cast<char *>(d.dptr);
    free(d.dptr);
    return 0;
  }

  return -1;
#elif defined(HAVE_BSDDB)
  DBT k = { data: const_cast<char *>(key.c_str()), size: key.length() + 1 };
  DBT d;

  int result = this->db->get(this->db, &k, &d, 0);

  if (0 == result)
  {
    data = reinterpret_cast<char *>(d.data);
  }
  else if (result < 0)
  {
    BotDB::errno_ = errno;
  }

  return result;
#else
  DBMap::iterator pos = db.find(key);

  if (pos != db.end())
  {
    data = pos->second;
    return 0;
  }

  return -1;
#endif
}


int
BotDB::put(const std::string & key, const std::string & data)
{
  BotDB::Lock lock(this->fd(), BotDB::Lock::EXCLUSIVE);

#if defined(HAVE_LIBGDBM)
  datum k, d;

  k.dptr = const_cast<char *>(key.c_str());
  k.dsize = key.length() + 1;
  d.dptr = const_cast<char *>(data.c_str());
  d.dsize = data.length() + 1;

  int result = gdbm_store(this->db, k, d, GDBM_REPLACE);

  return result;
#elif defined(HAVE_BSDDB)
  DBT k = { data: const_cast<char *>(key.c_str()), size: key.length() + 1 };
  DBT d = { data: const_cast<char *>(data.c_str()), size: data.length() + 1 };

  int result = this->db->put(this->db, &k, &d, 0);

  if (0 == result)
  {
    this->sync();
  }
  else if (result < 0)
  {
    BotDB::errno_ = errno;
  }

  return result;
#else
  db[key] = data;
  return 0;
#endif
}


int
BotDB::sync()
{
#if defined(HAVE_LIBGDBM)
  gdbm_sync(this->db);
  return 0;
#elif defined(HAVE_BSDDB)
  int result = this->db->sync(this->db, 0);

  if (result < 0)
  {
    BotDB::errno_ = errno;
  }

  return result;
#else
  return 0;
#endif
}


int
BotDB::db_errno()
{
#if defined(HAVE_LIBGDBM)
  return gdbm_errno;
#elif defined(HAVE_BSDDB)
  return BotDB::errno_;
#else
  return 0;
#endif
}


std::string
BotDB::strerror(const int db_errno)
{
#if defined(HAVE_LIBGDBM)
  return gdbm_strerror(db_errno);
#elif defined(HAVE_BSDDB)
# if defined(HAVE_STRERROR)
  return ::strerror(db_errno);
# else
  return std::string("error");
# endif
#else
  return "Unknown error";
#endif
}

