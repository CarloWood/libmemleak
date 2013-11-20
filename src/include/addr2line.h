// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file addr2line.h This file contains the declarations for addr2line.c.
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

#include <bfd.h>

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
void addr2line_init(void);

//! @brief Print a backtrace with source file and line numbers.
void addr2line_print(FILE* fbacktraces, void** backtrace, size_t backtrace_size);

//! @brief Print number of cache hits.
double frame_cache_stats(void);
