// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file Interval.h This file contains the declaration of struct Interval.
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

#ifndef INTERVAL_H
#define INTERVAL_H

#include <sys/types.h>
#include <sys/time.h>

//---------------------------------------------------------------------------------------------
// Interval

//! @brief Backtrace specific Time interval.
//
// Each BacktraceEntry is associated with a linked list of Interval objects,
// if an allocation with the corresponding backtrace was done during that
// interval (and recording was on).
struct Interval {
  struct Interval* prev;//!< Pointer to the previous interval (of the same backtrace), or NULL if this is the first one.
  struct Interval* next;//!< Pointer to the next interval (of the same backtrace), or NULL if this is the last one.
  time_t start;         //!< Allocation done on or after this moment fall into this interval.
  time_t end;           //!< ... provided they were done before this moment.
  size_t total_n;       //!< Number of allocations done in the interval [start, end> seconds since application start.
  size_t n;             //!< Same, that still aren't freed.
  size_t size;          //!< The total size in bytes of all n allocations (leaked memory).
  struct Header* first;	//!< Pointer to the oldest allocation in the interval, or NULL when n == 0.
};

//! @brief Abbreviation for struct Interval.
typedef struct Interval Interval;

#endif // INTERVAL_H

