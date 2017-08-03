/*
 * Module: sched.h
 *
 * Purpose:
 *      Provides an implementation of POSIX realtime extensions
 *      as defined in
 *
 *              POSIX 1003.1b-1993      (POSIX.1b)
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
#if !defined(_H_SCHED) && defined(_MSC_VER)
#define _H_SCHED

#include <errno.h>
#include <stdlib.h>

/*
 * [i_a] fix for using pthread_win32 with mongoose code, 
 * which #define's its own pid_t akin to
 * typedef HANDLE pid_t; 
 */
#undef pid_t
typedef void * pid_t;

/* Thread scheduling policies */

enum {
  SCHED_OTHER = 0,
  SCHED_FIFO,
  SCHED_RR,
  SCHED_MIN   = SCHED_OTHER,
  SCHED_MAX   = SCHED_RR
};

struct sched_param {
  int sched_priority;
};

/*
 * CPU affinity
 *
 * cpu_set_t:
 * Considered opaque but cannot be an opaque pointer
 * due to the need for compatibility with GNU systems
 * and sched_setaffinity() et.al. which include the
 * cpusetsize parameter "normally set to sizeof(cpu_set_t)".
 */

#define CPU_SETSIZE (sizeof(size_t) * 8)

#define CPU_COUNT(setptr) (_sched_affinitycpucount(setptr))

#define CPU_ZERO(setptr) (_sched_affinitycpuzero(setptr))

#define CPU_SET(cpu, setptr) (_sched_affinitycpuset((cpu), (setptr)))

#define CPU_CLR(cpu, setptr) (_sched_affinitycpuclr((cpu), (setptr)))

#define CPU_ISSET(cpu, setptr) (_sched_affinitycpuisset((cpu), (setptr)))

#define CPU_AND(destsetptr, srcset1ptr, srcset2ptr) (_sched_affinitycpuand((destsetptr), (srcset1ptr), (srcset2ptr)))

#define CPU_OR(destsetptr, srcset1ptr, srcset2ptr) (_sched_affinitycpuor((destsetptr), (srcset1ptr), (srcset2ptr)))

#define CPU_XOR(destsetptr, srcset1ptr, srcset2ptr) (_sched_affinitycpuxor((destsetptr), (srcset1ptr), (srcset2ptr)))

#define CPU_EQUAL(set1ptr, set2ptr) (_sched_affinitycpuequal((set1ptr), (set2ptr)))

typedef union
{
  char cpuset[CPU_SETSIZE / 8];
  size_t _align;
} cpu_set_t;

extern int __cdecl sched_yield (void);

extern int __cdecl sched_get_priority_min (int policy);

extern int __cdecl sched_get_priority_max (int policy);

extern int __cdecl sched_setscheduler (pid_t pid, int policy);

extern int __cdecl sched_getscheduler (pid_t pid);

/* Compatibility with Linux - not standard */

extern int __cdecl sched_setaffinity (pid_t pid, size_t cpusetsize, cpu_set_t * mask);

extern int __cdecl sched_getaffinity (pid_t pid, size_t cpusetsize, cpu_set_t * mask);

/*
 * Support routines and macros for cpu_set_t
 */
extern int __cdecl _sched_affinitycpucount (const cpu_set_t * set);

extern void __cdecl _sched_affinitycpuzero (cpu_set_t * pset);

extern void __cdecl _sched_affinitycpuset (int cpu, cpu_set_t * pset);

extern void __cdecl _sched_affinitycpuclr (int cpu, cpu_set_t * pset);

extern int __cdecl _sched_affinitycpuisset (int cpu, const cpu_set_t *pset);

extern void __cdecl _sched_affinitycpuand(cpu_set_t * pdestset, const cpu_set_t * psrcset1, const cpu_set_t * psrcset2);

extern void __cdecl _sched_affinitycpuor(cpu_set_t * pdestset, const cpu_set_t * psrcset1, const cpu_set_t * psrcset2);

extern void __cdecl _sched_affinitycpuxor(cpu_set_t * pdestset, const cpu_set_t * psrcset1, const cpu_set_t * psrcset2);

extern int __cdecl _sched_affinitycpuequal (const cpu_set_t * pset1, const cpu_set_t * pset2);

/*
 * Note that this macro returns ENOTSUP rather than
 * ENOSYS as might be expected. However, returning ENOSYS
 * should mean that sched_get_priority_{min,max} are
 * not implemented as well as sched_rr_get_interval.
 * This is not the case, since we just don't support
 * round-robin scheduling. Therefore I have chosen to
 * return the same value as sched_setscheduler when
 * SCHED_RR is passed to it.
 */
#define sched_rr_get_interval(_pid, _interval) \
  (errno = ENOTSUP, (int) -1)


#endif                          /* !_H_SCHED */