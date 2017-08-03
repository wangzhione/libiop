/* This is an implementation of the threads API of POSIX 1003.1-2001.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2012 Pthreads-win32 contributors
 *
 *      Homepage1: http://sourceware.org/pthreads-win32/
 *      Homepage2: http://sourceforge.net/projects/pthreads4w/
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#if !defined(_H_PTHREAD) && defined(_MSC_VER)
#define _H_PTHREAD

/*
 * See the README file for an explanation of the pthreads-win32 version
 * numbering scheme and how the DLL is named etc.
 */
#define PTW32_VERSION 2, 10, 0, 1
#define PTW32_VERSION_STRING "2, 10, 0, 1"

#pragma comment(lib, "pthread_lib.lib")

/*
 * -------------------------------------------------------------
 *
 *
 * Module: pthread.h
 *
 * Purpose:
 *      Provides an implementation of PThreads based upon the
 *      standard:
 *
 *              POSIX 1003.1-2001
 *  and
 *    The Single Unix Specification version 3
 *
 *    (these two are equivalent)
 *
 *      in order to enhance code portability between Windows,
 *  various commercial Unix implementations, and Linux.
 *
 *      See the ANNOUNCE file for a full list of conforming
 *      routines and defined constants, and a list of missing
 *      routines and constants not defined in this implementation.
 *
 * Authors:
 *      There have been many contributors to this library.
 *      The initial implementation was contributed by
 *      John Bossom, and several others have provided major
 *      sections or revisions of parts of the implementation.
 *      Often significant effort has been contributed to
 *      find and fix important bugs and other problems to
 *      improve the reliability of the library, which sometimes
 *      is not reflected in the amount of code which changed as
 *      result.
 *      As much as possible, the contributors are acknowledged
 *      in the ChangeLog file in the source code distribution
 *      where their changes are noted in detail.
 *
 *      Contributors are listed in the CONTRIBUTORS file.
 *
 *      As usual, all bouquets go to the contributors, and all
 *      brickbats go to the project maintainer.
 *
 * Maintainer:
 *      The code base for this project is coordinated and
 *      eventually pre-tested, packaged, and made available by
 *
 *              Ross Johnson <rpj@callisto.canberra.edu.au>
 *
 * QA Testers:
 *      Ultimately, the library is tested in the real world by
 *      a host of competent and demanding scientists and
 *      engineers who report bugs and/or provide solutions
 *      which are then fixed or incorporated into subsequent
 *      versions of the library. Each time a bug is fixed, a
 *      test case is written to prove the fix and ensure
 *      that later changes to the code don't reintroduce the
 *      same error. The number of test cases is slowly growing
 *      and therefore so is the code reliability.
 *
 * Compliance:
 *      See the file ANNOUNCE for the list of implemented
 *      and not-implemented routines and defined options.
 *      Of course, these are all defined is this file as well.
 *
 * Web site:
 *      The source code and other information about this library
 *      are available from
 *
 *              http://sources.redhat.com/pthreads-win32/
 *
 * -------------------------------------------------------------
 */

/*
 * Boolean values to make us independent of system includes.
 */
enum {
  PTW32_FALSE = 0,
  PTW32_TRUE = (! PTW32_FALSE)
};

#include <time.h>
#include <sched.h>
#include <limits.h>

/*
 * To avoid including windows.h we define only those things that we
 * actually need from it.
 */
#if !defined(PTW32_INCLUDE_WINDOWS_H)
#if !defined(HANDLE)
# define PTW32__HANDLE_DEF
# define HANDLE void *
#endif
#if !defined(DWORD)
# define PTW32__DWORD_DEF
# define DWORD unsigned long
#endif
#endif

#if !defined(SIG_BLOCK)
#define SIG_BLOCK 0
#endif /* SIG_BLOCK */

#if !defined(SIG_UNBLOCK)
#define SIG_UNBLOCK 1
#endif /* SIG_UNBLOCK */

