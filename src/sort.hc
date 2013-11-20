// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file sort.hc Sort linked list.
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

#ifndef SORT_HC
#define SORT_HC

#ifndef USE_PCH
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>	// memset
#include <endian.h>
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#define CATENATE2(x,y) x##y
#define CATENATE(x,y) CATENATE2(x,y)

struct SubList {
  Node* first;		// The first element.
  Node* last;		// The last element.
  uint32_t size;	// The total number of elements.
};

typedef struct SubList SubList;

// Convert the size of a linked list to it's 'size class':
// this function returns log2(3 * size) - 1, which is approximately round(log2(size)).
// See http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogIEEE64Float for the used magic.
static unsigned int size2index(uint32_t size)
{
  union { uint32_t u[2]; double d; } t;
  t.u[__FLOAT_WORD_ORDER==__LITTLE_ENDIAN] = 0x43300000;
  t.u[__FLOAT_WORD_ORDER!=__LITTLE_ENDIAN] = 3 * size;
  t.d -= 4503599627370496.0;
  return (t.u[__FLOAT_WORD_ORDER==__LITTLE_ENDIAN] >> 20) - 0x400;
}

#endif // SORT_HC

#define VALUE CATENATE(value_,SORTTAG)
#define NEXT CATENATE(next_,SORTTAG)

