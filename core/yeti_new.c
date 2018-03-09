/*
 * yeti_new.c -
 *
 * Various built-in functions using the ney Yorick API defined in "yapi.h".
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2005-2009 Eric Thiébaut <thiebaut@obs.univ-lyon1.fr>
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

#ifndef _YETI_NEW_C
#define _YETI_NEW_C 1

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <pstdlib.h>
#include <yapi.h>

/* Built-in functions defined in this file. */
void Y_is_dimlist(int argc);
void Y_same_dimlist(int argc);
void Y_make_dimlist(int argc);
void Y_make_range(int argc);
void Y_parse_range(int argc);


/*---------------------------------------------------------------------------*/
/* RANGE */

#if 0
void Y_is_range(int argc)
{
  int result = 0;

  if (argc != 1) y_error("wrong number of arguments");
  switch (yarg_typeid(0)) {
  case Y_CHAR:
  case Y_SHORT:
  case Y_INT:
  case Y_LONG:
    if (yarg_rank(0) == 0) {
      result = 2;
    }
    break;
  case Y_RANGE:
    result = 1;
    break;
  case Y_VOID:
    result = 3;
    break;
  }
  push_int(result);
}
#endif

void Y_make_range(int argc)
{
  long dims[Y_DIMSIZE], ntot, *arr;

  if (argc != 1) y_error("wrong number of arguments");
  switch (yarg_typeid(0)) {
  case Y_CHAR:
  case Y_SHORT:
  case Y_INT:
  case Y_LONG:
    arr = ygeta_l(0, &ntot, dims);
    if ((ntot == 4) && (dims[0] == 1)) {
      ypush_range(&arr[1], arr[0]);
      return;
    }
  }
  y_error("expecting an array of 4 integers");
}

void Y_parse_range(int argc)
{
  long dims[2], *arr;

  if (argc != 1) y_error("wrong number of arguments");
  if (yarg_typeid(0) != Y_RANGE) y_error("expecting a range");
  dims[0] = 1;
  dims[1] = 4;
  arr = ypush_l(dims);
  arr[0] = yget_range(1, &arr[1]); /* iarg=1 because result already pushed
                                      on top of stack */
}

/*---------------------------------------------------------------------------*/
/* DIMENSION LIST */

#define type_t     unsigned char
#define CHECK_DIMS check_dims_c
#include __FILE__

#define type_t     short
#define CHECK_DIMS check_dims_s
#include __FILE__

#define type_t     int
#define CHECK_DIMS check_dims_i
#include __FILE__

#define type_t     long
#define CHECK_DIMS check_dims_l
#include __FILE__

#if 0
void Y_tmpfile(int argc)
{
  const char *tail = "XXXXXX";
  ystring_t src, dst, *arr;
  long len, size;
  int pad, fd;

  if (argc != 1) y_error("tmpfile takes exaclty one argument");
  src = ygets_q(0);
  len = (src && src[0] ? strlen(src) : 0);
  if (len < 6 || strcmp(src + (len - 6), tail)) {
    pad = 1;
    size = len + 7;
  } else {
    pad = 0;
    size = len + 1;
  }
  arr = ypush_q(0);
  dst = p_malloc(size);
  dst[size - 1] = '\0'; /* mark end of string */
  arr[0] = dst; /* then store string pointer */
  if (len > 0) memcpy(dst, src, len);
  if (pad) memcpy(dst + len, tail, 6);
  fprintf(stderr, "template=\"%s\"\n", dst);
  fd = mkstemp(dst);
  if (fd < 0) {
    y_error("tmpfile failed to create a unique temporary file");
  } else {
    close(fd);
  }
}
#endif

#if 0
void Y_is_dimlist(int argc)
{
}

void Y_same_dimlist(int argc)
{
}
#endif