#if !defined(SIG_SETMASK)
#define SIG_SETMASK 2
#endif /* SIG_SETMASK */

/*
 * -------------------------------------------------------------
 *
 * POSIX 1003.1-2001 Options
 * =========================
 *
 * Options are normally set in <unistd.h>, which is not provided
 * with pthreads-win32.
 *
 * For conformance with the Single Unix Specification (version 3), all of the
 * options below are defined, and have a value of either -1 (not supported)
 * or 200112L (supported).
 *
 * These options can neither be left undefined nor have a value of 0, because
 * either indicates that sysconf(), which is not implemented, may be used at
 * runtime to check the status of the option.
 *
 * _POSIX_THREADS (== 20080912L)
 *                      If == 20080912L, you can use threads
 *
 * _POSIX_THREAD_ATTR_STACKSIZE (== 200809L)
 *                      If == 200809L, you can control the size of a thread's
 *                      stack
 *                              pthread_attr_getstacksize
 *                              pthread_attr_setstacksize
 *
 * _POSIX_THREAD_ATTR_STACKADDR (== -1)
 *                      If == 200809L, you can allocate and control a thread's
 *                      stack. If not supported, the following functions
 *                      will return ENOSYS, indicating they are not
 *                      supported:
 *                              pthread_attr_getstackaddr
 *                              pthread_attr_setstackaddr
 *
 * _POSIX_THREAD_PRIORITY_SCHEDULING (== -1)
 *                      If == 200112L, you can use realtime scheduling.
 *                      This option indicates that the behaviour of some
 *                      implemented functions conforms to the additional TPS
 *                      requirements in the standard. E.g. rwlocks favour
 *                      writers over readers when threads have equal priority.
 *
 * _POSIX_THREAD_PRIO_INHERIT (== -1)
 *                      If == 200809L, you can create priority inheritance
 *                      mutexes.
 *                              pthread_mutexattr_getprotocol +
 *                              pthread_mutexattr_setprotocol +
 *
 * _POSIX_THREAD_PRIO_PROTECT (== -1)
 *                      If == 200809L, you can create priority ceiling mutexes
 *                      Indicates the availability of:
 *                              pthread_mutex_getprioceiling
 *                              pthread_mutex_setprioceiling
 *                              pthread_mutexattr_getprioceiling
 *                              pthread_mutexattr_getprotocol     +
 *                              pthread_mutexattr_setprioceiling
 *                              pthread_mutexattr_setprotocol     +
 *
 * _POSIX_THREAD_PROCESS_SHARED (== -1)
 *                      If set, you can create mutexes and condition
 *                      variables that can be shared with another
 *                      process.If set, indicates the availability
 *                      of:
 *                              pthread_mutexattr_getpshared
 *                              pthread_mutexattr_setpshared
 *                              pthread_condattr_getpshared
 *                              pthread_condattr_setpshared
 *
 * _POSIX_THREAD_SAFE_FUNCTIONS (== 200809L)
 *                      If == 200809L you can use the special *_r library
 *                      functions that provide thread-safe behaviour
 *
 * _POSIX_READER_WRITER_LOCKS (== 200809L)
 *                      If == 200809L, you can use read/write locks
 *
 * _POSIX_SPIN_LOCKS (== 200809L)
 *                      If == 200809L, you can use spin locks
 *
 * _POSIX_BARRIERS (== 200809L)
 *                      If == 200809L, you can use barriers
 *
 * _POSIX_ROBUST_MUTEXES (== 200809L)
 *                      If == 200809L, you can use robust mutexes
 *                      Officially this should also imply
 *                      _POSIX_THREAD_PROCESS_SHARED != -1 however
 *                      not here yet.
 *
 * -------------------------------------------------------------
 */

/*
 * POSIX Options
 */
#undef _POSIX_THREADS
#define _POSIX_THREADS 200809L

