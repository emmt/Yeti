/*
 * yeti_math.c -
 *
 * Additional math builtin functions and generalized matrix-vector
 * multiplication for Yorick.
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

#include <math.h>
#include "config.h"
#include "yeti.h"
#include "ydata.h"
#include "bcast.h"
#include "yio.h"

/* Some constants. */
#define PI              3.141592653589793238462643383279502884197
#define TWO_PI          6.283185307179586476925286766559005768394
#define ONE_OVER_TWO_PI 0.1591549430918953357688837633725143620345

/* Use definition of sinc in use in signal processing (i.e. normalized
   sinc) otherwise use mathematical definition. */
#ifndef NORMALIZED_SINC
# define NORMALIZED_SINC 1
#endif


/*
 * Utility macros: STRINGIFY takes an argument and wraps it in "" (double
 * quotation marks), JOIN joins two arguments.  Both are capable of
 * performing macro expansion of their arguments.
 */
#define VERBATIM(x) x
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
# define STRINGIFY(x)   STRINGIFY1(x)
# define STRINGIFY1(x)  #x
# define JOIN(a,b)      JOIN1(a,b)
# define JOIN1(a,b)     a##b
#else
# define STRINGIFY(x)   "x"
# define JOIN(a,b)      VERBATIM(a)/**/VERBATIM(b)
#endif


extern BuiltIn Y_sinc;

/* ANSI standard math.h functions */
extern double sin(double);
extern double cos(double);
extern double tan(double);
extern double asin(double);
extern double acos(double);
extern double atan(double);
extern double atan2(double, double);
extern double sinh(double);
extern double cosh(double);
extern double tanh(double);
extern double exp(double);
extern double log(double);
extern double log10(double);
extern double sqrt(double);
extern double ceil(double);
extern double floor(double);

/* function either present in math library or implemented in nonc.c */
extern double hypot(double, double);

/* Some functions and definitions stolen from Yorick std0.c and ops0.c
   in order to not use 'private' Yorick API. */
typedef void looper_t(double *dst, const double *src, const long n);
static void *build_result(Operand *op, StructDef *base);
static void unary_worker(int nArgs, looper_t *DLooper, looper_t *ZLooper);
static void pop_to_d(Symbol *s);

/* same as PopToD in ops0.c */
static void pop_to_d(Symbol *s)
{
  Array *array = (Array *)sp->value.db;
  PopTo(s);
  if (s->ops==&dataBlockSym && !array->type.dims) {
    s->ops= &doubleScalar;
    s->value.d= array->value.d[0];
    Unref(array);
  }
}

/* similar to BuildResultU in ops0.c */
static void *build_result(Operand *op, StructDef *base)
{
  if (! op->references && op->type.base == base) {
    /* similar to PushCopy in ydata.c */
    Symbol *stack = sp + 1;
    Symbol *s = op->owner;
    int isDB = (s->ops == &dataBlockSym);
    stack->ops = s->ops;
    if (isDB) stack->value.db = Ref(s->value.db);
    else stack->value = s->value;
    sp = stack; /* sp updated AFTER new stack element intact */
    return (isDB ? op->value : &sp->value);
  } else {
    return (void *)(((Array *)(PushDataBlock(NewArray(base, op->type.dims))))->value.c);
  }
}

static void unary_worker(int nArgs, looper_t *DLooper, looper_t *ZLooper)
{
  Operand op;
  int promoteID;
  if (nArgs!=1) YError("expecting exactly one argument");
  if (!sp->ops) YError("unexpected keyword");
  sp->ops->FormOperand(sp, &op);
  promoteID = op.ops->promoteID;
  if (promoteID <= T_DOUBLE) {
    if (promoteID < T_DOUBLE) op.ops->ToDouble(&op);
    DLooper(build_result(&op, &doubleStruct), op.value, op.type.number);
    pop_to_d(sp - 2);
  } else {
    if (promoteID>T_COMPLEX) YError("expecting numeric argument");
    ZLooper(build_result(&op, &complexStruct), op.value, 2*op.type.number);
    PopTo(sp - 2);
  }
  Drop(1);
}

/* ----- sinc(x) = sin(PI*x)/PI/x ----- */

static void sincDLoop(double *dst, const double *src, const long n);
static void sincZLoop(double *dst, const double *src, const long n) ;

void Y_sinc(int nArgs) { unary_worker(nArgs, &sincDLoop, &sincZLoop); }

static void sincDLoop(double *dst, const double *src, const long n)
{
#if NORMALIZED_SINC
  const double pi = PI;
#endif
  double x;
  long i;

  for (i=0 ; i<n ; ++i) {
    x = src[i];
    if (x) {
#if NORMALIZED_SINC
      x *= pi;
#endif
      dst[i] = sin(x)/x;
    } else {
      dst[i] = 1.0;
    }
  }
}

static void sincZLoop(double *dst, const double *src, const long n)
{
#if NORMALIZED_SINC
  const double pi = PI;
#endif
  double lr, li, rr, ri;
  long i;

  for (i=0 ; i<n ; i+=2) {
    rr = src[i];
    ri = src[i+1];
    if (rr || ri) {
#if NORMALIZED_SINC
      rr *= pi;
      ri *= pi;
#endif
      lr = sin(rr) * cosh(ri);
      li = cos(rr) * sinh(ri);
      /* Take care of overflows (this piece of code should be faster than
         DivideZ() in Yorick/ops2.c).  We already know that (rr,ri) != (0,0),
         nevertheless, Yorick catch divisions by zero. */
      if (fabs(rr) > fabs(ri)) {
        ri /= rr;
        rr = 1.0/((1.0 + ri*ri)*rr);
        dst[i]   = (lr + li*ri)*rr;
        dst[i+1] = (li - lr*ri)*rr;
      } else {
        rr /= ri;
        ri = 1.0/((1.0 + rr*rr)*ri);
        dst[i]   = (lr*rr + li)*ri;
        dst[i+1] = (li*rr - lr)*ri;
      }
    } else {
      dst[i] = 1.0;
      dst[i+1] = 0.0;	/* Not needed? */
    }
  }
}

/*---------------------------------------------------------------------------*/
/* ARC */

extern BuiltIn Y_arc;

void Y_arc(int nArgs)
{
  Operand op;
  int promoteID;
  long number, i;
  if (nArgs != 1) YError("arc takes exactly one argument");
  if (! sp->ops) YError("unexpected keyword");
  sp->ops->FormOperand(sp, &op);
  promoteID = op.ops->promoteID;
  if (promoteID == T_DOUBLE) {
    const double rad = TWO_PI;
    const double scl = ONE_OVER_TWO_PI;
    double *x, *y;
    x = op.value;
    y = build_result(&op, &doubleStruct);
    number = op.type.number;
    for (i=0 ; i<number ; ++i) y[i] = x[i] - rad*round(scl*x[i]);
    pop_to_d(sp - 2);
  } else if (promoteID <= T_FLOAT) {
    const float rad = JOIN(TWO_PI,F);
    const float scl = JOIN(ONE_OVER_TWO_PI,F);
    float *x, *y;
    if (promoteID != T_FLOAT) op.ops->ToFloat(&op);
    x = op.value;
    y = build_result(&op, &floatStruct);
    number = op.type.number;
    for (i=0 ; i<number ; ++i) y[i] = x[i] - rad*roundf(scl*x[i]);
    PopTo(sp - 2);
  } else {
    YError("expecting non-complex numeric argument");
  }
  Drop(1);
}

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
