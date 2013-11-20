// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file memleak.c Keep track of all allocations, sorted by backtrace.
//
// Copyright (C) 2010, by
// 
// Aleric Inglewood <aleric.inglewood@gmail.com>
// Aleric on freenode.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef USE_PCH
#define _GNU_SOURCE
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <execinfo.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#endif

#include "addr2line.h"
#include "sort.h"

static void* malloc_bootstrap1(size_t size);
static void* calloc_bootstrap1(size_t nmemb, size_t size);
static void init(void);

//---------------------------------------------------------------------------------------------
// Debug stuff

#undef assert

#ifdef DEBUG

#undef DEBUG_EXPENSIVE
#undef DEBUG_PRINT

static void print(char const* ptr)
{
  write(1, ptr, strlen(ptr));
}

#ifdef DEBUG_PRINT
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

static void print_size(size_t size)
{
  char buf[32];
  char* ptr = &buf[31];
  *ptr-- = 0;
  do
  {
    *ptr-- = '0' + (size % 10);
    size /= 10;
  }
  while(size);
  print(ptr + 1);
}

static void print_ptr(void* ptrin)
{
  write(1, "0x", 2);
  intptr_t size = (intptr_t)ptrin;
  char buf[32];
  char* ptr = &buf[31];
  *ptr-- = 0;
  do
  {
    int d = size & 0xf;
    if (d < 10)
      *ptr-- = '0' + d;
    else
      *ptr-- = 'a' - 10 + d;
    size >>= 4;
  }
  while(size);
  print(ptr + 1);
}

static void print_lock(void)
{
  pthread_mutex_lock(&print_mutex);
  print_ptr((void*)pthread_self());
  write(1, ": ", 2);
}

static void print_unlock(void)
{
  write(1, "\n", 1);
  pthread_mutex_unlock(&print_mutex);
}

#define Debug(x) do { x; } while(0)
#endif // DEBUG_PRINT

#define assert(x) do { if (!(x)) { print("Assertion " #x " failed!\n"); abort(); } } while(0)

#else // DEBUG

#define assert(x) do { } while(0)

#endif // DEBUG

#ifndef Debug
#define Debug(x) do { } while(0)
#endif

//---------------------------------------------------------------------------------------------
// Function pointers to the real (or bootstrap) functions.

void* (*memleak_libc_malloc)(size_t size) = &malloc_bootstrap1;
static void* (*libc_calloc)(size_t nmemb, size_t size) = &calloc_bootstrap1;
static void* (*libc_realloc)(void* ptr, size_t size);
void (*memleak_libc_free)(void* ptr);
static void (*libc_free_final)(void* ptr) = (void (*)(void*))0;
static int (*libc_posix_memalign)(void** memptr, size_t alignment, size_t size) = (int (*)(void**, size_t, size_t))0;

//---------------------------------------------------------------------------------------------
// This is a tiny 'malloc library' that is used while
// looking up the real malloc functions in libc.
// We expect only a single call to malloc() from dlopen (from init_malloc_function_pointers)
// but provide a slightly more general set of stubs here anyway.

#define assert_reserve_heap_size 1024
#define assert_reserve_ptrs_size 6
static char allocation_heap[2048 + assert_reserve_heap_size];
static void* allocation_ptrs[8 + assert_reserve_ptrs_size];
static unsigned int allocation_counter = 0;
static char* allocation_ptr = allocation_heap;

static void* malloc_bootstrap2(size_t size)
{
  static size_t _assert_reserve_heap_size = assert_reserve_heap_size;
  static size_t _assert_reserve_ptrs_size = assert_reserve_ptrs_size;
  if (allocation_counter > sizeof(allocation_ptrs) / sizeof(void*) - _assert_reserve_ptrs_size
      || allocation_ptr + size > allocation_heap + sizeof(allocation_heap) - _assert_reserve_heap_size)
  {
    _assert_reserve_heap_size = 0;
    _assert_reserve_ptrs_size = 0;
    assert(allocation_counter <= sizeof(allocation_ptrs) / sizeof(void*) - assert_reserve_ptrs_size);
    assert(allocation_ptr + size <= allocation_heap + sizeof(allocation_heap) - assert_reserve_heap_size);
  }
  void* ptr = allocation_ptr;
  allocation_ptrs[allocation_counter++] = ptr;
  allocation_ptr += size;
  return ptr;
}

static void free_bootstrap2(void* ptr)
{
  for (unsigned int i = 0; i < allocation_counter; ++i)
    if (allocation_ptrs[i] == ptr)
    {
      allocation_ptrs[i] = allocation_ptrs[allocation_counter - 1];
      allocation_ptrs[allocation_counter - 1] = NULL;
      if (--allocation_counter == 0 && libc_free_final) // Done?
        memleak_libc_free = libc_free_final;
      return;
    }
  (*libc_free_final)(ptr);
}

// This appears not to be called by dlopen/dlsym/dlclose, but lets keep it anyway.
static void* calloc_bootstrap2(size_t nmemb, size_t size)
{
  void* ptr = malloc_bootstrap2(nmemb * size);
  memset(ptr, 0, nmemb * size);
  return ptr;
}

// This appears not to be called by dlopen/dlsym/dlclose, but lets keep it anyway.
static void* realloc_bootstrap2(void* ptr, size_t size)
{
  // This assumes that allocations during dlopen()/dlclose()
  // never use the fact that decreasing an allocation is
  // garanteed not to relocate it.
  void* res = malloc_bootstrap2(size);
  free_bootstrap2(ptr);
  return res;
}