/* n  or [l, n1, n2, .., nl] */
void Y_make_dimlist(int argc)
{
  long *dimlist, dims[Y_DIMSIZE], ref, ndims, j, n;
  int iarg;            /* argument index */
  int nvoids;          /* number of void arguments */
  int iarg_of_result;  /* index of potentially valid result */

  if (argc < 1) y_error("make_dimlist takes at least one argument");
  if (yarg_subroutine()) {
    ref = yget_ref(argc - 1);
    if (ref < 0L) y_error("expecting a simple reference for first argument");
  } else {
    ref = -1L;
  }

  /* First pass: count total number of dimensions. */
  nvoids = 0;
  iarg_of_result = -1;
  ndims = 0L;
  for (iarg = argc - 1; iarg >= 0; --iarg) {
    switch (yarg_typeid(iarg)) {
    case Y_CHAR:
      ndims += check_dims_c(ygeta_c(iarg, NULL, dims), dims);
      break;
    case Y_SHORT:
      ndims += check_dims_s(ygeta_s(iarg, NULL, dims), dims);
      break;
    case Y_INT:
      ndims += check_dims_i(ygeta_i(iarg, NULL, dims), dims);
      break;
    case Y_LONG:
      ndims += check_dims_l(ygeta_l(iarg, NULL, dims), dims);
      if (iarg_of_result < 0 && dims[0] == 1L) {
        /* First argument which is elligible as the resulting
           dimension list. */
        iarg_of_result = iarg;
      }
      break;
    case Y_VOID:
      ++nvoids;
      break;
    default:
      y_error("unexpected data type in dimension list");
    }
  }
  if (argc - nvoids == 1 && iarg_of_result >= 0) {
    /* Exactly one non void argument and which is elligible
       as the resulting dimension list. */
    if (ref < 0L) {
      /* Called as a function; nothing to do except dropping the
         void arguments above the result if any. */
      if (iarg_of_result > 0) {
        yarg_drop(iarg_of_result);
      }
      return;
    }

    /* Called as a subroutine: elligible result must be the first
       argument. */
    if (iarg_of_result == argc -1) {
      return;
    }
  }

  /* Second pass: build up new dimension list. */
  dims[0] = 1;
  dims[1] = ndims + 1;
  dimlist = ypush_l(dims);
  *dimlist = ndims;
  for (iarg = argc; iarg >= 1; --iarg) {
#define GET_DIMS(type_t, x)					\
      {								\
        type_t *ptr = (type_t *)ygeta_##x(iarg, &n, dims);	\
        if (dims[0]) {						\
          for (j=1L ; j<n ; ++j) {				\
            *++dimlist = ptr[j];				\
          }							\
        } else {						\
          *++dimlist = ptr[0];					\
        }							\
      }								\
      break
    switch (yarg_typeid(iarg)) {
    case Y_CHAR:  GET_DIMS(unsigned char, c);
    case Y_SHORT: GET_DIMS(short,         s);
    case Y_INT:   GET_DIMS(int,           i);
    case Y_LONG:  GET_DIMS(long,          l);
    }
#undef GET_DIMS
  }

  if (ref >= 0L) {
    /* replace reference by topmost stack element */
    yput_global(ref, 0);
  }
}

#else /* _YETI_NEW_C *********************************************************/

#ifdef CHECK_DIMS
static long CHECK_DIMS(const void *array, const long dims[])
{
  const type_t *value = array;
  long j, n;

  n = value[0];
  if (! dims[0] && n > 0L) return 1L;
  if (dims[0] == 1L && dims[1] == n + 1L) {
    for (j=1L ; j<=n ; ++j) {
      if (value[j] <= 0) {
        goto bad_dimlist;
      }
    }
    return n;
  }
 bad_dimlist:
  y_error("bad dimension list @");
  return -1L; /* avoid compiler warnings */
}
#endif /* CHECK_DIMS */

#undef type_t
#undef CHECK_DIMS

#endif /* _YETI_NEW_C ********************************************************/
