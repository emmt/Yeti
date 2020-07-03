/*
 * sort.c -
 *
 * Implement sorting functions for Yorick.
 *
 *-----------------------------------------------------------------------------
 *
 * This file is part of Yeti (https://github.com/emmt/Yeti) released under the
 * MIT "Expat" license.
 *
 * Copyright (C) 1996-2020: Éric Thiébaut.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef _YETI_SORT_C
#define _YETI_SORT_C 1

#include "config.h"
#include "yeti.h"
#if (YORICK_VERSION_MAJOR == 1) && (YORICK_VERSION_MINOR <= 4)
# include <stdio.h>
#else
# include "pstdio.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "yio.h"

#define index_t long

#define value_t char
#define HEAPSORT    heapsort_c
#define HEAPSORT1   heapsort1_c
#define QUICKSELECT quick_select_c
#include __FILE__

#define value_t short
#define HEAPSORT    heapsort_s
#define HEAPSORT1   heapsort1_s
#define QUICKSELECT quick_select_s
#include __FILE__

#define value_t int
#define HEAPSORT    heapsort_i
#define HEAPSORT1   heapsort1_i
#define QUICKSELECT quick_select_i
#include __FILE__

#define value_t long
#define HEAPSORT    heapsort_l
#define HEAPSORT1   heapsort1_l
#define QUICKSELECT quick_select_l
#include __FILE__

#define value_t float
#define HEAPSORT    heapsort_f
#define HEAPSORT1   heapsort1_f
#define QUICKSELECT quick_select_f
#include __FILE__

#define value_t double
#define HEAPSORT    heapsort_d
#define HEAPSORT1   heapsort1_d
#define QUICKSELECT quick_select_d
#include __FILE__

extern BuiltIn Y_heapsort;

void Y_heapsort(int argc)
{
  if (argc != 1) yor_error("heapsort takes exactly one argument");
  if (sp->ops == NULL) yor_unexpected_keyword_argument();
  Operand op;
  sp->ops->FormOperand(sp, &op);
  index_t number = op.type.number;
  int type = op.ops->typeID;
  if (type < YOR_CHAR || type > YOR_DOUBLE) {
    yor_error("bad data type");
  }
  void* array = op.value;
  if (CalledAsSubroutine()) {
    switch (type) {
    case YOR_CHAR:
      heapsort_c(array, number);
      return;
    case YOR_SHORT:
      heapsort_s(array, number);
      return;
    case YOR_INT:
      heapsort_i(array, number);
      return;
    case YOR_LONG:
      heapsort_l(array, number);
      return;
    case YOR_FLOAT:
      heapsort_f(array, number);
      return;
    case YOR_DOUBLE:
      heapsort_d(array, number);
      return;
    }
  } else {
    index_t* index = YOR_PUSH_NEW_ARRAY(index_t, yor_start_dimlist(number));
    switch (type) {
    case YOR_CHAR:
      heapsort1_c(index, array, number);
      return;
    case YOR_SHORT:
      heapsort1_s(index, array, number);
      return;
    case YOR_INT:
      heapsort1_i(index, array, number);
      return;
    case YOR_LONG:
      heapsort1_l(index, array, number);
      return;
    case YOR_FLOAT:
      heapsort1_f(index, array, number);
      return;
    case YOR_DOUBLE:
      heapsort1_d(index, array, number);
      return;
    }
  }
  yor_error("bad data type (BUG)");
}

extern BuiltIn Y_quick_select;

void Y_quick_select(int argc)
{
  Operand op;
  index_t number, k, first, last, offset, elsize;
  Symbol* s;
  void* ptr;
  int in_place, type;

  if (argc < 2 || argc > 4) {
    yor_error("quick_select takes 2 to 4 arguments");
  }
  s = &sp[1 - argc];
  if (argc >= 4) {
    last = yor_get_optional_integer(s + 3, 0);
  } else {
    last = 0;
  }
  if (argc >= 3) {
    first = yor_get_optional_integer(s + 2, 1);
  } else {
    first = 1;
  }
  k = yor_get_integer(s + 1);

  if (s->ops == NULL) yor_unexpected_keyword_argument();
  s->ops->FormOperand(s, &op);
  number = op.type.number;
  type = op.ops->typeID;
  elsize = op.type.base->size;
  switch (type) {
  case YOR_CHAR:
  case YOR_SHORT:
  case YOR_INT:
  case YOR_LONG:
  case YOR_FLOAT:
  case YOR_DOUBLE:
    if (k <= 0) k += number;
    if (k <= 0 || k > number) yor_error("out of range index K");
    if (first <= 0) first += number;
    if (first <= 0 || first > number) yor_error("out of range index FIRST");
    if (last <= 0) last += number;
    if (last <= 0 || last > number) yor_error("out of range index LAST");
    if (last < first || k < first || k > last) {
      yor_error("selected index range is empty");
    }
    break;
  default:
    yor_error("bad data type");
  }
  in_place = CalledAsSubroutine();
  if (in_place) {
    ptr = op.value;
  } else {
    /* creates a temporary copy as needed */
    if (op.references) {
      ptr = (void*)(((Array*)PushDataBlock(NewArray(op.type.base,
                                                    op.type.dims)))->value.c);
      memcpy(ptr, op.value, number*elsize);
      PopTo(s);
    } else {
      ptr = op.value;
    }
  }
  offset = first - 1;
  number = last - first + 1;
  k -= first; /* must be zero-based index */
  ptr = (void*)(((char*)ptr) + offset*elsize);

  switch (type) {
  case YOR_CHAR:
    quick_select_c(k, number, (char*)ptr);
    if (! in_place) {
      yor_push_char_value(((char*)ptr)[k]);
    }
    break;
  case YOR_SHORT:
    quick_select_s(k, number, (short*)ptr);
    if (! in_place) {
      yor_push_short_value(((short*)ptr)[k]);
    }
    break;
  case YOR_INT:
    quick_select_i(k, number, (int*)ptr);
    if (! in_place) {
      yor_push_int_value(((int*)ptr)[k]);
    }
    break;
  case YOR_LONG:
    quick_select_l(k, number, (long*)ptr);
    if (! in_place) {
      yor_push_long_value(((long*)ptr)[k]);
    }
    break;
  case YOR_FLOAT:
    quick_select_f(k, number, (float*)ptr);
    if (! in_place) {
      yor_push_float_value(((float*)ptr)[k]);
    }
    break;
  case YOR_DOUBLE:
    quick_select_d(k, number, (double*)ptr);
    if (! in_place) {
      yor_push_double_value(((double*)ptr)[k]);
    }
    break;
  }
}