static void init_malloc_function_pointers(void)
{
  // Point functions to next phase.
  memleak_libc_malloc = malloc_bootstrap2;
  libc_calloc = calloc_bootstrap2;
  libc_realloc = realloc_bootstrap2;
  memleak_libc_free = free_bootstrap2;
  void* (*libc_malloc_tmp)(size_t size);
  void* (*libc_calloc_tmp)(size_t nmemb, size_t size);
  void* (*libc_realloc_tmp)(void* ptr, size_t size);
  void (*libc_free_tmp)(void* ptr);
  libc_malloc_tmp = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
  assert(libc_malloc_tmp);
  libc_calloc_tmp = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  assert(libc_calloc_tmp);
  libc_realloc_tmp = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
  assert(libc_realloc_tmp);
  libc_free_tmp = (void (*)(void*))dlsym(RTLD_NEXT, "free");
  assert(libc_free_tmp);
  memleak_libc_malloc = libc_malloc_tmp;
  libc_calloc = libc_calloc_tmp;
  libc_realloc = libc_realloc_tmp;
  libc_posix_memalign = (int (*)(void**, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
  assert(libc_posix_memalign);
  if (allocation_counter == 0)  // Done?
    memleak_libc_free = libc_free_tmp;
  else
    // There are allocations left, we have to check
    // every free to see if it's one that was
    // allocated here... until it is finally freed.
    libc_free_final = libc_free_tmp;
  init();
}

// Very first time that malloc(3) or calloc(3) are called; initialize the function pointers.

static void* malloc_bootstrap1(size_t size)
{
  init_malloc_function_pointers();
  return (*memleak_libc_malloc)(size);
}

static void* calloc_bootstrap1(size_t nmemb, size_t size)
{
  init_malloc_function_pointers();
  return (*libc_calloc)(nmemb, size);
}

//---------------------------------------------------------------------------------------------
// Global stats

struct BacktraceEntry;

struct Stats {
  size_t total_memory;
  size_t allocations;
  size_t backtraces;
  time_t oldest_interval_end;
  int recording;
  int max_backtraces;
  struct BacktraceEntry* first_entry;
  struct BacktraceEntry* first_entry_n;
};

typedef struct Stats Stats;

static Stats stats;
static time_t application_start;

//---------------------------------------------------------------------------------------------
// Header and Interval

#include "Header.h"
#include "Interval.h"

static time_t interval_start;

void interval_print(Interval const* interval)
{
  printf("[%4lu,", interval->start);
  if (interval->end)
    printf("%4lu>(%4lu)", interval->end, interval->end - interval->start);
  else
    printf("now");
  printf(": %5lu allocations (%6lu total, %4.1f%%), size %7lu; %6.2f allocations/s, %lu bytes/s\n",
      interval->n, interval->total_n, (100.0 * interval->n / interval->total_n), interval->size,
      (double)interval->n / (interval->end - interval->start),
      interval->size / (interval->end - interval->start));
}

#ifdef DEBUG_EXPENSIVE
static void check_backtrace_headers(struct BacktraceEntry* entry);
static void check_interval_headers(struct BacktraceEntry* entry);
static void check_intervals(BacktraceEntry* entry);
static void check_interval_first(struct Interval* interval);
#endif

//---------------------------------------------------------------------------------------------
// Backtrace

#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define LIKELY(x) __builtin_expect(!!(x), 1)

#include "BacktraceEntry.h"

static void interval_link(BacktraceEntry* entry, Interval* interval)
{
  interval->prev = NULL;
  if ((interval->next = entry->intervals))
    interval->next->prev = interval;
  entry->intervals = interval;
}

static void interval_unlink(BacktraceEntry* entry, Interval* interval)
{
  if (interval->prev)
    interval->prev->next = interval->next;
  else
    entry->intervals = interval->next;
  if (interval->next)
    interval->next->prev = interval->prev;
}

static void interval_add(Interval* interval, Header* header)
{
  assert(stats.recording);
  assert(header->time >= interval->start && (interval->end == 0 || header->time < interval->end));
  if (interval->n == 0)
  {
    assert(interval->first == NULL);
    interval->first = header;
#ifdef DEBUG_VERBOSE
    printf("Backtrace: %p: interval [%lu - %lu>(%lu); added first = header = %p[%lu] (header->prev = %p; &header->backtrace->head = %p)\n",
    	header->backtrace, interval->start, interval->end, interval->end - interval->start, header, header->time, header->prev, &header->backtrace->head);
#endif
    assert(header->next == &header->backtrace->head || header->next->time < interval->start);
  }
#ifdef DEBUG_VERBOSE
  else
  {
    printf("Backtrace: %p: interval [%lu - %lu>(%lu); interval->n = %lu; added header %p[%lu] (&header->backtrace->head = %p",
    	header->backtrace, interval->start, interval->end, interval->end - interval->start, interval->n + 1, header, header->time, &header->backtrace->head);
    Header* h = interval->first;
    int cnt = 0;
    for(;;)
    { 
      printf("; first");
      if (cnt > 0)
	printf("->prev");
      if (cnt > 1)
	printf("(x%d)", cnt);
      printf(" = %p", h);
      if (h == &header->backtrace->head)
	break;
      printf("[%lu]", h->time);
      h = h->prev;
      ++cnt;
    }
    printf(")\n");
  }
#endif
  interval->total_n += 1;
  interval->n += 1;
  interval->size += header->size;
  header->interval = interval;
}

static void interval_del(Interval* interval, Header* header, time_t life_time __attribute__((unused)))
{
  assert(interval->n > 0);
  interval->n -= 1;
#ifdef DEBUG_VERBOSE
  printf("Backtrace %p: deleting header %p; interval->n is now %lu; &header->backtrace->head = %p; interval->first = %p\n", header->backtrace, header, interval->n, &header->backtrace->head, interval->first);
  Header* h = interval->first;
  int cnt = 0;
  for(;;)
  {
    printf("; first");
    if (cnt > 0)
      printf("->prev");
    if (cnt > 1)
      printf("(x%d)", cnt);
    printf(" = %p", h);
    if (h == &header->backtrace->head)
      break;
    h = h->prev;
    ++cnt;
  }
#endif
  interval->size -= header->size;
  if (interval->first == header)
  {
    interval->first = header->prev;
#ifdef DEBUG_VERBOSE
    printf("  interval->first == header; setting 'first' to %p\n", interval->first);
#endif
  }
  if (interval->n == 0)
  {
    assert(interval->first == &header->backtrace->head || !interval->first->interval || interval->first->interval->start > header->time);
#ifdef DEBUG_VERBOSE
    printf("  interval->n == 0; setting 'first' to NULL\n");
#endif
    interval->first = NULL;
    if (interval->end)
    {
      interval_unlink(header->backtrace, interval);
      if (header->backtrace->recording_interval == interval)
	header->backtrace->recording_interval = NULL;
      (*memleak_libc_free)(interval);
    }
  }
}

// Combine this interval with the one that comes after it.
static void interval_combine(BacktraceEntry* entry, Interval* interval)
{
#ifdef DEBUG_EXPENSIVE
  check_interval_first(interval);
#endif

  assert(interval && interval->prev);
  assert(interval->end);
  assert(interval->prev->start == interval->end);
#ifdef DEBUG_VERBOSE
  printf("Backtrace #%-2d: Combining [%4lu,%4lu>(%lu) with [%4lu,%4lu>(%lu).\n", entry->backtrace_nr,
      interval->start, interval->end, interval->end - interval->start,
      interval->prev->start, interval->prev->end, interval->prev->end - interval->prev->start);
#endif
  Interval* delinked_interval = interval->prev;

#ifdef DEBUG_EXPENSIVE
  check_interval_first(delinked_interval);
  check_intervals(entry);
#endif

  interval_unlink(entry, delinked_interval);
  interval->end = delinked_interval->end;
  interval->total_n += delinked_interval->total_n;
  interval->n += delinked_interval->n;
  interval->size += delinked_interval->size;
  assert((delinked_interval->first == NULL && delinked_interval->n == 0) || (delinked_interval->first != NULL && delinked_interval->n > 0));
#ifdef DEBUG_EXPENSIVE
  if (delinked_interval->first && interval->first)
  {
    Header* header = interval->first;
    while (header->interval == interval)
      header = header->prev;
    assert(header->interval == delinked_interval);
    assert(delinked_interval->first == header);
  }
#endif

  if (delinked_interval->first)
    // Use the fact that BacktraceEntry::head.interval == NULL.
    for (Header* header = delinked_interval->first; header->interval == delinked_interval; header = header->prev)
      header->interval = interval;
  if (!interval->first)
    interval->first = delinked_interval->first;

#ifdef DEBUG_EXPENSIVE
  check_interval_first(interval);
  check_intervals(entry);
#endif

  // Only combine when there are three the same, so this is never at the top.
  assert(entry->recording_interval != delinked_interval);
  assert(entry->intervals != delinked_interval);

  (*memleak_libc_free)(delinked_interval);
}

static BacktraceEntry* hashtable[0x100000];

static int equal(BacktraceEntry* bp, void** backtrace, int backtrace_size)
{
  if (UNLIKELY(bp->backtrace_size != backtrace_size))
    return 0;
  for (int i = 0; i < backtrace_size; ++i)
    if (UNLIKELY(bp->ptr[i] != backtrace[i]))
      return 0;
  return 1;
}

static BacktraceEntry* update_entry_add(void** backtrace, int backtrace_size)
{
  intptr_t hash = backtrace_size;
  for (int i = 0; i < backtrace_size; ++i)
  {
    intptr_t sp = (intptr_t)backtrace[i];
    hash += sp;
  }
  hash *= hash;
  hash >>= 8;
  hash &= 0xfffff;
  BacktraceEntry* bp;
  BacktraceEntry** bpp;
  static int depth;
  int depth_cnt = 0;
  for (bpp = &hashtable[hash];; bpp = &(*bpp)->hashnext)
  {
    if (++depth_cnt > depth)
    {
      depth = depth_cnt;
      printf("Max hash depth %d\n", depth);
    }
    if (UNLIKELY(!(bp = *bpp)))
    {
      bp = *bpp = (*libc_calloc)(sizeof(BacktraceEntry), 1);
      memcpy(bp->ptr, backtrace, backtrace_size * sizeof(void*));
      bp->backtrace_size = backtrace_size;
      bp->next = stats.first_entry;
      bp->next_n = stats.first_entry_n;
      stats.first_entry = bp;
      stats.first_entry_n = bp;
      bp->backtrace_nr = ++stats.backtraces;
      bp->head.prev = &bp->head;
      bp->head.next= &bp->head;
      break;
    }
    else if (LIKELY(equal(bp, backtrace, backtrace_size)))
      break;
  }
  ++(bp->allocations);
  return bp;
}

static void update_interval_add(Header* header)
{
  BacktraceEntry* bp = header->backtrace;
  Interval* interval = bp->recording_interval;
  if (stats.recording && !interval)
  {
#if 0
    if (bp->intervals && header->time < bp->intervals->end)
      interval = bp->recording_interval = bp->intervals;
    else
#endif
    {
      interval = bp->recording_interval = (*libc_calloc)(1, sizeof(Interval));
      interval_link(bp, interval);
    }
    interval->start = interval_start;
    // Pick up all allocations already done in the last second, before recording started.
    Header* h = bp->head.next->next;
    while (h != &bp->head && h->time == interval_start)
      h = h->next;
    h = h->prev;
    while (h != header)
    {
      interval_add(interval, h);
      h = h->prev;
    }
  }
  // interval_start can be one second larger than now (when restarting recording) because we
  // never want to have overlapping intervals. If that is the case then we need to record
  // this allocation in the previous interval.
  while (interval && header->time < interval->start)
    interval = interval->next;
  if (interval && (interval->end == 0 || header->time < interval->end))
    interval_add(interval, header);
  else if (interval && stats.recording)
  {
    // If we are recording, but still didn't record this allocation
    // then that means that the new interval didn't start yet but
    // the previous interval ended before now: there is a gap between
    // the intervals because no allocation was made for some time.
    assert(interval->end && header->time >= interval->end);
    interval = interval->prev;
    assert(interval && header->time < interval->start);
    assert(interval->start - header->time == 1);
    interval->start = header->time;
    interval_add(interval, header);
  }
}

static void update_entry_del(Header* header)
{
  BacktraceEntry* bp = header->backtrace;
  assert(bp->allocations > 0);
  --(bp->allocations);
}

static void update_interval_del(Header* header)
{
  Interval* interval = header->interval;
  header->interval = NULL;
  if (interval)
  {
    assert(stats.recording || interval->end != 0);
    assert((interval->end == 0 || header->time < interval->end) && header->time >= interval->start);
    struct timeval tm;
    gettimeofday(&tm, NULL);
    interval_del(interval, header, tm.tv_sec - application_start - header->time);
  }
}

//---------------------------------------------------------------------------------------------
// Our administration

static void* const MAGIC_NUMBER = (void*)0x1234FDB90102ACDCUL;
static void* const MAGIC_MEMLEAK_STATS = (void*)0x12129a9ab91f02a3UL;

static pthread_mutex_t memleak_mutex = PTHREAD_MUTEX_INITIALIZER;

static long pagesize;
static char exename[256];
static char* appname;

static void* monitor(void*);
static pthread_t monitor_thread;

static void init(void)
{
  // This is used in Header. Just check it here to be sure.
  assert(sizeof(intptr_t) == sizeof(void*));
  pagesize = sysconf(_SC_PAGESIZE);
  struct timeval tm;
  gettimeofday(&tm, NULL);
  application_start = tm.tv_sec;
  ssize_t exenamelen = readlink("/proc/self/exe", exename, sizeof(exename));
  exename[exenamelen] = 0;
  appname = strrchr(exename, '/');
  if (appname)
    ++appname;
  else
    appname = exename;
  //printf("exename = \"%s\"\n", exename);
  addr2line_init();
  pthread_create(&monitor_thread, NULL, &monitor, NULL);
  stats.max_backtraces = 4;
  if (unsetenv("LD_PRELOAD") == -1)
    fprintf(stderr, "Failed to unset LD_PRELOAD: %s\n", strerror(errno));
}

static __thread int inside_memleak_stats = 0;

static void add(Header* header, size_t size, void** backtrace, int backtrace_size, size_t offset)
{
  if (UNLIKELY(inside_memleak_stats))
  {
    header->magic_number = MAGIC_MEMLEAK_STATS;
    header->posix_memalign_offset = offset;
    return;
  }
  struct timeval tm;
  gettimeofday(&tm, NULL);
  header->posix_memalign_offset = offset;
  header->size = size;
  pthread_mutex_lock(&memleak_mutex);
  header->backtrace = update_entry_add(backtrace, backtrace_size);
#ifdef DEBUG_EXPENSIVE
  --(header->backtrace->allocations);
  check_intervals(header->backtrace);
  ++(header->backtrace->allocations);
#endif
  header->prev = &header->backtrace->head;
  header->next = header->backtrace->head.next;
  header->prev->next = header->next->prev = header;
  stats.total_memory += size;
  ++stats.allocations;
  header->interval = NULL;
  header->time = tm.tv_sec - application_start;
  header->magic_number = MAGIC_NUMBER;
#ifdef DEBUG_EXPENSIVE
  check_backtrace_headers(header->backtrace);
#endif
  update_interval_add(header);
#ifdef DEBUG_EXPENSIVE
  check_interval_headers(header->backtrace);
  check_intervals(header->backtrace);
#endif
  pthread_mutex_unlock(&memleak_mutex);
}

static void del(Header* header)
{
  if (UNLIKELY(header->magic_number == MAGIC_MEMLEAK_STATS))
  {
    header->magic_number = (void*)0xf3ee;
    return;
  }
  assert(header->magic_number == MAGIC_NUMBER);
  pthread_mutex_lock(&memleak_mutex);
#ifdef DEBUG_EXPENSIVE
  check_intervals(header->backtrace);
  check_backtrace_headers(header->backtrace);
#endif
  header->magic_number = (void*)0x123;
  update_interval_del(header);
  stats.total_memory -= header->size;
  --stats.allocations;
  header->prev->next = header->next;
  header->next->prev = header->prev;
  update_entry_del(header);
#ifdef DEBUG_EXPENSIVE
  check_interval_headers(header->backtrace);
#endif
  header->magic_number = (void*)0x1111fbee;
#ifdef DEBUG_EXPENSIVE
  check_intervals(header->backtrace);
#endif
  pthread_mutex_unlock(&memleak_mutex);
  return;
}

struct memleak_stats_helper {
  BacktraceEntry* entry;
  Interval interval;
};

typedef struct memleak_stats_helper memleak_stats_helper;

time_t interval_class(time_t interval)
{
  time_t v = interval;
  v >>= 1;
  v += interval;
  v >>= 1;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  ++v;
  return v;
}

void memleak_stats(void)
{
  // Do not record memory allocated from this function.
  inside_memleak_stats = 1;

  // Record the moment at which this function is called (mostly, copying stats).
  struct timeval tm;
  gettimeofday(&tm, NULL);
  time_t now = tm.tv_sec - application_start;

  // LOCK ADMINISTRATIVE DATA
  pthread_mutex_lock(&memleak_mutex);

  // Make a local copy of the stats.
  Stats local_stats;
  memcpy(&local_stats, &stats, sizeof(Stats));

  // Run over all backtraces and their intervals and combine intervals as needed.
  // Determine the sorting value of each backtrace from it's Intervals.
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
    Interval* interval = entry->intervals;
    int combine_count = 0;
    time_t combine_class = 0;
    size_t value_n = 0;
    time_t last_ivc = 100;	// Big
    while (interval)
    {
      time_t ivc = interval_class(interval->end - interval->start);
      if (LIKELY(ivc > combine_class))
      {
	combine_class = ivc;
	combine_count = 1;
      }
      else if (++combine_count == 3 || UNLIKELY(ivc < combine_class))
      {
	if (interval->prev->start == interval->end)
	{
	  // Combine interval->prev with interval.
	  interval_combine(entry, interval);
	  ivc = interval_class(interval->end - interval->start);
	}
	else
	{
	  // There is a hole between interval->end and interval->prev->start.
	  // For example, interval is [50, 65> and interval->prev is [84, 100> (so ivc == 16).
	  time_t new_end = interval->end + ivc;		// 65 + 16 = 81
	  // Close the hole.
	  interval->end = interval->prev->start;	// [50, 84>
	  // If the hole is larger than the class of interval
	  if (new_end < interval->prev->start)
	  {
	    // Then only gobble up that much.
	    interval->end = new_end;			// [50, 81>
	    // If the remaining hole is of a class smaller then the current class,
	    // then add it to interval->prev.
	    if (interval_class(interval->prev->start - interval->end) < ivc)
	      interval->prev->start = interval->end;	// [81, 100>
	  }
	}
	ivc = combine_class = interval_class(interval->end - interval->start);
	combine_count = 1;
      }
      // Determine the weight of this backtrace.
      if (interval->end)
      {
	if (last_ivc < ivc)
	  value_n *= 2;
	value_n += interval->n;
      }
      // Next (older) interval.
      interval = interval->next;
      last_ivc = ivc;
    }
    entry->value_n = value_n;
  }

  // Remember what is currently the first node.
  BacktraceEntry* first_node_n = stats.first_entry_n;

  // UNLOCK ADMINISTRATIVE DATA
  pthread_mutex_unlock(&memleak_mutex);

  // Sort the backtraces.
  local_stats.first_entry_n = sort_n(local_stats.first_entry_n, NULL);

  // LOCK ADMINISTRATIVE DATA
  pthread_mutex_lock(&memleak_mutex);

  // Find the node pointing to the previous first node (new nodes can have been inserted).
  BacktraceEntry** entry_ptr_n = &stats.first_entry_n;
  while(*entry_ptr_n != first_node_n)
    entry_ptr_n = &(*entry_ptr_n)->next_n;
  // Set it to the start of the sorted linked list.
  *entry_ptr_n = local_stats.first_entry_n;

  // Count number of intervals.
  int intervals = 0;
  int count = 0;
  for(BacktraceEntry* entry = local_stats.first_entry_n; entry && count < stats.max_backtraces; entry = entry->next_n)
  {
    int has_interval = 0;
    Interval* interval = entry->intervals;
    while (interval)
    {
      // Skip not-so-interesting "leaks".
      if (interval->n > 1 && interval->end)
      {
	++intervals;
	has_interval = 1;
      }
      interval = interval->next;
    }
    if (has_interval)
      ++count;
  }

  // Make a copy of all the Interval objects that we want to print.
  memleak_stats_helper* helper = (*memleak_libc_malloc)(intervals * sizeof(memleak_stats_helper));
#ifdef DEBUG_EXPENSIVE
  memset(helper, 0x13, intervals * sizeof(memleak_stats_helper));
#endif
  intervals = 0;
  count = 0;		// Print at most 4 backtraces.
  for(BacktraceEntry* entry = local_stats.first_entry_n; entry && count < stats.max_backtraces; entry = entry->next_n)
  {
    int has_interval = 0;
    Interval* interval = entry->intervals;
    while (interval)
    {
      // Skip not-so-interesting "leaks".
      if (interval->n > 1 && interval->end)
      {
	helper[intervals].entry = entry;
	memcpy(&helper[intervals].interval, interval, sizeof(Interval));
	++intervals;
	has_interval = 1;
      }
      interval = interval->next;
    }
    if (has_interval)
      ++count;
  }

  // UNLOCK ADMINISTRATIVE DATA
  pthread_mutex_unlock(&memleak_mutex);

  // Print a header.
  char total[256];
  long totm = local_stats.total_memory;
  char* p = &total[sizeof(total)];
  *--p = 0;
  int count2 = 0;
  while (totm > 0)
  {
    if (count2 && count2 % 3 == 0)
      *--p = ',';
    *--p = '0' + (totm % 10);
    totm /= 10;
    ++count2;
  }
  fprintf(stdout, "%s: Now: %lu; \tBacktraces: %lu; \tallocations: %lu; \ttotal memory: %s bytes.\n",
      appname, now, local_stats.backtraces, local_stats.allocations, p);

  // Print all intervals and mark the backtrace entries as needing printing.
  time_t oldest_interval_end = 10000000;
  for(int i = 0; i < intervals; ++i)
  {
    helper[i].entry->need_printing = 1;
    fprintf(stdout, " backtrace %d (value_n: %6.2f); ", helper[i].entry->backtrace_nr, helper[i].entry->value_n);
    interval_print(&helper[i].interval);
    if (helper[i].interval.end < oldest_interval_end)
      oldest_interval_end = helper[i].interval.end;
  }
  (*memleak_libc_free)(helper);
  fflush(stdout);

  // LOCK ADMINISTRATIVE DATA
  pthread_mutex_lock(&memleak_mutex);

  stats.oldest_interval_end = oldest_interval_end;

  // Count the number of entries that need to be written to file.
  int entries = 0;
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
    entries += (entry->need_printing && !entry->printed) ? 1 : 0;

  // Make a copy of the BacktraceEntry's.
  BacktraceEntry* backtraces = (*memleak_libc_malloc)(entries * sizeof(BacktraceEntry));
  entries = 0;
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
    if (!entry->need_printing || entry->printed)
      continue;
    memcpy(&backtraces[entries], entry, sizeof(BacktraceEntry));
    entry->printed = 1;
    ++entries;
  }

  // UNLOCK ADMINISTRATIVE DATA
  pthread_mutex_unlock(&memleak_mutex);

  // Create or append 'memleak_backtraces' file.
  static int first_time = 1;
  FILE* fbacktraces = fopen("memleak_backtraces", first_time ? "w": "a");
  if (first_time)
  {
    fprintf(fbacktraces, "Application: \"%s\"\n", exename);
    first_time = 0;
  }
  // Write all marked entries to the file.
  for(int e = entries - 1; e >= 0; --e)
  {
    if (e && (e % 100) == 0)
    {
      fprintf(stdout, "%d backtraces to go (%3.1f %% cache hits)...\n", e, 100.0 * frame_cache_stats());
      fflush(stdout);
    }
    BacktraceEntry* entry = &backtraces[e];
    fprintf(fbacktraces, "Backtrace %d:\n", entry->backtrace_nr);
    addr2line_print(fbacktraces, entry->ptr, entry->backtrace_size);
  }
  fclose(fbacktraces);
  if (entries > 0)
    fprintf(stdout, "libmemleak: Wrote %d new backtraces.\n", entries);
  (*memleak_libc_free)(backtraces);

  // Done.
  inside_memleak_stats = 0;
}

