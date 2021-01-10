// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file addr2line.h This file contains the declarations for addr2line.c.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <bfd.h>        // binutils 2.28 wants a config.h to be included first.

// Older versions of binutils (< 2.34) are using macros to access members
// of 'asection' structure. Since 2.34, it has been replace by static inline functions.
#if defined(bfd_get_section_flags)
// we must undefine old macros using a different prototype
#undef bfd_section_size
#undef bfd_section_vma
#undef bfd_section_flags
static inline bfd_size_type bfd_section_size(const asection *sec) { return sec->size; }
static inline bfd_vma       bfd_section_vma(const asection *sec) { return sec->vma; }
static inline flagword      bfd_section_flags(const asection *sec) { return sec->flags; }
#endif

#ifndef bool
//! @brief A boolean type.
#define bool int
#endif

//! @brief Administration of a BFD (executable or shared library).
//
// This class is used to decode addresses for a particular BFD to
// sourcefile, linenumber and function name.
struct Addr2Line {
  char* executable;		//!< Full path of the executable or shared library (as it appears in /proc/self/maps).
  bfd_vma some_vma;             //!< The virtual memory address of the section of the first symbol, if any. Otherwise 0.
  char* methodName;		//!< Demangled function name of decoded location.
  char const* fileName;		//!< Source file of decoded location.
  long line;			//!< Line number of decoded location.

  char const* _methodName;	//!< Unmangled function name.
  bfd* abfd;			//!< The underlaying bfd object.
  asymbol** syms;		//!< The symbol table of the bfd.
  bfd_vma pc;			//!< The address to decode.
  bool found;			//!< True if address could be decoded.
  bool needFree;		//!< True if methodName is allocated on the stack.
};

//! @brief Abbreviation for struct Addr2Line
typedef struct Addr2Line Addr2Line;

//! @brief Initialize the use of addr2line.
//
// This functions reads /proc/self/maps, finds the begin and end
// of every executable code section with a related filename
// and calls addr2line_init_bfd for each of them, storing the
// resulting Addr2Line object pointer in a red/black tree
// for fast retrieval as function of a program pointer.
void addr2line_init();

//! @brief Print a backtrace with source file and line numbers.
void addr2line_print(FILE* fbacktraces, void** backtrace, size_t backtrace_size);

//! @brief Print number of cache hits.
double frame_cache_stats();