// Inner loop.
//
// FIRST0 and FIRST1 point to the next unprocessed elements of the two lists.
// FIRST2 is assigned to constantly be the node following FIRST0.
//
// FIRST1 (and VALUE_FIRST1) are kept constant.
// FIRST0 and FIRST2 are advanced until the value of FIRST2 is smaller
// than the value of FIRST1, then a link is made from FIRST0 to FIRST1.
//
// The loop aborts without searching if it detects that searching isn't
// necessary because the list of FIRST0 doesn't contain an element that
// is smaller than FIRST1. Note how this completely avoids the need for
// detecting if we reached the end of the list.
//
#define INNER_LOOP(first2, first0, first1, last0, update_list_last)			\
    if (UNLIKELY(value_##first1 <= value_##last0))					\
    {											\
      last0->NEXT = first1;								\
      update_list_last;									\
      return;										\
    }											\
    while (value_##first1 <= (value_##first2 = (first2 = first0->NEXT)->VALUE))		\
      first0 = first2;									\
    first0->NEXT = first1

// Merge list1 into list0.
//
// The lists must be stored in descending order: if list --> first --> last, then value(last) <= value(first).
// *FIRST0_PTR is updated to be the first (largest) element after merging and
// *LAST0_PTR is updated to be the last (smallest) element after merging.
// The merged result will therefore be [*first0_ptr -> ... -> *last0_ptr].
static void CATENATE(merge_,SORTTAG)(Node** first0_ptr, Node** last0_ptr, Node* first1, Node* last1)
{
  Node* first0 = *first0_ptr;
  Node* last0 = *last0_ptr;
  value_type value_first0 = first0->VALUE;
  value_type value_first1 = first1->VALUE;
  value_type value_last0 = last0->VALUE;
  value_type value_last1 = last1->VALUE;
  Node* first2;
  value_type value_first2;
  if (value_first0 < value_first1)
  {
    *first0_ptr = first1;
    for (;;)
    {
      // Unrolled six times in order to avoid swapping last0 <--> last1, value_last0 <--> value_last1,
      // and rotating first0 --> first1 --> first2 --> and value_first0 --> value_first1 --> value_first2 -->.
      // Thus avoiding 14 instructions per LOOP.
      INNER_LOOP(first2, first1, first0, last1, /* *last0_ptr = last0 */);
      INNER_LOOP(first1, first0, first2, last0,    *last0_ptr = last1   );
      INNER_LOOP(first0, first2, first1, last1, /* *last0_ptr = last0 */);
      INNER_LOOP(first2, first1, first0, last0,    *last0_ptr = last1   );
      INNER_LOOP(first1, first0, first2, last1, /* *last0_ptr = last0 */);
      INNER_LOOP(first0, first2, first1, last0,    *last0_ptr = last1   );
    }
  }
  else
  {
    for (;;)
    {
      // Same as above, but with swapped lists 0 <--> 1.
      INNER_LOOP(first2, first0, first1, last0,    *last0_ptr = last1   );
      INNER_LOOP(first0, first1, first2, last1, /* *last0_ptr = last0 */);
      INNER_LOOP(first1, first2, first0, last0,    *last0_ptr = last1   );
      INNER_LOOP(first2, first0, first1, last1, /* *last0_ptr = last0 */);
      INNER_LOOP(first0, first1, first2, last0,    *last0_ptr = last1   );
      INNER_LOOP(first1, first2, first0, last1, /* *last0_ptr = last0 */);
    }
  }
}

// sort
//
// Sort the single linked LIST.
//
Node* CATENATE(sort_,SORTTAG)(Node* list, Node** last_out)
{
  if (!list)					// Empty list.
    return NULL;

  // Find the last sublist.
  Node* first = list;
  Node* last = list;
  uint32_t size = 1;
  value_type value0 = last->VALUE;
  Node* node1 = last->NEXT;
  value_type value1;
  while(node1 && (value1 = node1->VALUE) <= value0)
  {
    ++size;
    last = node1;
    value0 = value1;
    node1 = last->NEXT;
  }
  if (UNLIKELY(!node1))				// Already sorted.
    return list;

  // Reserve space for 32 sublists.
  SubList sublists[32];
  // They are unused.
  memset(sublists, 0, sizeof(sublists));

  unsigned int sublisti = size2index(size);
  while(1)
  {
    // Insert the sublist {first, last, size}
    if (sublists[sublisti].size == 0)
    {
      sublists[sublisti].first = first;
      sublists[sublisti].last = last;
      sublists[sublisti].size = size;
    }
    else
    {
      size += sublists[sublisti].size;
      unsigned int newsublisti = size2index(size);
      if (newsublisti == sublisti)
      {
        sublists[sublisti].size = size;
        CATENATE(merge_,SORTTAG)(&sublists[sublisti].first, &sublists[sublisti].last, first, last);
      }
      else
      {
	sublists[sublisti].size = 0;
	CATENATE(merge_,SORTTAG)(&first, &last, sublists[sublisti].first, sublists[sublisti].last);
	sublisti = newsublisti;
	continue;
      }
    }

    if (UNLIKELY(!node1))
      break;

    // Find the next sublist.
    last = first = node1;
    size = 1;
    value0 = value1;
    node1 = last->NEXT;
    while(node1 && (value1 = node1->VALUE) <= value0)
    {
      ++size;
      last = node1;
      value0 = value1;
      node1 = last->NEXT;
    }
    sublisti = size2index(size);
  }

  // Merge remaining sublists.
  int i = 0;
  while(sublists[i].size == 0)
    ++i;
  last = sublists[i].last;
  first = sublists[i].first;
  for (int j = i + 1; j < 32; ++j)
  {
    if (sublists[j].size == 0)
      continue;
    CATENATE(merge_,SORTTAG)(&first, &last, sublists[j].first, sublists[j].last);
  }
  last->NEXT = NULL;
  if (last_out)
    *last_out = last;
  return first;
}

#if 0
//---------------------------------------------------------------------------------------
// Test code

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int print_list(Node* last, Node* first)
{
  int count = 1;
  printf("%d", first->VALUE);
  while (last != first)
  {
    ++count;
    first = first->NEXT;
    printf(" %d", first->VALUE);
  }
  return count;
}

// A list.
//
// LIST points to the first element of the list,
// LAST points to the last element (which thus
// points to NULL).
//
// NULL <-- last <-- ... <-- first <-- list
struct List {
  Node* list;
  Node* last;
};

typedef struct List List;

int print(List* list)
{
  Node* node = list->list;
  if (!node)
  {
    printf("<empty>\n");
    return 0;
  }
  int size = 0;
  while(1)
  {
    ++size;
    if (node == list->last)
      break;
    node = node->NEXT;
  }
  value_type* data = malloc(size * sizeof(value_type));
  int i = 0;
  node = list->list;
  while(1)
  {
    data[i++] = node->VALUE;
    if (node == list->last)
      break;
    node = node->NEXT;
  }
  for (i = 0; i < size; ++i)
    printf("%d ", data[size - 1 - i]);
  printf("\n");
  free(data);
  return size;
}

int check(List* list)
{
  int count = 0;
  for (Node* node = list->list; node; node = node->NEXT)
  {
    ++count;
    assert(!node->NEXT || node->VALUE >= node->NEXT->VALUE);
    assert(node->NEXT || node == list->last);
  }
  return count;
}

#define LISTSIZE 5000000
int main()
{
  struct random_data rdata;
  char rstatebuf[256];
  Node* nodes1 = malloc(sizeof(Node) * LISTSIZE);

#if 0
  int v[15] = { 9, 4, 11, 3, 5, 7, 8, 10, 12, 13, 15, 1, 2, 6, 14 };
  for (int i = 0; i < 15; ++i)
  {
    nodes1[i].NEXT = (i == 0) ? NULL : &nodes1[i - 1];
    nodes1[i].VALUE =v[i];
  }
  List list1;
  list1.list = &nodes1[14];
  list1.last = &nodes1[0];
  int size1 = print(&list1);
  list1.list = CATENATE(sort_,SORTTAG)(list1.list, &list1.last);
  assert(check(&list1) == 15);
  print(&list1);
  return 0;
#endif

  initstate(0x3456aabc, rstatebuf, sizeof(rstatebuf));

  int loglistsize = 0;
  for (int listsize = 2; listsize <= LISTSIZE; listsize *= 2)
  {
    ++loglistsize;
    uint64_t total = 0;
    uint64_t loops = 20;
    for (int test = 0; test < loops; ++test)
    {
      value_type val = 0;
      for (int i = 0; i < listsize; ++i)
      {
	nodes1[i].NEXT = (i == 0) ? NULL : &nodes1[i - 1];
	nodes1[i].VALUE = random();
      }
      List list1;
      list1.list = &nodes1[listsize - 1];
      list1.last = &nodes1[0];

      //int size1 = print(&list1);

      struct timeval before, after;
      gettimeofday(&before, NULL);
      list1.list = CATENATE(sort_,SORTTAG)(list1.list, &list1.last);
      gettimeofday(&after, NULL);
      timersub(&after, &before, &after);
      uint64_t t = after.tv_sec;
      t *= 1000000;
      t += after.tv_usec;
      total += t;

      assert(check(&list1) == listsize);
      //print(&list1);
    }
    printf("Average sorting time for size %7d: %8.4f ns * N, %8.4f ns * N log2(N), %8.4f ns * N log2(N)^2, %8.4f ns * N log2(N)^3.\n", listsize,
	(double)total * 1000 / (loops * listsize),
	(double)total * 1000 / (loops * loglistsize * listsize),
	(double)total * 1000 / (loops * loglistsize * loglistsize * listsize),
	(double)total * 1000 / (loops * loglistsize * loglistsize * loglistsize * listsize));
  }
}
#endif

#undef VALUE
#undef NEXT