//---------------------------------------------------------------------------------------------
// The hooks catching the malloc functions.

static __thread void* backtrace_buffer[backtrace_size_max];
static __thread int inside_backtrace = 0;
static __thread int inside_realloc = 0;

// Set HEADER_OFFSET to sizeof(Header) rounded up to the nearest multiple of sizeof(void*).
#define HEADER_OFFSET (((sizeof(Header) - 1) / sizeof(void*) + 1) * sizeof(void*))

void* malloc(size_t size)
{
  assert(!inside_realloc);
  void* allocation = (*memleak_libc_malloc)(size + HEADER_OFFSET);
  if (!allocation)
    return NULL;
#ifdef DEBUG_EXPENSIVE
  memset(allocation, 0xa5, sizeof(Header));
#endif
  // If malloc is called from inside backtrace, then cut the loop here and add the previous backtrace
  // for this allocation too (it DID cause this allocation after all).
  int backtrace_size = 0;
  if (!inside_backtrace)
  {
    inside_backtrace = 1;
    backtrace_size = backtrace(backtrace_buffer, backtrace_size_max);
    inside_backtrace = 0;
  }
  add((Header*)allocation, size, backtrace_buffer, backtrace_size, 0);
  allocation = (char*)allocation + HEADER_OFFSET;
  Debug(print_lock(); print("malloc("); print_size(size); print(") = "); print_ptr(allocation); print_unlock());
  return allocation;
}

