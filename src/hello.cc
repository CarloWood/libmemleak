#include <iostream>
#include <cstdlib>
#include <inttypes.h>
#include <deque>
#include <cstdio>

extern "C" {
#include "addr2line.h"
}

#ifdef DIRECT_LINKED
extern "C" void memleak_stats();
extern "C" void interval_restart_recording(void);
extern "C" void interval_stop_recording(void);
#endif

void do_work(bool leak);
void* thread_entry0(void*) { do_work(false); }
void* thread_entry1(void*) { do_work(true); }
void* thread_entry2(void*) { do_work(false); }
void* thread_entry3(void*) { do_work(false); }
void purge(void);
#ifdef DIRECT_LINKED
void* monitor(void*);
int quit = 0;
#endif

size_t leaked_mem = 0;

int main()
{
  std::cout << "Entering main()" << std::endl;
#ifdef DIRECT_LINKED
  memleak_stats();
#endif

  int const threads = 4;
  pthread_t thread[threads];

  for (int i = 0; i < threads; ++i)
  {
    switch(i)
    {
      case 0:
	pthread_create(&thread[0], NULL, &thread_entry0, NULL);
	break;
      case 1:
	pthread_create(&thread[1], NULL, &thread_entry1, NULL);
	break;
      case 2:
	pthread_create(&thread[2], NULL, &thread_entry2, NULL);
	break;
      case 3:
	pthread_create(&thread[3], NULL, &thread_entry3, NULL);
	break;
    }
  }

#ifdef DIRECT_LINKED
  pthread_t monitor_thread;
  pthread_create(&monitor_thread, NULL, &monitor, NULL);
#endif

  for (int i = 0; i < threads; ++i)
    pthread_join(thread[i], NULL);

  std::cout << "All threads exited. Purging and terminating monitor thread." << std::endl;
  purge();
#ifdef DIRECT_LINKED
  quit = 1;

  pthread_join(monitor_thread, NULL);
  interval_stop_recording();
#endif

  std::cout << "Deliberate number of missed calls to free(): " << leaked_mem << std::endl;
#ifdef DIRECT_LINKED
  memleak_stats();
#endif
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

std::deque<void*> allocations;

void store(void* ptr)
{
  if (!ptr)
    return;
  pthread_mutex_lock(&mutex);
  allocations.push_back(ptr);
  pthread_mutex_unlock(&mutex);
}

void* remove(void)
{
  void* ret = NULL;
  pthread_mutex_lock(&mutex);
  if (!allocations.empty())
  {
    ret = allocations.front();
    allocations.pop_front();
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

void purge(void)
{
  int count = 0;
  pthread_mutex_lock(&mutex);
  while (!allocations.empty())
  {
    free(allocations.front());
    allocations.pop_front();
    ++count;
  }
  pthread_mutex_unlock(&mutex);
  std::cout << "Purged " << count << " allocations." << std::endl;
}

void do_work(bool leak)
{
  struct random_data rdata;
  int32_t rvalue;
  char rstatebuf[256];

  initstate_r(0x1234aabc, rstatebuf, sizeof(rstatebuf), &rdata);

  for (int i = 0; i < 10000000; ++i)
  {
    random_r(&rdata, &rvalue);
    rvalue >>= 8;
    bool allocate = rvalue & 1;
    int how = (rvalue >> 1) & 0x3;
    size_t size = (rvalue >> 3) & 0xff;
    bool leak_memory = leak && ((rvalue >> 19) & 0xfff) == 0;

    if (!allocate)
    {
      void* ptr = remove();
      if (!leak_memory)
      {
	free(ptr);
      }
      else if (ptr)
	++leaked_mem;
    }
    else
    {
      if (how == 0)
	store(realloc(remove(), size));
      else if (how == 1)
	store(calloc(13, size / 13));
      else
	store(malloc(size));
    }
  }

  pthread_exit(0);
}

#ifdef DIRECT_LINKED
void* monitor(void*)
{
  int count = 0;
  for(;;)
  {
    sleep(1);
    ++count;
    if (count % 4 == 3)
      interval_restart_recording();
    if (quit)
      pthread_exit(0);
    memleak_stats();
  }
}
#endif
