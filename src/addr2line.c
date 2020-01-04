// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file addr2line.c Print pretty backtraces.
//
// Copyright (C) 2010 - 2016, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
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

#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libiberty/demangle.h>
#include <assert.h>
#include <ctype.h>
#include <execinfo.h>
#include <stdarg.h>

#include "addr2line.h"
#include "rb_tree/red_black_tree.h"

// arnaudviala: the TARGET is defined in config.h (by ./configure script)
// #define TARGET "x86_64-pc-linux-gnu"

#define false 0
#define true 1

static rb_red_blk_tree* range_map;
static rb_red_blk_tree* frame_map;

#if HAVE_PRINTF_STYLE_BFD_ERROR_HANDLER_TYPE
static void addr2LineErrorHandler(char const* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}
#else
static void addr2LineErrorHandler(char const* fmt, va_list ap)
{
  vfprintf(stderr, fmt, ap);
}
#endif

static void addr2LinePrintErr(char const* string)
{
  char const* errmsg = bfd_errmsg(bfd_get_error());
  if (string)
    fprintf (stderr, "Addr2Line: %s: %s\n", string, errmsg);
  else
    fprintf (stderr, "Addr2Line: %s\n", errmsg);
}

static Addr2Line* addr2line_init_bfd(char const* executable)
{
  Addr2Line* self = malloc(sizeof(Addr2Line));
  if (!self)
    return NULL;
  self->executable = malloc(strlen(executable) + 1);
  strcpy(self->executable, executable);

  static int bfd_initialized = 0;
  if (!bfd_initialized)
  {
    bfd_initialized = 1;
    bfd_init();
    bfd_set_error_handler(addr2LineErrorHandler);
    if (!bfd_set_default_target(TARGET))
    {
      fprintf(stderr, "Can't set BFD default target to `%s': %s", TARGET, bfd_errmsg(bfd_get_error()));
      free(self);
      bfd_initialized = 0;
      return NULL;
    }
  }
  self->abfd = bfd_openr(self->executable, NULL);
  if (self->abfd == NULL)
  {
    addr2LinePrintErr(self->executable);
    free(self);
    return NULL;
  }
  if (bfd_check_format(self->abfd, bfd_archive))
  {
    fprintf(stderr, "%s: can not get addresses from archive", self->executable);
    bfd_close(self->abfd);
    free(self);
    return NULL;
  }
  char** matching;
  if (!bfd_check_format_matches(self->abfd, bfd_object, &matching))
  {
    addr2LinePrintErr(bfd_get_filename(self->abfd));
    if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
    {
//    list_matching_formats(matching);
      free(matching);
    }
    bfd_close(self->abfd);
    free(self);
    return NULL;
  }
  long storage;
  long symcount;
  if ((bfd_get_file_flags(self->abfd) & HAS_SYMS) == 0)
  {
    bfd_close(self->abfd);
    free(self);
    return NULL;
  }
  storage = bfd_get_symtab_upper_bound(self->abfd);
  if (storage < 0)
  {
    addr2LinePrintErr(bfd_get_filename(self->abfd));
    bfd_close(self->abfd);
    free(self);
    return NULL;
  }
  self->syms = (asymbol**)malloc(storage);
  symcount = bfd_canonicalize_symtab(self->abfd, self->syms);
  if (symcount < 0)
  {
    addr2LinePrintErr(bfd_get_filename(self->abfd));
    bfd_close(self->abfd);
    free(self->syms);
    free(self);
    return NULL;
  }
  self->some_vma = (symcount > 0) ? self->syms[0]->section->vma : 0;
  self->needFree = self->found = false;
  self->methodName = (char*)"??";
  self->fileName = NULL;
  self->line = 0;
  return self;
}

static void addr2line_close(Addr2Line* self)
{
  free(self->syms);
  if (self->abfd)
    bfd_close(self->abfd);
  free(self->executable);
  free(self);
}

struct Range {
  void const* begin;
  void const* end;
};

typedef struct Range Range;

static int is_no_pie(Addr2Line* self, Range* range)
{
  return (bfd_vma)range->begin <= self->some_vma && self->some_vma < (bfd_vma)range->end;
}