void* calloc(size_t nmemb, size_t size)
{
  if (!nmemb || !size)
    return NULL;
  size_t alloc_nmemb = nmemb + (HEADER_OFFSET + size - 1) / size;
  void* allocation = (*libc_calloc)(alloc_nmemb, size);
  if (!allocation)
    return NULL;
#ifdef DEBUG_EXPENSIVE
  memset(allocation, 0xa6, sizeof(Header));
#endif
  int backtrace_size = 0;
  if (!inside_backtrace)
  {
    inside_backtrace = 1;
    backtrace_size = backtrace(backtrace_buffer, backtrace_size_max);
    inside_backtrace = 0;
  }
  add((Header*)allocation, nmemb * size, backtrace_buffer, backtrace_size, 0);
  allocation = (char*)allocation + HEADER_OFFSET;
  Debug(print_lock(); print("calloc("); print_size(nmemb); print(", "); print_size(size); print(") = "); print_ptr(allocation); print_unlock());
  return allocation;
}

void* realloc(void* void_ptr, size_t size)
{
  if (!void_ptr)
    return malloc(size);
  if (!size)
  {
    free(void_ptr);
    return NULL;
  }
  void_ptr = (char*)void_ptr - HEADER_OFFSET;
  del((Header*)void_ptr);
  inside_realloc = 1;
#ifdef DEBUG_EXPENSIVE
  memset(void_ptr, 0xf9, sizeof(Header));
#endif
  void* allocation = (*libc_realloc)(void_ptr, size + HEADER_OFFSET);
#ifdef DEBUG_EXPENSIVE
  assert(allocation);
  memset(allocation, 0xa7, sizeof(Header));
#endif
  inside_realloc = 0;
  if (!allocation)
  {
    // If realloc() fails the original block is left untouched; it is not freed or moved.
    // So we must revert the call to del() above.
    int backtrace_size = backtrace(backtrace_buffer, backtrace_size_max);
    add((Header*)void_ptr, ((Header*)void_ptr)->size, backtrace_buffer, backtrace_size, 0);
    return NULL;
  }
  int backtrace_size = 0;
  if (!inside_backtrace)
  {
    inside_backtrace = 1;
    backtrace_size = backtrace(backtrace_buffer, backtrace_size_max);
    inside_backtrace = 0;
  }
  add((Header*)allocation, size, backtrace_buffer, backtrace_size, 0);
  allocation = (char*)allocation + HEADER_OFFSET;
  Debug(print_lock(); print("realloc("); print_ptr(void_ptr); print(", "); print_size(size); print(") = "); print_ptr(allocation); print_unlock());
  return allocation;
}