#else /* _YETI_SORT_C */

#ifdef HEAPSORT
/* HEAPSORT - in-place sorting of array */
static void HEAPSORT(value_t a[], const index_t n)
{
  index_t i,j,k,l;
  value_t asave;

  if (n < 2) return;
  k = n/2;
  l = n - 1;
  for (;;) {
    if (k > 0) {
      asave = a[--k];
    } else {
      asave = a[l];
      a[l] = a[0];
      if (--l == 0) {
        a[0] = asave;
        return;
      }
    }
    i = k;
    while ((j = 2*i + 1) <= l) {
      if (j < l && a[j] < a[j + 1]) ++j;
      if (a[j] <= asave) break;
      a[i] = a[j];
      i = j;
    }
    a[i] = asave;
  }
}
#endif /* HEAPSORT */

#ifdef HEAPSORT0
/* HEAPSORT0 - indirect sorting of an array, with C-indexing (starting at 0) */
static void HEAPSORT0(index_t index[restrict],
                      const value_t a[restrict],
                      const index_t n)
{
  // Initialize index table, assuming 0-based indices.
  for (index_t i = 0; i < n; ++i) {
    index[i] = i;
  }
  if (n < 2) return;
  index_t k = n/2;
  index_t l = n - 1;
  for (;;) {
    index_t isave;
    if (k > 0) {
      isave = index[--k];
    } else {
      isave = index[l];
      index[l] = index[0];
      if (--l == 0) {
        index[0] = isave;
        return;
      }
    }
    value_t asave = a[isave];
    index_t i = k;
    index_t j;
    while ((j = 2*i + 1) <= l) {
      if (j < l && a[index[j]] < a[index[j + 1]]) ++j;
      if (a[index[j]] <= asave) break;
      index[i] = index[j];
      i = j;
    }
    index[i] = isave;
  }
}
#endif /* HEAPSORT0 */

#ifdef HEAPSORT1
/* HEAPSORT1 - indirect sorting of an array, with Yorick/FORTRAN indexing
   (starting at 1) */
static void HEAPSORT1(index_t index[restrict],
                      const value_t* restrict a,
                      const index_t n)
{
  // Initialize index table, assuming 1-based indices.
  for (index_t i = 0; i < n; ++i) {
    index[i] = i + 1;
  }
  if (n < 2) return;
  --a; // offset array for 1-based indexing
  index_t k = n/2;
  index_t l = n - 1;
  for (;;) {
    index_t isave;
    if (k > 0) {
      isave = index[--k];
    } else {
      isave = index[l];
      index[l] = index[0];
      if (--l == 0) {
        index[0] = isave;
        return;
      }
    }
    value_t asave = a[isave];
    index_t i = k;
    index_t j;
    while ((j = 2*i + 1) <= l) {
      if (j < l && a[index[j]] < a[index[j + 1]]) ++j;
      if (a[index[j]] <= asave) break;
      index[i] = index[j];
      i = j;
    }
    index[i] = isave;
  }
}
#endif /* HEAPSORT1 */

#ifdef QUICKSELECT
#define SWAP(a,b) t=(a);(a)=(b);(b)=t
static value_t QUICKSELECT(index_t k, index_t n, value_t arr[])
{
  index_t bot = 0;
  index_t top = n - 1;
  for (;;) {
    value_t t;
    if (top <= bot + 1) {
      if (top == bot + 1 && arr[bot] > arr[top]) {
        SWAP(arr[bot], arr[top]);
      }
      return arr[k];
    } else {
      index_t mid = (bot + top)/2;
      SWAP(arr[mid], arr[bot + 1]);
      if (arr[bot] > arr[top]) {
        SWAP(arr[bot], arr[top]);
      }
      if (arr[bot + 1] > arr[top]) {
        SWAP(arr[bot + 1], arr[top]);
      }
      if (arr[bot] > arr[bot + 1]) {
        SWAP(arr[bot], arr[bot + 1]);
      }
      index_t i = bot + 1;
      index_t j = top;
      value_t a = arr[i];
      for (;;) {
        while (arr[++i] < a)
          ;
        while (arr[--j] > a)
          ;
        if (j < i) break;
        SWAP(arr[i], arr[j]);
      }
      arr[bot + 1] = arr[j];
      arr[j] = a;
      if (j >= k) top = j - 1;
      if (j <= k) bot = i;
    }
  }
}
#undef SWAP

#endif /* QUICKSELECT */

#undef HEAPSORT
#undef HEAPSORT0
#undef HEAPSORT1
#undef QUICKSELECT
#undef value_t

#endif /* _YETI_SORT_C */
