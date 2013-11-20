// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file sort.h This file contains the declaration of the sorting functions.
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

#include "BacktraceEntry.h"

//! @brief The type of the linked list node to sort.
typedef BacktraceEntry Node;
//! @brief The value type of the linked list node to sort.
typedef double value_type;

//! @brief Sort LIST by value_n and next_n.
Node* sort_n(Node* list, Node** last_out);