void free(void* void_ptr)
{
  Debug(print_lock(); print("free("); print_ptr(void_ptr); print(")"); print_unlock());
  assert(!inside_realloc);
  if (!void_ptr)
    return;
  Header* header = (Header*)((char*)void_ptr - HEADER_OFFSET);
  del(header);
  void* tmp = (char*)void_ptr - (header->posix_memalign_offset ? header->posix_memalign_offset : (intptr_t)HEADER_OFFSET);
#ifdef DEBUG_EXPENSIVE
  memset(header, 0x19, sizeof(Header));
#endif
  (*memleak_libc_free)(tmp);
}

int posix_memalign(void** memptr, size_t alignment, size_t size)
{
  if (size == 0)
  {
    *memptr = NULL;
    return 0;
  }
  size_t offset = ((HEADER_OFFSET - 1) / alignment + 1) * alignment;
  int ret = (*libc_posix_memalign)(memptr, alignment, size + offset);
  if (ret != 0)
    return ret;
  int backtrace_size = 0;
  *memptr = (char*)*memptr + offset;
  Header* header = (Header*)((char*)*memptr - HEADER_OFFSET);
#ifdef DEBUG_EXPENSIVE
  memset(header, 0xc3, sizeof(Header));
#endif
  if (!inside_backtrace)
  {
    inside_backtrace = 1;
    backtrace_size = backtrace(backtrace_buffer, backtrace_size_max);
    inside_backtrace = 0;
  }
  add(header, size, backtrace_buffer, backtrace_size, offset);
  Debug(print_lock(); print("posix_memalign("); print_ptr(memptr); print(", "); print_size(alignment); print(", "); print_size(size);
        print(") = 0 (*memptr = "); print_ptr(*memptr); print(")"); print_unlock());
  return 0;
}

