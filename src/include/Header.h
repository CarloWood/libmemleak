// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file Header.h This file contains the declaration of struct Header.
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
#ifndef HEADER_H
#define HEADER_H

#include <stdint.h>

struct BacktraceEntry;
struct Interval;

//---------------------------------------------------------------------------------------------
// Header

//! @brief Allocation header.
//
// Each memory allocation is increased in size and has this data prepended.
struct Header {
  struct Header* prev;                          //!< Previous allocation with the same backtrace.
  struct Header* next;                          //!< Next allocation with the same backtrace.
  intptr_t size;                                //!< Size of the allocation (minus Header).
  intptr_t time;                                //!< Time at which the allocation was made (in seconds UTC).
  intptr_t posix_memalign_offset;               //!< The offset in case of a posix_memalign.
  struct BacktraceEntry* backtrace;             //!< Pointer to the backtrace that this allocation belongs to.
  struct Interval* interval;                    //!< Pointer to interval this allocation was made in, if any.
  void* magic_number;                           //!< Magic Number.
} __attribute__((__packed__));

//! @brief Abbreviation for struct Header.
typedef struct Header Header;

#endif // HEADER_H