static int range_compare(void const* r1, void const* r2)
{
  if (((Range const* )r1)->begin >= ((Range const*)r2)->end)
    return 1;
  if (((Range const*)r1)->end <= ((Range const*)r2)->begin)
    return -1;
  return 0;
}

static void range_destroy(void* r1)
{
  (*memleak_libc_free)(r1);
}

static void range_print(void const* r1)
{
  printf("(Range){begin:%p, end:%p}",
      ((Range const*)r1)->begin, ((Range const*)r1)->end);
}

static void addr2line_print2(void const* a1)
{
  printf("(Addr2Line){methodName:\"%s\", fileName:\"%s\", line:%lu}",
      ((Addr2Line const*)a1)->methodName, ((Addr2Line const*)a1)->fileName, ((Addr2Line const*)a1)->line);
}

static int frame_compare(void const* f1, void const* f2)
{
  if (f1 < f2)
    return 1;
  else if (f1 > f2)
    return -1;
  return 0;
}

static void frame_destroy(void* f1 __attribute__((unused)))
{
}

static void framestr_destroy(void* f1)
{
  (*memleak_libc_free)(f1);
}

static void frame_print(void const* f1)
{
  printf("%p", f1);
}

static void framestr_print(void const* f1)
{
  printf("\"%s\"", (char const*)f1);
}

void addr2line_init()
{
  printf("Entering addr2line_init\n");
  range_map = RBTreeCreate(range_compare, range_destroy, (void(*)(void*))addr2line_close, range_print, addr2line_print2);
  FILE* fmaps = fopen("/proc/self/maps", "r");
  assert(fmaps);
  long offset = 0;
  char buf[512];
  buf[sizeof(buf) - 1] = 0;
  for(;;)
  {
    size_t rlen = fread(buf + offset, 1, sizeof(buf) - 1 - offset, fmaps);
    if (rlen == 0)
      break;
    char* line_start = buf;
    for (char* newline = strchr(buf + offset, '\n'); newline; line_start = newline + 1, newline = strchr(line_start, '\n'))
    {
      *newline = 0;
      char* mode = strchr(line_start, ' ');
      if (!mode)
	continue;
      if (strncmp(mode, " r-xp", 5) != 0)
	continue;
      char* filename = newline;
      while(*--filename != ' ');
      ++filename;
      //printf("*** Full line: \"%s\"\n", line_start);
      //printf("*** File name: \"%s\"\n", filename);
      if (*filename != '/')
	continue;
      unsigned long ebegin = 0;
      unsigned long eend = 0;
      char* digit = line_start - 1;
      while (*++digit != '-') { ebegin <<= 4; ebegin += *digit - (isdigit(*digit) ? '0' : (islower(*digit) ? 'a' : 'A') - 10); }
      while (*++digit != ' ') { eend <<= 4; eend += *digit - (isdigit(*digit) ? '0' : (islower(*digit) ? 'a' : 'A') - 10); }
      Range* range = (*memleak_libc_malloc)(sizeof(Range));
      range->begin = (void*)ebegin;
      range->end = (void*)eend;
      range_print(range); printf(" : %s\n", filename);
      RBTreeInsert(range_map, range, addr2line_init_bfd(filename));
    }
    if (rlen < sizeof(buf) - 1 - offset)
      break;
    offset = buf + sizeof(buf) - 1 - line_start;
    memmove(buf, line_start, offset);
  }
  fclose(fmaps);
  frame_map = RBTreeCreate(frame_compare, frame_destroy, framestr_destroy, frame_print, framestr_print);
}

static void find_address_in_section(bfd* abfd, asection* section, void* data)
{
  bfd_vma vma;
  bfd_size_type size;
  Addr2Line* x = (Addr2Line*)data;

  if (x->found)
    return;

  if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0)
    return;

  vma = bfd_get_section_vma(abfd, section);
  if (x->pc < vma)
    return;

  size = bfd_get_section_size(section);
  if (x->pc >= vma + size)
    return;

  x->found = bfd_find_nearest_line(abfd, section, x->syms, x->pc - vma, &x->fileName, &x->_methodName, (unsigned int*)&x->line);
}