void* memalign(size_t boundary, size_t size)
{
  if (size == 0)
    size = 1;
  if (boundary < sizeof(void*))
  {
    assert(((boundary - 1) & boundary) == 0);	// Must be a power of two.
    boundary = sizeof(void*);
  }
  void* ret;
  if (posix_memalign(&ret, boundary, size) != 0)
    return NULL;
  return ret;
}

void* valloc(size_t size)
{
  return memalign(pagesize, size);
}

//---------------------------------------------------------------------------------------------

void delete_intervals(void);

void interval_start_recording(void)
{
  delete_intervals();
  struct timeval tm;
  gettimeofday(&tm, NULL);
  interval_start = tm.tv_sec - application_start;
  printf("*** START RECORDING ***\n");
  stats.recording = 1;
}

void interval_stop_recording(void)
{
  if (!stats.recording)
    return;

  struct timeval tm;
  gettimeofday(&tm, NULL);
  time_t interval_end = tm.tv_sec - application_start + 1;
  pthread_mutex_lock(&memleak_mutex);
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
    if (entry->recording_interval)
    {
      entry->recording_interval->end = interval_end;
      if (entry->recording_interval->n == 0)
      {
        interval_unlink(entry, entry->recording_interval);
	(*memleak_libc_free)(entry->recording_interval);
      }
      entry->recording_interval = NULL;
    }
  }
  stats.recording = 0;
  pthread_mutex_unlock(&memleak_mutex);
  printf("*** STOP RECORDING ***\n");
}

void interval_delete(time_t end)
{
  pthread_mutex_lock(&memleak_mutex);
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
#ifdef DEBUG_EXPENSIVE
    check_intervals(entry);
#endif
    Interval* interval = entry->intervals;
    if (!interval)
      continue;
    while (interval->next)
      interval = interval->next;
    while (interval && interval->end <= end)
    {
      Interval* prev = interval->prev;
      for (Header* header = interval->first; header && header->interval == interval; header = header->prev)
	header->interval = NULL;
      interval_unlink(entry, interval);
      if (entry->recording_interval == interval)
	entry->recording_interval = NULL;
      (*memleak_libc_free)(interval);
      interval = prev;
    }
#ifdef DEBUG_EXPENSIVE
    check_intervals(entry);
#endif
  }
  pthread_mutex_unlock(&memleak_mutex);
}

void delete_intervals(void)
{
  interval_stop_recording();
  pthread_mutex_lock(&memleak_mutex);
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
    Interval* interval = entry->intervals;
    while (interval)
    {
      Interval* next = interval->next;
      for (Header* header = interval->first; header && header->interval == interval; header = header->prev)
	header->interval = NULL;
      interval_unlink(entry, interval);
      (*memleak_libc_free)(interval);
      interval = next;
    }
    assert(entry->intervals == NULL);
  }
  pthread_mutex_unlock(&memleak_mutex);
}

void interval_restart_recording(void)
{
  if (!stats.recording)
  {
    interval_start_recording();
    return;
  }
  struct timeval tm;
  gettimeofday(&tm, NULL);
  time_t interval_end = tm.tv_sec - application_start + 1;
  pthread_mutex_lock(&memleak_mutex);
  for(BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
    if (entry->recording_interval)
    {
      entry->recording_interval->end = interval_end;
      if (entry->recording_interval->n == 0)
      {
        interval_unlink(entry, entry->recording_interval);
	(*memleak_libc_free)(entry->recording_interval);
      }
      entry->recording_interval = NULL;
    }
  }
  interval_start = interval_end;
  pthread_mutex_unlock(&memleak_mutex);
  printf("*** RESTART RECORDING ***\n");
}

static int quit = 0;
static void terminate(void);
static int sockfd;
static int fd = 0;
static char const* sockname;