#undef _POSIX_READER_WRITER_LOCKS
#define _POSIX_READER_WRITER_LOCKS 200809L

#undef _POSIX_SPIN_LOCKS
#define _POSIX_SPIN_LOCKS 200809L

#undef _POSIX_BARRIERS
#define _POSIX_BARRIERS 200809L

#undef _POSIX_THREAD_SAFE_FUNCTIONS
#define _POSIX_THREAD_SAFE_FUNCTIONS 200809L

#undef _POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_ATTR_STACKSIZE 200809L

#undef _POSIX_ROBUST_MUTEXES
#define _POSIX_ROBUST_MUTEXES 200809L

/*
 * The following options are not supported
 */
#undef _POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_ATTR_STACKADDR -1

#undef _POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_INHERIT -1

#undef _POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PRIO_PROTECT -1

/* TPS is not fully supported.  */
#undef _POSIX_THREAD_PRIORITY_SCHEDULING
#define _POSIX_THREAD_PRIORITY_SCHEDULING -1

#undef _POSIX_THREAD_PROCESS_SHARED
#define _POSIX_THREAD_PROCESS_SHARED -1


/*
 * POSIX 1003.1-2001 Limits
 * ===========================
 *
 * These limits are normally set in <limits.h>, which is not provided with
 * pthreads-win32.
 *
 * PTHREAD_DESTRUCTOR_ITERATIONS
 *                      Maximum number of attempts to destroy
 *                      a thread's thread-specific data on
 *                      termination (must be at least 4)
 *
 * PTHREAD_KEYS_MAX
 *                      Maximum number of thread-specific data keys
 *                      available per process (must be at least 128)
 *
 * PTHREAD_STACK_MIN
 *                      Minimum supported stack size for a thread
 *
 * PTHREAD_THREADS_MAX
 *                      Maximum number of threads supported per
 *                      process (must be at least 64).
 *
 * SEM_NSEMS_MAX
 *                      The maximum number of semaphores a process can have.
 *                      (must be at least 256)
 *
 * SEM_VALUE_MAX
 *                      The maximum value a semaphore can have.
 *                      (must be at least 32767)
 *
 */
#undef _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS     4

#undef PTHREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_DESTRUCTOR_ITERATIONS           _POSIX_THREAD_DESTRUCTOR_ITERATIONS

#undef _POSIX_THREAD_KEYS_MAX
#define _POSIX_THREAD_KEYS_MAX                  128

#undef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX                        _POSIX_THREAD_KEYS_MAX

#undef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN                       0

#undef _POSIX_THREAD_THREADS_MAX
#define _POSIX_THREAD_THREADS_MAX               64

  /* Arbitrary value */
#undef PTHREAD_THREADS_MAX
#define PTHREAD_THREADS_MAX                     2019

#undef _POSIX_SEM_NSEMS_MAX
#define _POSIX_SEM_NSEMS_MAX                    256

  /* Arbitrary value */
#undef SEM_NSEMS_MAX
#define SEM_NSEMS_MAX                           1024

#undef _POSIX_SEM_VALUE_MAX
#define _POSIX_SEM_VALUE_MAX                    32767

#undef SEM_VALUE_MAX
#define SEM_VALUE_MAX                           INT_MAX

/*
 * Generic handle type - intended to extend uniqueness beyond
 * that available with a simple pointer. It should scale for either
 * IA-32 or IA-64.
 */
typedef struct {
    void * p;                   /* Pointer to actual object */
    unsigned int x;             /* Extra information - reuse count etc */
} ptw32_handle_t;

typedef ptw32_handle_t pthread_t;
typedef struct pthread_attr_t_ * pthread_attr_t;
typedef struct pthread_once_t_ pthread_once_t;
typedef struct pthread_key_t_ * pthread_key_t;
typedef struct pthread_mutex_t_ * pthread_mutex_t;
typedef struct pthread_mutexattr_t_ * pthread_mutexattr_t;
typedef struct pthread_cond_t_ * pthread_cond_t;
typedef struct pthread_condattr_t_ * pthread_condattr_t;

