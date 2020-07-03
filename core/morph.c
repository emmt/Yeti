/*
 * morph.c -
 *
 * Implement morpho-math operators.
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

#ifndef _YETI_MORPH_C
#define _YETI_MORPH_C 1

#include "config.h"
#include "yeti.h"
#include <stdio.h>
#include <stdlib.h>

#define voxel_t            unsigned char
#define MORPH_DILATION     dilation_c
#define MORPH_EROSION      erosion_c
#include __FILE__

#define voxel_t            short
#define MORPH_DILATION     dilation_s
#define MORPH_EROSION      erosion_s
#include __FILE__

#define voxel_t            int
#define MORPH_DILATION     dilation_i
#define MORPH_EROSION      erosion_i
#include __FILE__

#define voxel_t            long
#define MORPH_DILATION     dilation_l
#define MORPH_EROSION      erosion_l
#include __FILE__

#define voxel_t            float
#define MORPH_DILATION     dilation_f
#define MORPH_EROSION      erosion_f
#include __FILE__

#define voxel_t            double
#define MORPH_DILATION     dilation_d
#define MORPH_EROSION      erosion_d
#include __FILE__

static void morph_op(int argc, int mop);
static long* get_offset(Symbol* s, Dimension** dims);

extern BuiltIn Y_morph_erosion, Y_morph_dilation;

void Y_morph_erosion(int argc)
{
  morph_op(argc, 0);
}

void Y_morph_dilation(int argc)
{
  morph_op(argc, 1);
}

static void morph_op(int argc, int mop)
{
  if (argc != 2) {
    yor_error((mop ?
               "morph_dilation() takes exactly 2 arguments" :
               "morph_erosion() takes exactly 2 arguments"));
  }

  /* Get input array. */
  Symbol* s = sp - 1;
  if (s->ops == NULL) yor_unexpected_keyword_argument();
  Operand op;
  Dimension* dims = s->ops->FormOperand(s, &op)->type.dims;
  long ndims = 0;
  long width = 0;
  long height = 0;
  long depth = 0;
  while (dims != NULL) {
    if (++ndims > 3) yor_error("too many dimensions for input array");
    depth = height;
    height = width;
    width = dims->number;
    dims = dims->next;
  }

  /* Get radius or offset array. */
  long* off = get_offset(sp, &dims);
  long* dx;
  long* dy;
  long* dz;
  long number;
  if (dims == NULL) {
    /* Only one extra scalar argument: the structuring element is a
       sphere. */
    long r = off[0];
    if (r < 0) {
      yor_error("radius of structuring element must be non-negative");
    }
    Drop(1); /* to be able to push temporary workspace */
    long n = 2*r + 1;
    long lim0 = r*(r + 1);
    number = 0;
    if (depth > 1) {
      long mx = n*n*n; /* maximum number of offsets per dimension */
      dx = yor_push_workspace(3*sizeof(long)*mx);
      dy = dx + mx;
      dz = dy + mx;
      for (long z = -r; z <= r; ++z) {
        long lim1 = lim0 - z*z;
        for (long y = -r; y <= r; ++y) {
          long lim2 = lim1 - y*y;
          for (long x = -r; x <= r; ++x) {
            /* To be inside the structuring element, we must have
             *   sqrt(x*x + y*y + z*z) < r + 1/2
             * which is the same as:
             *   x*x + y*y + z*z <= r*(r + 1)
             * because X, Y, Z and R are integers.
             */
            if (x*x <= lim2) {
              dx[number] = x;
              dy[number] = y;
              dz[number] = z;
              ++number;
            }
          }
        }
      }
    } else if (height > 1) {
      long mx = n*n; /* maximum number of offsets per dimension */
      dx = yor_push_workspace(2*sizeof(long)*mx);
      dy = dx + mx;
      dz = NULL;
      for (long y = -r; y <= r; ++y) {
        long lim1 = lim0 - y*y;
        for (long x = -r; x <= r; ++x) {
          if (x*x <= lim1) {
            dx[number] = x;
            dy[number] = y;
            ++number;
          }
        }
      }
    } else {
      long mx = n;
      dx = yor_push_workspace(sizeof(long)*mx);
      dy = NULL;
      dz = NULL;
      for (long x = -r; x <= r; ++x) {
        dx[number] = x;
        ++number;
      }
    }
  } else {
    if (ndims > 1) {
      if (dims->number != ndims) {
        yor_error("last dimension of OFF not equal to number of dimensions of A");
      }
      dims = dims->next;
    }
    number = 1;
    while (dims != NULL) {
      number *= dims->number;
      dims = dims->next;
    }
    dx = off;
    dy = (ndims >= 2 ? dx + number : NULL);
    dz = (ndims >= 3 ? dy + number : NULL);
  }

  /* Allocate output array and apply the operation. */
  Array* ap = ((Array*)PushDataBlock(NewArray(op.type.base, op.type.dims)));
  switch (op.ops->typeID) {
#undef _
#define _(T) (mop ? dilation_##T : erosion_##T)((void*)ap->value.T, \
              op.value, width, height, depth, dx, dy, dz, number); break
  case YOR_CHAR:   _(c);
  case YOR_SHORT:  _(s);
  case YOR_INT:    _(i);
  case YOR_LONG:   _(l);
  case YOR_FLOAT:  _(f);
  case YOR_DOUBLE: _(d);