static void* monitor(void* dummy __attribute__((unused)))
{
  if (strcmp(appname, "SLPlugin") == 0)
    pthread_exit(0);
  atexit(terminate);
  sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (sockfd < 0)
  {
    fprintf(stderr, "Failed to open AF_UNIX socket: %s\n", strerror(errno));
    pthread_exit(0);
  }
  sockname = getenv("LIBMEMLEAK_SOCKNAME");
  if (!sockname)
    sockname = "memleak_sock";
  printf("sockname = \"%s\"\n", sockname);
  struct sockaddr_un serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, sockname);
  int servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
  for(int try = 0; try < 3; ++try)
  {
    if (bind(sockfd, (struct sockaddr*)&serv_addr, servlen) < 0)
    {
      if (errno == EADDRINUSE)
      {
	unlink(sockname);
	continue;
      }
      fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
      close(sockfd);
      unlink(sockname);
      pthread_exit(0);
    }
    break;
  }
  if (listen(sockfd, 5) < 0)
  {
    fprintf(stderr, "Error listening on socket: %s\n", strerror(errno));
    close(sockfd);
    unlink(sockname);
    pthread_exit(0);
  }
  printf("libmemleak: Listening on \"%s\".\n", sockname);

  int count = 0;
  int restart_multiplier = 5;
  char const*  restart_multiplier_str = getenv("LIBMEMLEAK_RESTART_MULTIPLIER");
  if (restart_multiplier_str)
    restart_multiplier = atoi(restart_multiplier_str);
  if (restart_multiplier < 2)
  {
    fprintf(stderr, "libmemleak: LIBMEMLEAK_RESTART_MULTIPLIER: invalid value. Restart multiplier must be at least 2.");
    close(sockfd);
    unlink(sockname);
    pthread_exit(0);
  }
  printf("libmemleak: Restart multiplier set to %d\n", restart_multiplier);
  char const* stats_interval_str = getenv("LIBMEMLEAK_STATS_INTERVAL");
  struct timeval sleeptime = { stats_interval_str ? atoi(stats_interval_str) : 1, 0 };
  printf("libmemleak: Printing memory statistics every %lu seconds.\n", sleeptime.tv_sec);
  for(;;)
  {
    // Only linux, select modifies timeout to be the remaining time, so initialize it here.
    struct timeval timeout = sleeptime;
    for(;;)
    {
      fd_set wfds, rfds;
      FD_ZERO(&wfds);
      FD_ZERO(&rfds);
      FD_SET(sockfd, &rfds);
      if (fd > 0)
	FD_SET(fd, &rfds);
      //FD_SET(fd, &wfds);  set if we need writing.
      int n;
      if ((n = select(FD_SETSIZE, &rfds, &wfds, NULL, stats.recording? &timeout : NULL)) < 0)
      {
	if (errno == EINTR)
	{
	  timeout = sleeptime;	// Man page says this is undefined after an error.
	  continue;
	}
	perror("select");
	close(sockfd);
	unlink(sockname);
	pthread_exit(0);
      }
      if (n == 0)		// Timed out?
	break;
      if (FD_ISSET(fd, &wfds))
      {
	// write here.
      }
      if (FD_ISSET(sockfd, &rfds))
      {
	struct sockaddr_un cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	fd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
	if (fd < 0)
	{
	  perror("accept");
	  close(sockfd);
	  unlink(sockname);
	  pthread_exit(0);
	}
	printf("libmemleak: Accepted a connection on \"%s\".\n", sockname);
	if (fd > 0)
	  write(fd, "PROMPT\n", 7);
      }
      if (FD_ISSET(fd, &rfds))
      {
	static char buf[81];
	int len = read(fd, buf, 80);
	if (len == -1)
	{
	  fprintf(stderr, "read: %s\n", strerror(errno));
	  close(sockfd);
	  unlink(sockname);
	  pthread_exit(0);
	}
	if (len == 0)
	{
	  printf("libmemleak: Closing socket \"%s\".\n", sockname);
	  close(fd);
	  fd = 0;
	  break;
	}
	buf[len] = 0;
	for(--len; len >= 0; --len)
	{
	  if (isspace(buf[len]))
	    buf[len] = 0;
	  else
	  {
	    ++len;
	    break;
	  }
	}
	if (len > 0)
	{
	  if (strcmp(buf, "help") == 0)
	  {
	    static char const* helptext[] = {
	      "help     : Print this help.\n",
	      "start    : Erase all intervals and start recording the first interval.\n",
	      "stop     : Stop recording.\n",
	      "restart  : Start a new interval. Keep, and possibly combine, previous intervals.\n",
	      "delete   : Delete the oldest interval.\n",
              "stats    : Print overview of backtrace with highest leak probability.\n",
	      "stats N  : Automatically print stats every N seconds (use 0 to turn off).\n",
	      "restart M: Automatically restart every N * M stats.\n",
	      "list N   : When printing stats, print only the first N backtraces.\n",
	      "dump N   : Print backtrace number N.\n"
	    };
	    for (size_t line = 0; line < sizeof(helptext) / sizeof(char*); ++line)
	      write(fd, helptext[line], strlen(helptext[line]));
	  }
	  else if ((!stats.recording && strcmp(buf, "start") == 0) ||
	           ( stats.recording && strcmp(buf, "restart") == 0))
	  {
	    timeout = sleeptime;
	    count = -1;
	    int len = snprintf(buf, sizeof(buf), "Auto restart interval is %d * %lu seconds.\n", restart_multiplier, sleeptime.tv_sec);
	    if (len > 80)
	      len = 80;
	    write(fd, buf, len);
	    if (fd > 0)
	      write(fd, "PROMPT\n", 7);
	    break;
	  }
	  else if (stats.recording && strcmp(buf, "stop") == 0)
	  {
	    interval_stop_recording();
	    write(fd, "Stopped.\n", 9);
	  }
	  else if (strcmp(buf, "delete") == 0)
	  {
            int len = snprintf(buf, sizeof(buf), "Deleting all intervals that end before %lu seconds since application start.\n", stats.oldest_interval_end);
	    if (len > 80)
	      len = 80;
	    write(fd, buf, len);
	    interval_delete(stats.oldest_interval_end);
	  }
	  else if (strcmp(buf, "stats") == 0)
	  {
	    if (fd > 0)
	      write(fd, "PROMPT\n", 7);
	    break;
	  }
	  else if (strncmp(buf, "stats ", 6) == 0)
	  {
	    int arg = atoi(buf + 6);
	    int len;
	    if (arg >= 1)
	    {
	      sleeptime.tv_sec = arg;
	      len = snprintf(buf, sizeof(buf), "Printing memory statistics every %lu seconds.\n", sleeptime.tv_sec);
	    }
	    else
	    {
	      len = snprintf(buf, sizeof(buf), "Interval between printing of stats must be at least 1 second.\n");
	    }
	    if (len > 80)
	      len = 80;
	    write(fd, buf, len);
	  }
	  else if (strncmp(buf, "restart ", 8) == 0)
	  {
	    int arg = atoi(buf + 8);
	    int len;
	    if (arg >= 2)
	    {
	      restart_multiplier = arg;
	      len = snprintf(buf, sizeof(buf), "Restart multiplier set to %d.\n", restart_multiplier);
	    }
	    else
	    {
	      len = snprintf(buf, sizeof(buf), "Restart multiplier must be at least 2.\n");
	    }
	    if (len > 80)
	      len = 80;
	    write(fd, buf, len);
	  }
	  else if (strncmp(buf, "list ", 5) == 0)
	  {
	    int arg = atoi(buf + 5);
	    int len;
	    if (arg >= 1)
	    {
	      stats.max_backtraces = arg;
	      if (stats.max_backtraces == 1)
		len = snprintf(buf, sizeof(buf), "Now printing only the first backtrace.\n");
	      else
		len = snprintf(buf, sizeof(buf), "Now printing the first %d backtraces.\n", stats.max_backtraces);
	    }
	    else
	    {
	      len = snprintf(buf, sizeof(buf), "Argument of list must be at least 1.\n");
	    }
	    if (len > 80)
	      len = 80;
	    write(fd, buf, len);
	  }
	  else if (strncmp(buf, "dump ", 5) == 0)
	  {
	    int arg = atoi(buf + 5);
	    pthread_mutex_lock(&memleak_mutex);

	    BacktraceEntry* entry = stats.first_entry_n;
	    while (entry && entry->backtrace_nr != arg)
	      entry = entry->next_n;

	    pthread_mutex_unlock(&memleak_mutex);
	    if (entry)
	    {
	      FILE* fp = fdopen(fd, "a");
	      addr2line_print(fp, entry->ptr, entry->backtrace_size);
	      fflush(fp);
	    }
            else
	    {
	      int len = snprintf(buf, sizeof(buf), "Backtrace %d doesn't exist.\n", arg);
	      if (len > 80)
		len = 80;
	      write(fd, buf, len);
	    }
	  }
	  else
	    write(fd, "Ignored.\n", 9);
	}
	if (fd > 0)
	  write(fd, "PROMPT\n", 7);
      }
    }
    ++count;
    if (count % restart_multiplier == 0)
      interval_restart_recording();
    if (quit)
      pthread_exit(0);
    memleak_stats();
  }
  return NULL;
}