static bool addr2line_lookup(Addr2Line* self, bfd_vma pc)
{
  if (self->needFree)
    free(self->methodName);

  self->pc = pc;
  self->found = false;

  self->needFree = false;
  self->methodName = (char*)"??";

  self->fileName = NULL;
  self->line = 0;

  if (self->abfd == NULL)
    return false;

  bfd_map_over_sections(self->abfd, find_address_in_section, self);
  if (self->found)
  {
    if(self->_methodName == NULL)
      self->methodName = (char*)"??";
    else
    {
      self->needFree = true;
      char* res = cplus_demangle(self->_methodName, DMGL_ANSI|DMGL_PARAMS);
      if (res == NULL)
      {
	size_t len = strlen(self->_methodName);
	if (len > 1024)
	  len = 1023;
	self->methodName = malloc(len + 1);
	strncpy(self->methodName, self->_methodName, len);
	self->methodName[len] = 0;
      }
      else
	self->methodName = res;
    }
    return true;
  }
  return false;
}

static int frame_cache_total = 0;
static int frame_cache_hits = 0;

double frame_cache_stats()
{
  double ret = (double)frame_cache_hits / frame_cache_total;
  frame_cache_total = frame_cache_hits = 0;
  return ret;
}

void addr2line_print(FILE* fbacktraces, void** backtrace, size_t backtrace_size)
{
  char** strs = NULL;
  for (unsigned int i = 0; i < backtrace_size; ++i)
  {
    void* addr = backtrace[i];

    ++frame_cache_total;
    rb_red_blk_node* node = RBExactQuery(frame_map, addr);
    if (node)
    {
      ++frame_cache_hits;
      fprintf(fbacktraces, " #%-2d %.16lx %s\n", i, (bfd_vma)addr, (char const*)(node->info));
      continue;
    }
    fprintf(fbacktraces, " #%-2d %.16lx ", i, (bfd_vma)addr);
    char* framestr;
    Range ptr;
    ptr.begin = addr;
    ptr.end = (char*)addr + 1;
    node = RBExactQuery(range_map, &ptr);
    Addr2Line* addr2line;
    if (node && (addr2line = (Addr2Line*)(node->info)))
    {
      Range* range = (Range*)node->key;
      // Do not substract the range begin when this isn't a Position Independent Executable.
      bool found = addr2line_lookup(addr2line, (bfd_vma)addr - (is_no_pie(addr2line, range) ? (bfd_vma)0 : (bfd_vma)range->begin));
      if (found)
      {
	size_t len = strlen(addr2line->methodName) + 4;
	if (addr2line->fileName)
	{
	  len += strlen(addr2line->fileName) + 4;
	  if (addr2line->line)
	    len += 8;	// :9999999 Maximal 10,000,000 lines per file.
	}
	framestr = (*memleak_libc_malloc)(len + 1);
	len = sprintf(framestr, " in %s", addr2line->methodName);
	if (addr2line->fileName && addr2line->line)
	  sprintf(framestr + len, " at %s:%ld", addr2line->fileName, addr2line->line);
	else if (addr2line->fileName)
	  sprintf(framestr + len, " at %s", addr2line->fileName);
      }
      else
      {
	char* lastslash = strrchr(addr2line->executable, '/');
	size_t len = strlen(lastslash + 1) + 6;
	framestr = (*memleak_libc_malloc)(len + 1);
	sprintf(framestr, " in \"%s\"", lastslash + 1);
      }
    }
    else
    {
      if (!strs)
	strs = backtrace_symbols(backtrace, backtrace_size);
      size_t len = strlen(strs[i]);
      framestr = (*memleak_libc_malloc)(len + 1);
      strcpy(framestr, strs[i]);
    }
    fprintf(fbacktraces, "%s\n", framestr);
    RBTreeInsert(frame_map, (void*)addr, framestr);
  }
  if (strs)
    free(strs);
}

#if 0
#include "sort.h"
Node* sort_n(Node* list, Node** last_out)
{
}

