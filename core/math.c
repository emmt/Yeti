/*
 * math.c -
 *
 * Additional math builtin functions and generalized matrix-vector
 * multiplication for Yorick.
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
 * Utility macros: (X)STRINGIFY takes an argument and wraps it in "" (double
 * quotation marks), (X)JOIN joins two arguments.  The version prefixed by an X
 * are capable of performing macro expansion of their arguments.
 */
#define VERBATIM(x) x
#define XSTRINGIFY(x)  STRINGIFY(x)
#define STRINGIFY(x)   #x
#define XJOIN(a,b)     JOIN(a,b)
#define JOIN(a,b)      a##b

extern BuiltIn Y_sinc;
extern BuiltIn Y_arc;

/* same as PopToD in ops0.c */
static void pop_to_d(Symbol* s)
{
  Array* array = (Array*)sp->value.db;
  PopTo(s);
  if (s->ops == &dataBlockSym && array->type.dims == NULL) {
    s->ops = &doubleScalar;
    s->value.d = array->value.d[0];
    Unref(array);
  }
}

/* similar to BuildResultU in ops0.c */
static void* build_result(Operand* op, StructDef* base)
{
  if (op->references == 0 && op->type.base == base) {
    /* similar to PushCopy in ydata.c */
    Symbol* stack = sp + 1;
    Symbol* s = op->owner;
    int isDB = (s->ops == &dataBlockSym);
    stack->ops = s->ops;
    if (isDB) {
      stack->value.db = Ref(s->value.db);
    } else {
      stack->value = s->value;
    }
    sp = stack; /* sp updated AFTER new stack element intact */
    return (isDB ? op->value : &sp->value);
  } else {
    Array* arr = (Array*)PushDataBlock(NewArray(base, op->type.dims));
    return (void*)(arr->value.c);
  }
}

typedef void math_func_f(float dst[restrict],
                         const float src[restrict], long number);

typedef void math_func_d(double dst[restrict],
                         const double src[restrict], long number);

typedef void math_func_z(double dst[restrict],
                         const double src[restrict], long number);

static void apply_unary_math_func(int argc,
                                  math_func_f* func_f,
                                  math_func_d* func_d,
                                  math_func_z* func_z)
{
  if (argc != 1) yor_error("expecting exactly one argument");
  if (sp->ops == NULL) yor_unexpected_keyword_argument();
  Operand op;
  sp->ops->FormOperand(sp, &op);
  int type = op.ops->typeID;
  if (type == YOR_FLOAT && func_f != NULL) {
    func_f(build_result(&op, &floatStruct), op.value, op.type.number);
    PopTo(sp - 2);
  } else if (type <= YOR_DOUBLE && func_d != NULL) {
    if (type < YOR_DOUBLE) {
      op.ops->ToDouble(&op);
      type = op.ops->typeID;
    }
    func_d(build_result(&op, &doubleStruct), op.value, op.type.number);
    pop_to_d(sp - 2);
  } else if (type == YOR_COMPLEX && func_z != NULL) {
    func_z(build_result(&op, &complexStruct), op.value, op.type.number);
    PopTo(sp - 2);
  } else {
    yor_error("unexpected type of argument");
  }
  Drop(1);
}

/* ----- sinc(x) = sin(PI*x)/PI/x ----- */

static void sinc_f(float dst[restrict], const float src[restrict], long n)
{
#if NORMALIZED_SINC
  const float pi = PI;
#endif
  for (long i = 0; i < n; ++i) {
    float x = src[i];
    if (x == 0.0) {
      dst[i] = 1.0;
    } else {
#if NORMALIZED_SINC
      x *= pi;
#endif
      dst[i] = sinf(x)/x;
    }
  }
}

static void sinc_d(double dst[restrict], const double src[restrict], long n)
{
#if NORMALIZED_SINC
  const double pi = PI;
#endif
  for (long i = 0; i < n; ++i) {
    double x = src[i];
    if (x == 0.0) {
      dst[i] = 1.0;
    } else {
#if NORMALIZED_SINC
      x *= pi;
#endif
      dst[i] = sin(x)/x;
    }
  }
}

static void sinc_z(double dst[restrict], const double src[restrict], long n)
{
#if NORMALIZED_SINC
  const double pi = PI;
#endif
  for (long i = 0; i < n; i += 2) {
    double rr = src[i];
    double ri = src[i+1];
    if (rr == 0 && ri == 0) {
      dst[i] = 1.0;
      dst[i+1] = 0.0;	/* Not needed? */
    } else {
#if NORMALIZED_SINC
      rr *= pi;
      ri *= pi;
#endif
      double lr = sin(rr) * cosh(ri);
      double li = cos(rr) * sinh(ri);
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
    }
  }
}

void Y_sinc(int argc)
{
  apply_unary_math_func(argc, sinc_f, sinc_d, sinc_z);
}

/*---------------------------------------------------------------------------*/
/* ARC */

static void arc_f(float dst[restrict], const float src[restrict], long n)
{
  const float rad = XJOIN(TWO_PI,F);
  const float scl = XJOIN(ONE_OVER_TWO_PI,F);
  for (long i = 0; i < n; ++i) {
    dst[i] = src[i] - rad*roundf(scl*src[i]);
  }
}

static void arc_d(double dst[restrict], const double src[restrict], long n)
{
  const double rad = TWO_PI;
  const double scl = ONE_OVER_TWO_PI;
  for (long i = 0; i < n; ++i) {
    dst[i] = src[i] - rad*round(scl*src[i]);
  }
}

void Y_arc(int argc)
{
  apply_unary_math_func(argc, arc_f, arc_d, NULL);
}