static void terminate(void)
{
  if (sockfd > 0)
  {
    close(sockfd);
    unlink(sockname);
  }
  if (fd > 0)
  {
    write(fd, "QUIT\n", 5);
    close(fd);
    fd = 0;
  }
  quit = 1;
  pthread_join(monitor_thread, NULL);
  interval_stop_recording();
  printf("libmemleak: Final memleak stats:\n");
  memleak_stats();
}

#ifdef DEBUG_EXPENSIVE
//---------------------------------------------------------------------------------------------
// Tests.

static void __attribute__ ((unused)) check_backtrace_headers(BacktraceEntry* entry)
{
  struct timeval tm;
  gettimeofday(&tm, NULL);
  time_t t = tm.tv_sec - application_start;
  // Run over all headers of this backtrace.
  Header* end = &entry->head;
  int count = 0;
  Debug(print_lock(); print("Backtrace: "); print_ptr(entry); print(" current headers: head ("); print_ptr(end); print(") ->"));
  for (Header* h = end->next; h != end; h = h->next)
  {
    Debug(print(" "); print_ptr(h); print("["); print_size(h->time); print("] ->"));
    assert(h->next->prev == h);
    assert(h->prev->next == h);
    assert(h->backtrace == entry);
    assert(h->time <= t);		// Every next Header is older.
    t = h->time;
    assert(count < entry->allocations);
    ++count;
  }
  Debug(print(" head ("); print_ptr(end); print(")"); print_unlock());
  assert(entry->allocations == count);
}

static void __attribute__ ((unused)) check_interval_headers(BacktraceEntry* entry)
{
  if (stats.recording || entry->recording_interval)
  {
    struct timeval tm;
    gettimeofday(&tm, NULL);
    assert(!entry->recording_interval || !stats.recording || entry->recording_interval->start == interval_start || entry->recording_interval->start == interval_start - 1);
    time_t is = entry->recording_interval ? entry->recording_interval->start : stats.recording ? interval_start : 0;
    time_t ie = stats.recording ? (tm.tv_sec - application_start + 1) : entry->recording_interval ? entry->recording_interval->end : 0;
    Header* end = &entry->head;
    size_t count = 0;
    size_t count_equal = 0;
    Header* fh = NULL;
    for (Header* h = end->next; h != end; h = h->next)
    {
      assert(h->time <= is || entry->recording_interval);
      if (h->time >= is && h->time < ie)
      {
	fh = h;
	++count;
      }
      if (h->time == is)
	++count_equal;
    }
    assert((!entry->recording_interval && count == count_equal) || (entry->recording_interval && entry->recording_interval->n == count));
    // entry->recording_interval->first points to the oldest allocation that falls into the interval.
    assert(!entry->recording_interval || entry->recording_interval->first == fh);
  }
  Interval* interval = entry->intervals;
  assert(!interval || !interval->prev);
  assert(!entry->recording_interval || entry->recording_interval == interval);
  time_t prev_start = (time_t)-1;
  while(interval)
  {
    assert(!entry->recording_interval || interval == entry->recording_interval || interval->end);
    assert(interval != entry->recording_interval || interval == entry->intervals);
    assert((!interval->end || interval->start < interval->end) && (prev_start == -1 || interval->end <= prev_start));
    Interval* prev = interval;
    prev_start = prev->start;
    interval = interval->next;
    assert(!interval || interval->prev == prev);
  }
}

static void __attribute__ ((unused)) check_intervals(BacktraceEntry* entry)
{
  int header_count = 0;
  unsigned int count = 0;
  int found_interval = 0;
  int passed_intervals = 0;
  int last_interval = 0;
  Interval* prev_interval = NULL;
  time_t end = 0;
  for (Header* header = entry->head.prev; header != &entry->head; header = header->prev)
  {
    ++header_count;
    Interval* interval = header->interval;
    if (!found_interval && !interval)
      continue;
    found_interval = 1;
    if (!interval)
    {
      assert(!last_interval);
      passed_intervals = 1;
      assert(header->time >= end);
      continue;
    }
    assert(!passed_intervals);
    if (interval != prev_interval)
    {
      assert(interval->first == header);
      if (prev_interval)
      {
       assert(count == prev_interval->n);
       assert(prev_interval->end <= interval->start);
      }
      count = 0;
    }
    ++count;
    end = interval->end;
    if (!interval->end)
      last_interval = 1;
    assert(header->time >= interval->start && (last_interval || header->time < interval->end));
    prev_interval = interval;
  }
  assert(header_count == entry->allocations);
}

static void __attribute__ ((unused)) check_interval_first(Interval* interval)
{
  assert(!interval->first || interval->n > 0);
  assert(interval->first || interval->n == 0);
  if (interval->first)
  {
    assert(interval->first->interval == interval);
    assert(interval->first->next->interval != interval);
    unsigned int cnt = 0;
    for (Header* header = interval->first; header->interval == interval; header = header->prev)
      ++cnt;
    assert(cnt == interval->n);
  }
}
#endif

#ifdef DEBUG
void print_entry(BacktraceEntry* entry)
{
  printf("Entry %p; allocations: %d\n", entry, entry->allocations);
  printf("Oldest Header first:\n");
  int count = 0;
  for (Header* header = entry->head.prev; header != &entry->head; header = header->prev)
  {
    ++count;
    Interval* interval = header->interval;
    printf("%d: Header %p; time %lu; interval %p", count, header, header->time, interval);
    if (interval)
      printf(" [%lu, %lu>; interval->first = %p", interval->start, interval->end, interval->first);
    printf("\n");
  }
}

void find_interval(Interval* interval)
{
  // Run over all backtraces.
  for (BacktraceEntry* entry = stats.first_entry; entry; entry = entry->next)
  {
    if (entry->recording_interval == interval)
    {
      printf("recording_interval: backtrace %d @ %p\n", entry->backtrace_nr, entry);
    }
    if (entry->intervals == interval)
    {
      printf("intervals: backtrace %d @ %p\n", entry->backtrace_nr, entry);
    }
    // Run over all Headers
    for (Header* header = entry->head.next; header != &entry->head; header = header->next)
    {
      if (header->interval == interval)
      {
        printf("interval: Header @ %p, with backtrace %d @ %p\n", header, entry->backtrace_nr, entry);
      }
    }
    // Run over all Intervals
    for (Interval* ival = entry->intervals; ival; ival = ival->next)
    {
      if (ival->next == interval)
      {
        printf("Interval::next of Interval @ %p\n", ival);
      }
      if (ival->prev == interval)
      {
        printf("Interval::prev of Interval @ %p\n", ival);
      }
    }
  }
}
#endif