typedef struct pthread_rwlock_t_ * pthread_rwlock_t;
typedef struct pthread_rwlockattr_t_ * pthread_rwlockattr_t;
typedef struct pthread_spinlock_t_ * pthread_spinlock_t;
typedef struct pthread_barrier_t_ * pthread_barrier_t;
typedef struct pthread_barrierattr_t_ * pthread_barrierattr_t;

/*
 * ====================
 * ====================
 * POSIX Threads
 * ====================
 * ====================
 */

enum {
/*
 * pthread_attr_{get,set}detachstate
 */
  PTHREAD_CREATE_JOINABLE       = 0,  /* Default */
  PTHREAD_CREATE_DETACHED       = 1,

/*
 * pthread_attr_{get,set}inheritsched
 */
  PTHREAD_INHERIT_SCHED         = 0,
  PTHREAD_EXPLICIT_SCHED        = 1,  /* Default */

/*
 * pthread_{get,set}scope
 */
  PTHREAD_SCOPE_PROCESS         = 0,
  PTHREAD_SCOPE_SYSTEM          = 1,  /* Default */

/*
 * pthread_setcancelstate paramters
 */
  PTHREAD_CANCEL_ENABLE         = 0,  /* Default */
  PTHREAD_CANCEL_DISABLE        = 1,

/*
 * pthread_setcanceltype parameters
 */
  PTHREAD_CANCEL_ASYNCHRONOUS   = 0,
  PTHREAD_CANCEL_DEFERRED       = 1,  /* Default */

/*
 * pthread_mutexattr_{get,set}pshared
 * pthread_condattr_{get,set}pshared
 */
  PTHREAD_PROCESS_PRIVATE       = 0,
  PTHREAD_PROCESS_SHARED        = 1,

/*
 * pthread_mutexattr_{get,set}robust
 */
  PTHREAD_MUTEX_STALLED         = 0,  /* Default */
  PTHREAD_MUTEX_ROBUST          = 1,

/*
 * pthread_barrier_wait
 */
  PTHREAD_BARRIER_SERIAL_THREAD = -1
};

/*
 * ====================
 * ====================
 * Cancellation
 * ====================
 * ====================
 */
#define PTHREAD_CANCELED       ((void *)(size_t) -1)


/*
 * ====================
 * ====================
 * Once Key
 * ====================
 * ====================
 */
#define PTHREAD_ONCE_INIT       { PTW32_FALSE, 0, 0, 0}

struct pthread_once_t_ {
  int          done;        /* indicates if user function has been executed */
  void *       lock;
  int          reserved1;
  int          reserved2;
};


/*
 * ====================
 * ====================
 * Object initialisers
 * ====================
 * ====================
 */
#define PTHREAD_MUTEX_INITIALIZER ((pthread_mutex_t)(size_t) -1)
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER ((pthread_mutex_t)(size_t) -2)
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER ((pthread_mutex_t)(size_t) -3)

/*
 * Compatibility with LinuxThreads
 */
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP PTHREAD_ERRORCHECK_MUTEX_INITIALIZER

#define PTHREAD_COND_INITIALIZER ((pthread_cond_t)(size_t) -1)

#define PTHREAD_RWLOCK_INITIALIZER ((pthread_rwlock_t)(size_t) -1)

#define PTHREAD_SPINLOCK_INITIALIZER ((pthread_spinlock_t)(size_t) -1)


/*
 * Mutex types.
 */
enum {
  /* Compatibility with LinuxThreads */
  PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_TIMED_NP        = PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_ADAPTIVE_NP     = PTHREAD_MUTEX_FAST_NP,
  /* For compatibility with POSIX */
  PTHREAD_MUTEX_NORMAL          = PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_RECURSIVE       = PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK      = PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_DEFAULT         = PTHREAD_MUTEX_NORMAL
};


