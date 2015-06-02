/*
 * yeti_morph.c -
 *
 * Implement morpho-math operators in Yeti.
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
static long *get_offset(Symbol *s, Dimension **dims);

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
  char msg[80];
  Operand op;
  Dimension *dims;
  Symbol *s;
  Array *ap;
  long ndims, width, height, depth, number, *off, *dx, *dy, *dz;

  if (argc != 2) {
    sprintf(msg, "morph_%s takes exactly 2 arguments",
            (mop ? "dilation" : "erosion"));
    YError(msg);
  }

  /* Get input array. */
  s = sp - 1;
  if (! s->ops) YError("unexpected keyword argument");
  dims = s->ops->FormOperand(s, &op)->type.dims;
  ndims = 0;
  width = height = depth = 0;
  while (dims) {
    if (++ndims > 3) YError("too many dimensions for input array");
    depth = height;
    height = width;
    width = dims->number;
    dims = dims->next;
  }

  /* Get radius / offset array. */
  off = get_offset(sp, &dims);
  if (! dims) {
    /* Only one extra scalar argument: the structuring element is a
       sphere. */
    long x, y, z, r, n, lim0, lim1, lim2;
    r = off[0];
    if (r < 0) {
      YError("radius of structuring element must be a positive integer");
    }
    Drop(1); /* to be able to push temporary workspace */
    n = 2*r + 1;
    lim0 = r*(r + 1);
    number = 0;
    if (depth > 1) {
      n = n*n*n; /* maximum number of offsets per dimension */
      off = yeti_push_workspace(3*sizeof(long)*n);
      dx = off;
      dy = dx + n;
      dz = dy + n;
      for (z=-r ; z<=r ; ++z) {
        lim1 = lim0 - z*z;
        for (y=-r ; y<=r ; ++y) {
          lim2 = lim1 - y*y;
          for (x=-r ; x<=r ; ++x) {
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
      n = n*n; /* maximum number of offsets per dimension */
      dx = yeti_push_workspace(2*sizeof(long)*n);
      dy = dx + n;
      dz = NULL;
      for (y=-r ; y<=r ; ++y) {
        lim1 = lim0 - y*y;
        for (x=-r ; x<=r ; ++x) {
          if (x*x <= lim1) {
            dx[number] = x;
            dy[number] = y;
            ++number;
          }
        }
      }
    } else {
      dx = yeti_push_workspace(sizeof(long)*n);
      dy = NULL;
      dz = NULL;
      for (x=-r ; x<=r ; ++x) {
        dx[number++] = x;
      }
    }
  } else {
    if (ndims > 1) {
      if (dims->number != ndims) {
        YError("last dimension of OFF not equal to number of dimensions of A");
      }
      dims = dims->next;
    }
    number = 1;
    while (dims) {
      number *= dims->number;
      dims = dims->next;
    }
    dx = off;
    dy = (ndims >= 2 ? dx + number : NULL);
    dz = (ndims >= 3 ? dy + number : NULL);
  }

  /* Allocate output array and apply the operation. */
  ap = ((Array *)PushDataBlock(NewArray(op.type.base, op.type.dims)));
  switch (op.ops->typeID) {
#undef _
#define _(ID) (mop ? dilation_##ID : erosion_##ID)((void *)ap->value.ID, \
              op.value, width, height, depth, dx, dy, dz, number); break
  case T_CHAR:   _(c);
  case T_SHORT:  _(s);
  case T_INT:    _(i);
  case T_LONG:   _(l);
  case T_FLOAT:  _(f);
  case T_DOUBLE: _(d);
#undef _
  default:
    YError("bad data type");
  }
}

/* almost the same as YGet_L */
static long *get_offset(Symbol *s, Dimension **dims)
{
  Operand op;

  if (! s->ops) YError("unexpected keyword argument");
  if (s->ops == &referenceSym && globTab[s->index].ops == &longScalar) {
    if (dims) *dims= 0;
    return &globTab[s->index].value.l;
  }
  switch (s->ops->FormOperand(s, &op)->ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
    op.ops->ToLong(&op);
  case T_LONG:
    if (dims) *dims = op.type.dims;
    return op.value;
  }
  YError("bad data type for index offsets");
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

  long number, x, y;
  long i, j, k, l, mark, count;
  voxel_t level;
  search_t s;

  mark = 1;
  number = width*height;
  for (i=0 ; i<number ; ++i) {
    if (region[i]) continue; /* pixel already marked */
    count = 0;           /* start a new region with zero pixels */
    level = img[i];      /* pixel value within current region */
    region[i] = mark;    /* mark current pixel */
    search[i] = ALL;     /* will check all neighbors of current pixel */
    index[count++] = i;  /* current pixel belongs to current region */
    for (j=0 ; j<count ; ++j) {
      k = index[j];
      x = k%width;
      y = k/width;
      s = search[k];

#define CHECK_PIXEL(DIRECTION, BOUND_CHECK, POSITION)	\
      if ((s & DIRECTION) && (BOUND_CHECK)) {		\
        l = POSITION;					\
        if (img[l] == level) {				\
          if (region[l] == mark) {			\
            search[l] &= (ALL & (~(DIRECTION)));	\
          } else {					\
            region[l] = mark;				\
            search[l] = (ALL & (~(DIRECTION)));		\
            index[count++] = l;				\
          }						\
        }						\
      }
      CHECK_PIXEL(LEFT,  x > 0,            k - 1);
      CHECK_PIXEL(RIGHT, x < (width - 1),  k + 1);
      CHECK_PIXEL(DOWN,  y > 0,            k - width);
      CHECK_PIXEL(UP,    y < (height - 1), k + width);
#undef CHECK_PIXEL
    }

    if (nb) {
      nb[mark - 1] = count;
    }
    if (x0 || x1) {
      long xmin, xmax;
      xmin = xmax = index[0]%width;
      for (j=1 ; j<count ; ++j) {
        x = index[j]%width;
        if (x < xmin) xmin = x;
        if (x > xmax) xmax = x;
      }
      if (x0) x0[mark - 1] = xmin;
      if (x1) x1[mark - 1] = xmax;
    }
    if (y0 || y1) {
      long ymin, ymax;
      ymin = ymax = index[0]/width;
      for (j=1 ; j<count ; ++j) {
        y = index[j]/width;
        if (y < ymin) ymin = y;
        if (y > ymax) ymax = y;
      }
      if (y0) y0[mark - 1] = ymin;
      if (y1) y1[mark - 1] = ymax;
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

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * fill-column: 78
 * coding: utf-8
 * End:
 */