#undef _
  default:
    yor_error("bad data type");
  }
}

/* almost the same as YGet_L */
static long* get_offset(Symbol* s, Dimension** dims)
{
  if (s->ops == NULL) yor_unexpected_keyword_argument();
  if (s->ops == &referenceSym && globTab[s->index].ops == &longScalar) {
    if (dims != NULL) *dims = 0;
    return &globTab[s->index].value.l;
  }
  Operand op;
  switch (s->ops->FormOperand(s, &op)->ops->typeID) {
  case YOR_CHAR:
  case YOR_SHORT:
  case YOR_INT:
    op.ops->ToLong(&op);
  case YOR_LONG:
    if (dims) *dims = op.type.dims;
    return op.value;
  }
  yor_error("bad data type for index offsets");
  return 0L;
}

#else /* _YETI_MORPH_C ------------------------------------------------------*/

#ifdef MORPH_SEGMENTATION

long MORPH_SEGMENTATION(long region[],
                        search_t search[],
                        long index[], long nb[],
                        unsigned short x0[], unsigned short x1[],
                        unsigned short y0[], unsigned short y1[],
                        const voxel_t img[], long width, long height)
{
#define LEFT   ((search_t)1)
#define RIGHT  ((search_t)2)
#define DOWN   ((search_t)4)
#define UP     ((search_t)8)
#define ALL    ((search_t)(LEFT|RIGHT|DOWN|UP))

  long mark = 1;
  long number = width*height;
  for (long i = 0; i < number; ++i) {
    if (region[i] != 0) continue; /* pixel already marked */
    long count = 0;         /* start a new region with zero pixels */
    voxel_t level = img[i]; /* pixel value within current region */
    region[i] = mark;       /* mark current pixel */
    search[i] = ALL;        /* will check all neighbors of current pixel */
    index[count++] = i;     /* current pixel belongs to current region */
    for (long j = 0; j < count; ++j) {
      long k = index[j];
      long x = k%width;
      long y = k/width;
      search_t s = search[k];

#define CHECK_PIXEL(DIRECTION, BOUND_CHECK, POSITION)	\
      do {                                              \
        if ((s & DIRECTION) && (BOUND_CHECK)) {         \
          long l = (POSITION);                          \
          if (img[l] == level) {                        \
            if (region[l] == mark) {			\
              search[l] &= (ALL & (~(DIRECTION)));	\
            } else {					\
              region[l] = mark;				\
              search[l] = (ALL & (~(DIRECTION)));       \
              index[count++] = l;                       \
            }						\
          }						\
        }                                               \
      } while (0)
      CHECK_PIXEL(LEFT,  x > 0,            k - 1);
      CHECK_PIXEL(RIGHT, x < (width - 1),  k + 1);
      CHECK_PIXEL(DOWN,  y > 0,            k - width);
      CHECK_PIXEL(UP,    y < (height - 1), k + width);
#undef CHECK_PIXEL
    }

    if (nb != NULL) {
      nb[mark - 1] = count;
    }
    if (x0 != NULL || x1 != NULL) {
      long xmin, xmax;
      xmin = xmax = index[0]%width;
      for (long j = 1; j < count; ++j) {
        x = index[j]%width;
        if (x < xmin) xmin = x;
        if (x > xmax) xmax = x;
      }
      if (x0 != NULL) x0[mark - 1] = xmin;
      if (x1 != NULL) x1[mark - 1] = xmax;
    }
    if (y0 != NULL || y1 != NULL) {
      long ymin, ymax;
      ymin = ymax = index[0]/width;
      for (long j = 1; j < count; ++j) {
        long y = index[j]/width;
        if (y < ymin) ymin = y;
        if (y > ymax) ymax = y;
      }
      if (y0 != NULL) y0[mark - 1] = ymin;
      if (y1 != NULL) y1[mark - 1] = ymax;
    }

    ++mark;
  }
#undef LEFT
#undef RIGHT
#undef DOWN
#undef UP
#undef ALL

  return mark;
}
#endif /* MORPH_SEGMENTATION */