typedef struct ptw32_cleanup_t ptw32_cleanup_t;

/* Disable MSVC 'anachronism used' warning */
#pragma warning(disable : 4229)

typedef void (* __cdecl ptw32_cleanup_callback_t)(void *);

#pragma warning(default : 4229)

struct ptw32_cleanup_t {
  ptw32_cleanup_callback_t routine;
  void * arg;
  struct ptw32_cleanup_t * prev;
};

/*
 * WIN32 SEH version of cancel cleanup.
 */

#define pthread_cleanup_push( _rout, _arg ) \
        { \
            ptw32_cleanup_t     _cleanup; \
			_cleanup.routine    = (ptw32_cleanup_callback_t)(_rout); \
            _cleanup.arg        = (_arg); \
            __try { \

#define pthread_cleanup_pop( _execute ) \
            } \
            __finally { \
                if( _execute || AbnormalTermination()) { \
                        (*(_cleanup.routine))( _cleanup.arg ); \
                } \
            } \
        }

/*
 * ===============
 * ===============
 * Methods
 * ===============
 * ===============
 */

/*
 * PThread Attribute Functions
 */
extern int __cdecl pthread_attr_init (pthread_attr_t * attr);

extern int __cdecl pthread_attr_destroy (pthread_attr_t * attr);

extern int __cdecl pthread_attr_getdetachstate (const pthread_attr_t * attr, int * detachstate);

extern int __cdecl pthread_attr_getstackaddr (const pthread_attr_t * attr, void ** stackaddr);

extern int __cdecl pthread_attr_getstacksize (const pthread_attr_t * attr, size_t * stacksize);

extern int __cdecl pthread_attr_setdetachstate (pthread_attr_t * attr, int detachstate);

extern int __cdecl pthread_attr_setstackaddr (pthread_attr_t * attr, void * stackaddr);

extern int __cdecl pthread_attr_setstacksize (pthread_attr_t * attr, size_t stacksize);

extern int __cdecl pthread_attr_getschedparam (const pthread_attr_t * attr, struct sched_param * param);

extern int __cdecl pthread_attr_setschedparam (pthread_attr_t * attr, const struct sched_param * param);

extern int __cdecl pthread_attr_setschedpolicy (pthread_attr_t * , int);

extern int __cdecl pthread_attr_getschedpolicy (const pthread_attr_t * , int * );

extern int __cdecl pthread_attr_setinheritsched(pthread_attr_t * attr, int inheritsched);

extern int __cdecl pthread_attr_getinheritsched(const pthread_attr_t * attr, int * inheritsched);

extern int __cdecl pthread_attr_setscope (pthread_attr_t * , int );

extern int __cdecl pthread_attr_getscope (const pthread_attr_t * , int * );

/*
 * PThread Functions
 */
extern int __cdecl pthread_create (pthread_t * tid,
                            const pthread_attr_t * attr,
                            void * (__cdecl * start) (void *),
                            void * arg);

extern int __cdecl pthread_detach (pthread_t tid);

extern int __cdecl pthread_equal (pthread_t t1, pthread_t t2);

extern void __cdecl pthread_exit (void * value_ptr);

extern int __cdecl pthread_join (pthread_t thread, void ** value_ptr);

extern pthread_t __cdecl pthread_self (void);

extern int __cdecl pthread_cancel (pthread_t thread);

extern int __cdecl pthread_setcancelstate (int state, int * oldstate);

extern int __cdecl pthread_setcanceltype (int type, int * oldtype);

extern void __cdecl pthread_testcancel (void);

extern int __cdecl pthread_once (pthread_once_t * once_control, void (__cdecl * init_routine) (void));

extern ptw32_cleanup_t * __cdecl ptw32_pop_cleanup (int execute);

