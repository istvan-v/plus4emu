
// plus4emu -- portable Commodore Plus/4 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/plus4emu/
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

#ifndef PLUS4EMU_SYSTEM_HPP
#define PLUS4EMU_SYSTEM_HPP

#include "plus4emu.hpp"

#ifdef WIN32
#  include <stdarg.h>
#  include <windef.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#endif

namespace Plus4Emu {

  class ThreadLock {
   private:
    struct ThreadLock_ {
#ifdef WIN32
      HANDLE          evt;
#else
      pthread_mutex_t m;
      pthread_cond_t  c;
      unsigned char   s;
#endif
      long            refCnt;
    };
    ThreadLock_ *st;
   public:
    ThreadLock(bool isSignaled = false);
    ThreadLock(const ThreadLock&);
    ~ThreadLock();
    ThreadLock& operator=(const ThreadLock&);
    void wait();
    // wait with a timeout of 't' (in milliseconds)
    // returns 'true' if the lock was signaled before the timeout,
    // and 'false' otherwise
    bool wait(size_t t);
    void notify();
  };

  class Thread {
   private:
#ifdef WIN32
    HANDLE    thread_;
    static unsigned int __stdcall threadRoutine_(void *userData);
#else
    pthread_t thread_;
    static void * threadRoutine_(void *userData);
#endif
    ThreadLock  threadLock_;
    bool    isJoined_;
   protected:
    // thread routine (should be implemented by classes derived from Thread)
    virtual void run() = 0;
    // wait until start() is called by another thread
    inline void wait()
    {
      threadLock_.wait();
    }
    // wait until start() is called by another thread (return value = true),
    // or the timeout of 't' milliseconds is elapsed (return value = false)
    inline bool wait(size_t t)
    {
      return threadLock_.wait(t);
    }
   public:
    Thread();
    virtual ~Thread();
    // signal the child thread, allowing it to execute run() after the thread
    // object is created, or to return from wait()
    inline void start()
    {
      threadLock_.notify();
    }
    // wait until the child thread finishes (implies calling start() first,
    // and the destructor calls join())
    void join();
  };

  class Mutex {
   private:
    struct Mutex_ {
#ifdef WIN32
      CRITICAL_SECTION  mutex_;
#else
      pthread_mutex_t   mutex_;
#endif
      long    refCnt_;
    };
    Mutex_  *m;
   public:
    Mutex();
    Mutex(const Mutex& m_);
    ~Mutex();
    Mutex& operator=(const Mutex& m_);
    void lock();
    void unlock();
  };

  class Timer {
   private:
    uint64_t  startTime;
    double    secondsPerTick;
    static uint64_t getRealTime_();
   public:
    Timer();
    ~Timer();
    double getRealTime();
    void reset();
    void reset(double t);
    static void wait(double t);
    static uint32_t getRandomSeedFromTime();
  };

  // remove leading and trailing whitespace from string
  void stripString(std::string& s);

  // convert string to upper case
  void stringToUpperCase(std::string& s);

  // convert string to lower case
  void stringToLowerCase(std::string& s);

  // split path into directory name and base name
  // note: the result of passing multiple references to the same string
  // is undefined
  void splitPath(const std::string& path_,
                 std::string& dirname_, std::string& basename_);

  // returns full path to ~/.plus4emu, creating the directory first
  // if it does not exist yet
  std::string getPlus4EmuHomeDirectory();

  // returns a pseudo-random number in the range 1 to 0x7FFFFFFE
  int getRandomNumber(int& seedValue);

  // initialize a pseudo-random generator to be used with getRandomNumber()
  void setRandomSeed(int& seedValue, uint32_t n);

  // Set the process priority to 'n' (-2 to +3, 0 is the normal priority,
  // higher values mean higher priority). On error, such as not having
  // sufficient privileges, Plus4Emu::Exception may be thrown.
  void setProcessPriority(int n);

#ifndef WIN32
  PLUS4EMU_INLINE std::FILE *fileOpen(const char *fileName, const char *mode)
  {
    return std::fopen(fileName, mode);
  }
  PLUS4EMU_INLINE int fileRemove(const char *fileName)
  {
    return std::remove(fileName);
  }
#else
  /*!
   * Convert UTF-8 encoded string to wchar_t.
   */
  void convertUTF8(wchar_t *buf, const char *s, size_t bufSize);

  // file I/O wrappers with support for UTF-8 encoded file names
  std::FILE *fileOpen(const char *fileName, const char *mode);
  int fileRemove(const char *fileName);
#endif

}       // namespace Plus4Emu

#endif  // PLUS4EMU_SYSTEM_HPP

