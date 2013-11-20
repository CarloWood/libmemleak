// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file BacktraceEntry.h This file contains the declaration of struct BacktraceEntry.
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

#ifndef BACKTRACEENTRY_H
#define BACKTRACEENTRY_H

#include "Header.h"
#include "Interval.h"

//! @brief Maximum size of a backtrace.
#define backtrace_size_max 40

//! @brief Backtrace representation.
//
// Representation of the backtrace of some allocation.
struct BacktraceEntry {
  void* ptr[backtrace_size_max];                //!< The backtrace.
  int backtrace_size;                           //!< Number of valid pointers in 'ptr'.
  int allocations;                              //!< Number of current allocations with this backtrace.
  struct BacktraceEntry* next;                  //!< Next backtrace.
  struct BacktraceEntry* hashnext;              //!< Next backtrace with the same hash.
  int backtrace_nr;                             //!< Small unique ID assigned to this backtrace.
  int need_printing;                            //!< Set to 1 when this backtrace needs printing.
  int printed;                                  //!< Set to 1 when this backtrace was already printed.
  struct BacktraceEntry* next_n;		//!< Next backtace with a value_n that is less or equal.
  double value_n;				//!< Value used for sorting.
  Header head;                                  //!< Root of doubly linked list of all current allocations.
  Interval* recording_interval;			//!< The currently active (recording) Interval for this backtrace.
  Interval* intervals;				//!< A linked list of all Interval's related to this backtrace.
};

//! @brief Abbreviation for struct BacktraceEntry.
typedef struct BacktraceEntry BacktraceEntry;

#endif // BACKTRACEENTRY_H