extern void __cdecl ptw32_push_cleanup (ptw32_cleanup_t * cleanup, ptw32_cleanup_callback_t routine, void * arg);

/*
 * Thread Specific Data Functions
 */
extern int __cdecl pthread_key_create (pthread_key_t * key, void (__cdecl * destructor) (void *));

extern int __cdecl pthread_key_delete (pthread_key_t key);

extern int __cdecl pthread_setspecific (pthread_key_t key, const void * value);

extern void * __cdecl pthread_getspecific (pthread_key_t key);


/*
 * Mutex Attribute Functions
 */
extern int __cdecl pthread_mutexattr_init (pthread_mutexattr_t * attr);

extern int __cdecl pthread_mutexattr_destroy (pthread_mutexattr_t * attr);

extern int __cdecl pthread_mutexattr_getpshared (const pthread_mutexattr_t * attr, int * pshared);

extern int __cdecl pthread_mutexattr_setpshared (pthread_mutexattr_t * attr, int pshared);

extern int __cdecl pthread_mutexattr_settype (pthread_mutexattr_t * attr, int kind);
extern int __cdecl pthread_mutexattr_gettype (const pthread_mutexattr_t * attr, int *kind);

extern int __cdecl pthread_mutexattr_setrobust(pthread_mutexattr_t * attr, int robust);
extern int __cdecl pthread_mutexattr_getrobust(const pthread_mutexattr_t * attr, int * robust);

/*
 * Barrier Attribute Functions
 */
extern int __cdecl pthread_barrierattr_init (pthread_barrierattr_t * attr);

extern int __cdecl pthread_barrierattr_destroy (pthread_barrierattr_t * attr);

extern int __cdecl pthread_barrierattr_getpshared (const pthread_barrierattr_t * attr, int * pshared);

extern int __cdecl pthread_barrierattr_setpshared (pthread_barrierattr_t * attr, int pshared);

/*
 * Mutex Functions
 */
extern int __cdecl pthread_mutex_init (pthread_mutex_t * mutex, const pthread_mutexattr_t * attr);

extern int __cdecl pthread_mutex_destroy (pthread_mutex_t * mutex);

extern int __cdecl pthread_mutex_lock (pthread_mutex_t * mutex);

extern int __cdecl pthread_mutex_timedlock(pthread_mutex_t * mutex, const struct timespec * abstime);

extern int __cdecl pthread_mutex_trylock (pthread_mutex_t * mutex);

extern int __cdecl pthread_mutex_unlock (pthread_mutex_t * mutex);

extern int __cdecl pthread_mutex_consistent (pthread_mutex_t * mutex);

/*
 * Spinlock Functions
 */
extern int __cdecl pthread_spin_init (pthread_spinlock_t * lock, int pshared);

extern int __cdecl pthread_spin_destroy (pthread_spinlock_t * lock);

extern int __cdecl pthread_spin_lock (pthread_spinlock_t * lock);

extern int __cdecl pthread_spin_trylock (pthread_spinlock_t * lock);

extern int __cdecl pthread_spin_unlock (pthread_spinlock_t * lock);

/*
 * Barrier Functions
 */
extern int __cdecl pthread_barrier_init (pthread_barrier_t * barrier, const pthread_barrierattr_t * attr, unsigned int count);

extern int __cdecl pthread_barrier_destroy (pthread_barrier_t * barrier);

extern int __cdecl pthread_barrier_wait (pthread_barrier_t * barrier);

/*
 * Condition Variable Attribute Functions
 */
extern int __cdecl pthread_condattr_init (pthread_condattr_t * attr);

extern int __cdecl pthread_condattr_destroy (pthread_condattr_t * attr);

extern int __cdecl pthread_condattr_getpshared (const pthread_condattr_t * attr, int * pshared);

extern int __cdecl pthread_condattr_setpshared (pthread_condattr_t * attr, int pshared);