// Compile with:
// gcc -g -pthread -Iinclude addr2line.c memleak.c rb_tree/red_black_tree.c rb_tree/misc.c rb_tree/stack.c -lbfd -ldl
//
// To see the actual offset of an executable, run:
//
//   LD_TRACE_PRELINKING=1 ./a.out | grep '=>'
//
// If the offset changes when run multiple times, then Address Space Layout Randomization is on.
// You can turn this off temporarily with:
//
//   $ sudo sysctl -w kernel.randomize_va_space=0
//
// (and on again with =1).
// After that the offset repport with LD_TRACE_PRELINKING should be the same
// as the one printed by ./a.out.
//
// For example,
//
// sean:~/projects/libmemleak/libmemleak/src>sudo sysctl -w kernel.randomize_va_space=0
// kernel.randomize_va_space = 0
// sean:~/projects/libmemleak/libmemleak/src>LD_TRACE_PRELINKING=1 ./a.out | grep '=>'
//        ./a.out => ./a.out (0x0000555555554000, 0x0000555555554000) TLS(0x1, 0x0000000000000150)
//        libgtk3-nocsd.so.0 => /usr/lib/x86_64-linux-gnu/libgtk3-nocsd.so.0 (0x00007ffff7bce000, 0x00007ffff7bce000)
//        libbfd-2.30-system.so => /usr/lib/x86_64-linux-gnu/libbfd-2.30-system.so (0x00007ffff787e000, 0x00007ffff787e000)
//        libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007ffff767a000, 0x00007ffff767a000)
//        libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007ffff745b000, 0x00007ffff745b000)
//        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007ffff706a000, 0x00007ffff706a000) TLS(0x2, 0x00000000000001e0)
//        libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007ffff6e4d000, 0x00007ffff6e4d000)
//        /lib64/ld-linux-x86-64.so.2 => /lib64/ld-linux-x86-64.so.2 (0x00007ffff7dd5000, 0x00007ffff7dd5000)
// sean:~/projects/libmemleak/libmemleak/src>./a.out
// Entering addr2line_init
// (Range){begin:0x555555554000, end:0x55555555d000} : /home/carlo/projects/libmemleak/libmemleak/src/a.out
// (Range){begin:0x7ffff6c35000, end:0x7ffff6c4c000} : /lib/x86_64-linux-gnu/libgcc_s.so.1
// (Range){begin:0x7ffff6e4d000, end:0x7ffff6e69000} : /lib/x86_64-linux-gnu/libz.so.1.2.11
// (Range){begin:0x7ffff706a000, end:0x7ffff7251000} : /lib/x86_64-linux-gnu/libc-2.27.so
// (Range){begin:0x7ffff745b000, end:0x7ffff7475000} : /lib/x86_64-linux-gnu/libpthread-2.27.so
// (Range){begin:0x7ffff767a000, end:0x7ffff767d000} : /lib/x86_64-linux-gnu/libdl-2.27.so
// (Range){begin:0x7ffff787e000, end:0x7ffff79ae000} : /usr/lib/x86_64-linux-gnu/libbfd-2.30-system.so
// (Range){begin:0x7ffff7bce000, end:0x7ffff7bd4000} : /usr/lib/x86_64-linux-gnu/libgtk3-nocsd.so.0
// (Range){begin:0x7ffff7dd5000, end:0x7ffff7dfc000} : /lib/x86_64-linux-gnu/ld-2.27.so
// sockname = "/home/carlo/projects/libmemleak/libmemleak-objdir/src/memleak_sock"
// libmemleak: Listening on "/home/carlo/projects/libmemleak/libmemleak-objdir/src/memleak_sock".
// libmemleak: Restart multiplier set to 2
// libmemleak: Printing memory statistics every 1 seconds.
// #0  0000555555556f20  in main at /home/carlo/projects/libmemleak/libmemleak/src/addr2line.c:426
//
int main()
{
  Addr2Line* addr2line = addr2line_init_bfd("/proc/self/exe");
  if (addr2line)
  {
    void* backtrace[1] = { main };
    addr2line_print(stdout, backtrace, 1);
    addr2line_close(addr2line);
  }
}
#endif