#undef _
#define _(CMP) (voxel_t dst[], const voxel_t src[],			\
                long width, long height, long depth,			\
                const long dx[], const long dy[], const long dz[],	\
                long number)						\
{									\
  long i, x, y, z, xp, yp, zp;						\
  voxel_t val, tmp;							\
                                                                        \
  val = 0; /* avoids compiler warnings */				\
  if (depth > 1) {							\
    /* 3-D case. */							\
    for (z=0 ; z<depth ; ++z) {						\
      for (y=0 ; y<height ; ++y) {					\
        for (x=0 ; x<width ; ++x) {					\
          int any = 0;							\
          for (i = 0 ; i < number ; ++i) {				\
            xp = x + dx[i];						\
            yp = y + dy[i];						\
            zp = z + dz[i];						\
            if (xp >= 0 && xp < width &&				\
                yp >= 0 && yp < height &&				\
                zp >= 0 && zp < depth) {				\
              if (any) {						\
                if ((tmp = src[(zp*height + yp)*width + xp]) CMP val) {	\
                  val = tmp;						\
                }							\
              } else {							\
                val = src[(zp*height + yp)*width + xp];			\
                any = 1;						\
              }								\
            }								\
          }								\
          if (any) {							\
            dst[(z*height + y)*width + x] = val;			\
          }								\
        }								\
      }									\
    }									\
  } else if (height > 1) {						\
    /* 2-D case. */							\
    for (y=0 ; y<height ; ++y) {					\
      for (x=0 ; x<width ; ++x) {					\
        int any = 0;							\
        for (i = 0 ; i < number ; ++i) {				\
          xp = x + dx[i];						\
          yp = y + dy[i];						\
          if (xp >= 0 && xp < width &&					\
              yp >= 0 && yp < height) {					\
            if (any) {							\
              if ((tmp = src[yp*width + xp]) CMP val) {			\
                val = tmp;						\
              }								\
            } else {							\
              val = src[yp*width + xp];					\
              any = 1;							\
            }								\
          }								\
        }								\
        if (any) {							\
          dst[y*width + x] = val;					\
        }								\
      }									\
    }									\
  } else {								\
    /* 1-D case. */							\
    for (x=0 ; x<width ; ++x) {						\
      int any = 0;							\
      for (i=0 ; i<number ; ++i) {					\
        xp = x + dx[i];							\
        if (xp >= 0 && xp < width) {					\
          if (any) {							\
            if ((tmp = src[xp]) CMP val) {				\
              val = tmp;						\
            }								\
          } else {							\
            val = src[xp];						\
            any = 1;							\
          }								\
        }								\
      }									\
      if (any) {							\
        dst[x] = val;							\
      }									\
    }									\
  }									\
}

#ifdef MORPH_DILATION
static void MORPH_DILATION _(>)
#endif
#ifdef MORPH_EROSION
static void MORPH_EROSION _(<)
#endif /* MORPH_EROSION */
#undef _

#undef MORPH_SEGMENTATION
#undef MORPH_DILATION
#undef MORPH_EROSION
#undef voxel_t

#endif /* _YETI_MORPH_C -----------------------------------------------------*/