/*
 * Condition Variable Functions
 */
extern int __cdecl pthread_cond_init (pthread_cond_t * cond, const pthread_condattr_t * attr);

extern int __cdecl pthread_cond_destroy (pthread_cond_t * cond);

extern int __cdecl pthread_cond_wait (pthread_cond_t * cond, pthread_mutex_t * mutex);

extern int __cdecl pthread_cond_timedwait (pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime);

extern int __cdecl pthread_cond_signal (pthread_cond_t * cond);

extern int __cdecl pthread_cond_broadcast (pthread_cond_t * cond);

/*
 * Scheduling
 */
extern int __cdecl pthread_setschedparam (pthread_t thread, int policy, const struct sched_param * param);

extern int __cdecl pthread_getschedparam (pthread_t thread, int * policy, struct sched_param * param);

extern int __cdecl pthread_setconcurrency (int);

extern int __cdecl pthread_getconcurrency (void);

/*
 * Read-Write Lock Functions
 */
extern int __cdecl pthread_rwlock_init(pthread_rwlock_t * lock, const pthread_rwlockattr_t * attr);

extern int __cdecl pthread_rwlock_destroy(pthread_rwlock_t * lock);

extern int __cdecl pthread_rwlock_tryrdlock(pthread_rwlock_t *);

extern int __cdecl pthread_rwlock_trywrlock(pthread_rwlock_t *);

extern int __cdecl pthread_rwlock_rdlock(pthread_rwlock_t * lock);

extern int __cdecl pthread_rwlock_timedrdlock(pthread_rwlock_t * lock, const struct timespec * abstime);

extern int __cdecl pthread_rwlock_wrlock(pthread_rwlock_t * lock);

extern int __cdecl pthread_rwlock_timedwrlock(pthread_rwlock_t * lock, const struct timespec * abstime);

extern int __cdecl pthread_rwlock_unlock(pthread_rwlock_t * lock);

extern int __cdecl pthread_rwlockattr_init (pthread_rwlockattr_t * attr);

extern int __cdecl pthread_rwlockattr_destroy (pthread_rwlockattr_t * attr);

extern int __cdecl pthread_rwlockattr_getpshared (const pthread_rwlockattr_t * attr, int * pshared);

extern int __cdecl pthread_rwlockattr_setpshared (pthread_rwlockattr_t * attr, int pshared);

/*
 * Signal Functions. Should be defined in <signal.h> but MSVC and MinGW32
 * already have signal.h that don't define these.
 */
extern int __cdecl pthread_kill(pthread_t thread, int sig);

/*
 * Non-portable functions
 */

/*
 * Compatibility with Linux.
 */
extern int __cdecl pthread_mutexattr_setkind_np(pthread_mutexattr_t * attr, int kind);
extern int __cdecl pthread_mutexattr_getkind_np(pthread_mutexattr_t * attr, int * kind);
extern int __cdecl pthread_timedjoin_np(pthread_t thread, void ** value_ptr, const struct timespec * abstime);
extern int __cdecl pthread_tryjoin_np(pthread_t thread, void ** value_ptr);
extern int __cdecl pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t * cpuset);
extern int __cdecl pthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t * cpuset);

/*
 * Possibly supported by other POSIX threads implementations
 */
extern int __cdecl pthread_delay_np (struct timespec * interval);
extern int __cdecl pthread_num_processors_np(void);
extern unsigned __int64 __cdecl pthread_getunique_np(pthread_t thread);

/*
 * Useful if an application wants to statically link
 * the lib rather than load the DLL at run-time.
 */
extern int __cdecl pthread_win32_process_attach_np(void);
extern int __cdecl pthread_win32_process_detach_np(void);
extern int __cdecl pthread_win32_thread_attach_np(void);
extern int __cdecl pthread_win32_thread_detach_np(void);

/*
 * Features that are auto-detected at load/run time.
 */
