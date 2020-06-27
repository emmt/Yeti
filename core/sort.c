/*
 * sort.c -
 *
 * Implement sorting functions in Yeti.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 1996-2010 Eric Thiébaut <thiebaut@obs.univ-lyon1.fr>
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can use, modify
 * and/or redistribute the software under the terms of the CeCILL-C license as
 * circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty and the software's author, the holder of the
 * economic rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more
 * generally, to use and operate it in the same conditions as regards
 * security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
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

#define value_t unsigned char
#define HEAPSORT    _yeti_heapsort_c
#define HEAPSORT1   _yeti_heapsort1_c
#define QUICKSELECT _yeti_quick_select_c
#include __FILE__

#define value_t short
#define HEAPSORT    _yeti_heapsort_s
#define HEAPSORT1   _yeti_heapsort1_s
#define QUICKSELECT _yeti_quick_select_s
#include __FILE__

#define value_t int
#define HEAPSORT    _yeti_heapsort_i
#define HEAPSORT1   _yeti_heapsort1_i
#define QUICKSELECT _yeti_quick_select_i
#include __FILE__

#define value_t long
#define HEAPSORT    _yeti_heapsort_l
#define HEAPSORT1   _yeti_heapsort1_l
#define QUICKSELECT _yeti_quick_select_l
#include __FILE__

#define value_t float
#define HEAPSORT    _yeti_heapsort_f
#define HEAPSORT1   _yeti_heapsort1_f
#define QUICKSELECT _yeti_quick_select_f
#include __FILE__

#define value_t double
#define HEAPSORT    _yeti_heapsort_d
#define HEAPSORT1   _yeti_heapsort1_d
#define QUICKSELECT _yeti_quick_select_d
#include __FILE__

extern BuiltIn Y_heapsort;

void Y_heapsort(int argc)
{
  Operand op;
  index_t *index = NULL, number;

  if (argc != 1) YError("heapsort takes exactly one argument");
  if (! sp->ops) YError("unexpected keyword");
  sp->ops->FormOperand(sp, &op);
  number = op.type.number;
  if (CalledAsSubroutine()) {
    switch (op.ops->typeID) {
    case T_CHAR:
      _yeti_heapsort_c(op.value, number);
      return;
    case T_SHORT:
      _yeti_heapsort_s(op.value, number);
      return;
    case T_INT:
      _yeti_heapsort_i(op.value, number);
      return;
    case T_LONG:
      _yeti_heapsort_l(op.value, number);
      return;
    case T_FLOAT:
      _yeti_heapsort_f(op.value, number);
      return;
    case T_DOUBLE:
      _yeti_heapsort_d(op.value, number);
      return;
    }
  } else {
    switch (op.ops->typeID) {
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_FLOAT:
    case T_DOUBLE:
      index = YETI_PUSH_NEW_L(yeti_start_dimlist(number));
      switch (op.ops->typeID) {
      case T_CHAR:
        _yeti_heapsort1_c(index, op.value, number);
        break;
      case T_SHORT:
        _yeti_heapsort1_s(index, op.value, number);
        break;
      case T_INT:
        _yeti_heapsort1_i(index, op.value, number);
        break;
      case T_LONG:
        _yeti_heapsort1_l(index, op.value, number);
        break;
      case T_FLOAT:
        _yeti_heapsort1_f(index, op.value, number);
        break;
      default:
        _yeti_heapsort1_d(index, op.value, number);
        break;
      }
      return;
    }
  }
  YError("bad data type");
}

extern BuiltIn Y_quick_select;

void Y_quick_select(int argc)
{
  Operand op;
  index_t number, k, first, last, offset, elsize;
  Symbol *s;
  void *ptr;
  int in_place, type;

  if (argc < 2 || argc > 4) {
    YError("quick_select takes 2 to 4 arguments");
  }
  s = &sp[1 - argc];
  if (argc >= 4) {
    last = yeti_get_optional_integer(s + 3, 0);
  } else {
    last = 0;
  }
  if (argc >= 3) {
    first = yeti_get_optional_integer(s + 2, 1);
  } else {
    first = 1;
  }
  k = YGetInteger(s + 1);

  if (! s->ops) YError("unexpected keyword");
  s->ops->FormOperand(s, &op);
  number = op.type.number;
  type = op.ops->typeID;
  elsize = op.type.base->size;
  switch (type) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
  case T_LONG:
  case T_FLOAT:
  case T_DOUBLE:
    if (k <= 0) k += number;
    if (k <= 0 || k > number) YError("out of range index K");
    if (first <= 0) first += number;
    if (first <= 0 || first > number) YError("out of range index FIRST");
    if (last <= 0) last += number;
    if (last <= 0 || last > number) YError("out of range index LAST");
    if (last < first || k < first || k > last) {
      YError("selected index range is empty");
    }
    break;
  default:
    YError("bad data type");
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
  ptr = (void *)(((char *)ptr) + offset*elsize);


  switch (type) {
  case T_CHAR:
    _yeti_quick_select_c(k, number, (unsigned char *)ptr);
    if (! in_place) {
      yeti_push_char_value(((unsigned char *)ptr)[k]);
    }
    break;
  case T_SHORT:
    _yeti_quick_select_s(k, number, (short *)ptr);
    if (! in_place) {
      yeti_push_short_value(((short *)ptr)[k]);
    }
    break;
  case T_INT:
    _yeti_quick_select_i(k, number, (int *)ptr);
    if (! in_place) {
      yeti_push_int_value(((int *)ptr)[k]);
    }
    break;
  case T_LONG:
    _yeti_quick_select_l(k, number, (long *)ptr);
    if (! in_place) {
      yeti_push_long_value(((long *)ptr)[k]);
    }
    break;
  case T_FLOAT:
    _yeti_quick_select_f(k, number, (float *)ptr);
    if (! in_place) {
      yeti_push_float_value(((float *)ptr)[k]);
    }
    break;
  case T_DOUBLE:
    _yeti_quick_select_d(k, number, (double *)ptr);
    if (! in_place) {
      yeti_push_double_value(((double *)ptr)[k]);
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
static void HEAPSORT0(index_t index[], const value_t a[], const index_t n)
{
  index_t i,j,k,l,isave;
  value_t asave;

  for (i=0 ; i<n ; ++i) index[i] = i;
  if (n < 2) return;
  k = n/2;
  l = n - 1;
  for (;;) {
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
    asave = a[isave];
    i = k;
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
static void HEAPSORT1(index_t index[], const value_t *a, index_t n)
{
  index_t i,j,k,l,isave;
  value_t asave;

  for (i=0 ; i<n ; ++i) index[i] = i + 1;
  if (n < 2) return;
  --a;
  k = n/2;
  l = n - 1;
  for (;;) {
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
    asave = a[isave];
    i = k;
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
static value_t QUICKSELECT(long k, long n, value_t arr[])
{
  index_t i, j, top, bot, mid;
  value_t a, t;

  bot = 0;
  top = n - 1;
  for (;;) {
    if (top <= bot + 1) {
      if (top == bot + 1 && arr[bot] > arr[top]) {
        SWAP(arr[bot], arr[top]);
      }
      return arr[k];
    } else {
      mid = (bot + top)/2;
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
      i = bot + 1;
      j = top;
      a = arr[i];
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