extern int __cdecl pthread_win32_test_features_np(int);
enum ptw32_features {
  PTW32_SYSTEM_INTERLOCKED_COMPARE_EXCHANGE = 0x0001,	/* System provides it. */
  PTW32_ALERTABLE_ASYNC_CANCEL              = 0x0002	/* Can cancel blocked threads. */
};

/*
 * Register a system time change with the library.
 * Causes the library to perform various functions
 * in response to the change. Should be called whenever
 * the application's top level window receives a
 * WM_TIMECHANGE message. It can be passed directly to
 * pthread_create() as a new thread if desired.
 */
extern void * __cdecl pthread_timechange_handler_np(void *);

/*
 * Returns the Win32 HANDLE for the POSIX thread.
 */
extern HANDLE __cdecl pthread_getw32threadhandle_np(pthread_t thread);
/*
 * Returns the win32 thread ID for POSIX thread.
 */
extern DWORD __cdecl pthread_getw32threadid_np (pthread_t thread);


/*
 * Protected Methods
 *
 * This function blocks until the given WIN32 handle
 * is signaled or pthread_cancel had been called.
 * This function allows the caller to hook into the
 * PThreads cancel mechanism. It is implemented using
 *
 *              WaitForMultipleObjects
 *
 * on 'waitHandle' and a manually reset WIN32 Event
 * used to implement pthread_cancel. The 'timeout'
 * argument to TimedWait is simply passed to
 * WaitForMultipleObjects.
 */
extern int __cdecl pthreadCancelableWait (HANDLE waitHandle);
extern int __cdecl pthreadCancelableTimedWait (HANDLE waitHandle, DWORD timeout);

/*
 * If pthreads-win32 is compiled as a DLL with MSVC, and
 * both it and the application are linked against the static
 * C runtime (i.e. with the /MT compiler flag), then the
 * application will not see the same C runtime globals as
 * the library. These include the errno variable, and the
 * termination routine called by terminate(). For details,
 * refer to the following links:
 *
 * http://support.microsoft.com/kb/94248
 * (Section 4: Problems Encountered When Using Multiple CRT Libraries)
 *
 * http://social.msdn.microsoft.com/forums/en-US/vclanguage/thread/b4500c0d-1b69-40c7-9ef5-08da1025b5bf
 *
 * When pthreads-win32 is built with PTW32_USES_SEPARATE_CRT
 * defined, the following features are enabled:
 *
 * (1) In addition to setting the errno variable when errors
 * occur, the library will also call SetLastError() with the
 * same value. The application can then use GetLastError()
 * to obtain the value of errno. (This pair of routines are
 * in kernel32.dll, and so are not affected by the use of
 * multiple CRT libraries.)
 *
 * (2) When C++ or SEH cleanup is used, the library defines
 * a function pthread_win32_set_terminate_np(), which can be
 * used to set the termination routine that should be called
 * when an unhandled exception occurs in a thread function
 * (or otherwise inside the library).
 *
 * Note: "_DLL" implies the /MD compiler flag.
 */
typedef void (* ptw32_terminate_handler)();
extern ptw32_terminate_handler __cdecl pthread_win32_set_terminate_np(ptw32_terminate_handler termFunction);

/* FIXME: This is only required if the library was built using SEH */
/*
 * Get internal SEH tag
 */
extern DWORD __cdecl ptw32_get_exception_services_code(void);

/*
 * Redefine the SEH __except keyword to ensure that applications
 * propagate our internal exceptions up to the library's internal handlers.
 */
#define __except( E ) \
        __except( ( GetExceptionCode() == ptw32_get_exception_services_code() ) \
                 ? EXCEPTION_CONTINUE_SEARCH : ( E ) )

#if defined(PTW32__HANDLE_DEF)
# undef HANDLE
#endif
#if defined(PTW32__DWORD_DEF)
# undef DWORD
#endif

#endif /* _H_PTHREAD */