/*
 * yfftw.c -
 *
 * Implement support for FFTW (version 2), the "fastest Fourier transform in
 * the West", in Yorick.  This package is certainly outdated, consider using
 * XFFT instead (https://github.com/emmt/XFFT).
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "play.h"
#include "pstdlib.h"
#include "ydata.h"
#include "yio.h"

/* BUILT-IN ROUTINES */
extern BuiltIn Y_fftw, Y_fftw_plan;

#ifndef HAVE_FFTW
# define HAVE_FFTW 0
#endif

#if HAVE_FFTW
/*---------------------------------------------------------------------------*/

#if defined(FFTW_PREFIX) && (FFTW_PREFIX != 0)
# include "dfftw.h"
# include "drfftw.h"
#else
# include "fftw.h"
# include "rfftw.h"
#endif

#ifdef FFTW_ENABLE_FLOAT
# error only double precision real supported
#endif

/* Offset (in bytes) of MEMBER in structure TYPE. */
#define OFFSET_OF(type, member)    ((char*)&((type*)0)->member - (char*)0)

/* PRIVATE ROUTINES */
static int get_boolean(Symbol* s);
static void FreePlan(void* addr);
static void PrintPlan(Operand* op);

/*---------------------------------------------------------------------------*/
/* FFTW plan opaque object */

typedef struct y_fftw_plan_struct y_fftw_plan_t;

struct y_fftw_plan_struct {
  int  references; /* reference counter */
  Operations* ops; /* virtual function table */
  int       flags; /* FFTW flags */
  int         dir; /* transform direction FFTW_FORWARD or FFTW_BACKWARD */
  int        real; /* real transform? */
  void*      plan; /* FFTW plan for transform */
  void*       buf; /* NULL or an array of N complex numbers, that FFTW will
                      use as temporary space to perform the in-place
                      computation. */
  int        rank; /* dimensionality of the arrays to be transformed */
  int        dims[1]; /* Dimension list for FFTW which uses row-major format to
                      store arrays: the first dimension's index varies most
                      slowly and the last dimension's index varies most quickly
                      (i.e. opposite of Yorick interpreter but same order as
                      the chained dimension list).  _MUST_ BE LAST MEMBER
                      (actual size is max(rank,1)). */
};

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
extern MemberOp GetMemberX;

Operations fftwPlanOps = {
  &FreePlan, T_OPAQUE, 0, T_STRING, "fftw_plan",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &EvalX, &SetupX, &GetMemberX, &MatMultX, &PrintPlan
};

static void FreePlan(void* addr)
{
  if (addr) {
    y_fftw_plan_t* p = (y_fftw_plan_t*)addr;
    if (p->rank >= 1 && p->plan) {
      if (p->real)           rfftwnd_destroy_plan(p->plan);
      else if (p->rank == 1)    fftw_destroy_plan(p->plan);
      else                    fftwnd_destroy_plan(p->plan);
    }
    if (p->buf) p_free(p->buf);
    p_free(addr);
  }
}

static void PrintPlan(Operand* op)
{
  y_fftw_plan_t* p = (y_fftw_plan_t*)op->value;
  const char* dir;
  char line[80];
  int i, flags = p->flags;
  if (p->real) {
    if (p->dir == FFTW_REAL_TO_COMPLEX) dir = "REAL_TO_COMPLEX";
    else                                dir = "COMPLEX_TO_REAL";
  } else {
    if (p->dir == FFTW_FORWARD) dir = "FORWARD";
    else                        dir = "BACKWARD";
  }
  ForceNewline();
  PrintFunc("Object of type: ");
  PrintFunc(p->ops->typeName);
  sprintf(line, " (dims=[");
  PrintFunc(line);
  for (i=p->rank-1 ; i>=0 ; --i) {
    sprintf(line, (i >= 1 ? "%d," : "%d"), p->dims[i]);
    PrintFunc(line);
  }
#ifdef DEBUG
  sprintf(line, "], references=%d, dir=%s, flags=", p->references, dir);
#else
  sprintf(line, "], dir=%s, flags=", dir);
#endif
  PrintFunc(line);
  PrintFunc((flags & FFTW_IN_PLACE) ? "IN_PLACE" : "OUT_OF_PLACE");
  PrintFunc((flags & FFTW_MEASURE) ? "|MEASURE)" : "|ESTIMATE)");
  ForceNewline();
}

/* IMPLEMENTATION NOTES:
 *
 * FFTW can compute many different kinds of FFT's (1-D or n-D, in-place or
 * out-of-place, complex or real), here are the (hopefully motivated)
 * choices I have made:
 *
 *  . FFT of a scalar (rank=0) is a no-operation:
 *          y[l] = sum_{k=0}^{N-1} x[k]*exp(±i*2*PI*k*l/N)
 *      ==> y[0] = x[0]  when  N=1
 *
 *  . 1-D complex FFT's are computed in-place with a scratch buffer;
 *
 *  . n-D complex FFT's are computed in-place (no scratch buffer);
 *
 *  . All real FFT's use the rfftwnd calls (i.e. I do not want to translate
 *    the way 1-D real FFT's are organized in FFTW);
 *
 *  . Real to complex FFT's are computed in-place (no scratch buffer);
 *
 *  . Complex to real FFT's are computed out-of-place, since complex to
 *    real FFTW destroy the contents of input arrays, I take care of making
 *    a temporary copy as needed.
 *
 */
void Y_fftw_plan(int argc)
{
  /* Parse arguments from first to last one. */
  long* dimlist = NULL;
  long number = 0;
  long rank = 0;
  long len = 0;
  int measure = 0;
  int real = 0;
  int dir = 0;
  for (Symbol* stack = sp - argc + 1; stack <= sp; ++stack) {
    if (stack->ops) {
      /* Positional argument. */
      if (dimlist == NULL) {
        Operand op;
        stack->ops->FormOperand(stack, &op);
        switch (op.ops->typeID) {
        case T_CHAR:
        case T_SHORT:
        case T_INT:
          op.ops->ToLong(&op);
        case T_LONG:
          /* Check dimension list and compute rank. */
          dimlist = op.value;
          if (op.type.dims == NULL) {
            /* dimension list specified as a scalar */
            if ((number = dimlist[0]) <= 0) {
              goto bad_dimlist;
            }
            rank = (number > 1 ? 1 : 0);
          } else if (! op.type.dims->next) {
            /* dimension list specified as a vector */
            rank = dimlist[0];
            len = op.type.number;
            if (len != rank + 1) {
              goto bad_dimlist;
            }
            for (long i = 1; i < len; ++i) {
              if (dimlist[i] < 1) {
                goto bad_dimlist;
              }
            }
          } else {
          bad_dimlist:
            YError("bad dimension list");
          }
          break;
        default:
          YError("bad data type for dimension list");
        }
      } else if (dir == 0) {
        /* Use the same convention as in Yorick's FFT. */
        dir = YGetInteger(stack);
        if (dir == 1) {
          dir = FFTW_FORWARD;
        } else if (dir == -1) {
          dir = FFTW_BACKWARD;
        } else {
          YError("bad value for FFT direction");
        }
      } else {
        YError("too many arguments in fftw_plan");
      }
    } else {
      /* Keyword argument. */
      const char* keyword = globalTable.names[stack->index];
      ++stack;
      if (strcmp(keyword, "real") == 0) {
        real = get_boolean(stack);
      } else if (strcmp(keyword, "measure") == 0) {
        measure = get_boolean(stack);
      } else {
        YError("unknown keyword in fftw_plan");
      }
    }
  }
  if (dir == 0) {
    YError("too few arguments in fftw_plan");
  }

  /* Allocate new plan (with at least one slot for dims member) and push
     it on top of the stack. */
  y_fftw_plan_t* p;
  size_t size = (OFFSET_OF(y_fftw_plan_t, dims)
                 + (rank > 1 ? rank : 1)*sizeof(*p->dims));
  p = p_malloc(size);
  memset(p, 0, size);
  p->ops = &fftwPlanOps;
  PushDataBlock(p); /* _AFTER_ having set OPS */
  p->dir = dir;
  p->flags = ((measure ? FFTW_MEASURE : FFTW_ESTIMATE) |
              ((real && dir == FFTW_COMPLEX_TO_REAL) ?
               FFTW_OUT_OF_PLACE : FFTW_IN_PLACE));
  p->real = real;
  p->rank = rank;

  /* Store list of dimensions for this plan in row-major order. */
  if (len == 0) {
    p->dims[0] = number;
  } else {
    long i = 0;
    while (--len >= 1) p->dims[i++] = dimlist[len];
  }

  /* Create plan (noting to do for rank=0, because FFT of a scalar is a
     no-op). */
  if (rank >= 1) {
    if (real) {
      /* Always use n-D plan for real FFT (because storage of complex
         values in 1-D RFFTW is not very useful in a language like
         Yorick). */
      p->plan = rfftwnd_create_plan(rank, p->dims, p->dir, p->flags);
    } else if (rank == 1) {
      /* Allocate plan and scratch buffer for 1-D complex transforms. */
      p->plan = fftw_create_plan(p->dims[0], p->dir, p->flags);
      p->buf = p_malloc(2*sizeof(double)*p->dims[0]);
    } else {
      /* Allocate plan for N-D complex transforms. */
      p->plan =  fftwnd_create_plan(rank, p->dims, p->dir, p->flags);
    }
    if (! p->plan) YError("failed to create FFTW plan");
  }
#ifdef DEBUG
  for (int i = 0; i < p->rank; ++i) {
    printf("dims[%d]=%d\n", i, p->dims[i]);
  }
#endif
}

void Y_fftw(int argc)
{
  if (argc != 2) YError("fftw takes exactly 2 arguments");

  /* Get FFTW plan. */
  Symbol* s = sp;
  if (s->ops == &referenceSym) s = &globTab[s->index];
  if (s->ops == NULL) YError("unexpected keyword");
  if (s->ops != &dataBlockSym ||
      s->value.db->ops != &fftwPlanOps) YError("expecting a FFTW plan");
  y_fftw_plan_t* p = (y_fftw_plan_t*)s->value.db;

  /* Get input array and check its element type. */
  s = sp - 1;
  if (s->ops == NULL) YError("unexpected keyword");
  Operand op;
  s->ops->FormOperand(s, &op);
  int real_to_complex = (p->real && p->dir == FFTW_REAL_TO_COMPLEX);
  int complex_to_real = (p->real && p->dir == FFTW_COMPLEX_TO_REAL);
  switch (op.ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
  case T_LONG:
  case T_FLOAT:
  case T_DOUBLE:
    break;
  case T_COMPLEX:
    if (! real_to_complex) break;
  default:
    YError("bad data type for input array");
  }

  /* Check dimension list. */
  int* dims = p->dims;
  int rank = p->rank;
  Dimension* dimlist = op.type.dims;
  int i = 0;
  while (dimlist != NULL) {
    if (i >= rank || dimlist->number != ((complex_to_real && i == rank-1) ?
                                         dims[i]/2+1 : dims[i])) {
      i = -1; /* trigger error below */
      break;
    }
    ++i;
    dimlist = dimlist->next;
  }
  if (i != rank) {
    YError("dimension list of input array incompatible with FFTW plan");
  }
  if (rank == 0) {

    /*********************************
     ***                           ***
     ***   transform of a scalar   ***
     ***                           ***
     *********************************/

    if (complex_to_real) op.ops->ToDouble(&op);
    else                 op.ops->ToComplex(&op);

  } else if (real_to_complex) {

    /*******************************
     ***                         ***
     ***   real -> complex FFT   ***
     ***                         ***
     *******************************/
    const double zero = 0.0;

    /* Push a complex array with proper dimensions to store
       the result on top of the stack. */
    long nr = dims[rank - 1]; /* number of real's along 1st dim */
    long nhc = nr/2 + 1;      /* number of complex's along 1st dim */
    if (tmpDims) {
      Dimension* oldDims = tmpDims;
      tmpDims = 0;
      FreeDimension(oldDims);
    }
    tmpDims = NewDimension(nhc, 1, tmpDims);
    long n = 1;
    for (int i = rank - 2; i >= 0; --i) {
      long len = dims[i];
      n *= len;
      tmpDims = NewDimension(len, 1, tmpDims);
    }
    if (n*nr != op.type.number) {
      YError("BUG in dimension list code");
    }
    Array* array = NewArray(&complexStruct, tmpDims);
    PushDataBlock(array);
    void* inp = array->value.d;

    /* Copy input into output array, taking care of padding (zero padding
       is in case rank=0). */
    double* cptr = inp; /* complex array as double */
    long nc = 2*nhc; /* complex array as double */
#define COPY(type_t)                                            \
    {                                                           \
      type_t* rptr = op.value;                                  \
      for (long j = 0; j < n; ++j, rptr += nr, cptr += nc) {	\
        for (i=0 ; i<nr ; ++i) cptr[i] = rptr[i];               \
        for (    ; i<nc ; ++i) cptr[i] = zero;                  \
      }                                                         \
    }                                                           \
    break
    switch (op.ops->typeID) {
    case T_CHAR:   COPY(char);
    case T_SHORT:  COPY(short);
    case T_INT:    COPY(int);
    case T_LONG:   COPY(long);
    case T_FLOAT:  COPY(float);
    case T_DOUBLE: COPY(double);
    }
#undef COPY
    /* Replace input array by output one. */
    PopTo(op.owner);

    /* Apply the real->complex transform. */
    rfftwnd_one_real_to_complex(p->plan, inp, NULL);

  } else {

    /******************************************
     ***                                    ***
     ***   complex -> real or complex FFT   ***
     ***                                    ***
     ******************************************/

    /* Make sure input array is complex and a temporary one (either because
       it will be the result or because FFTW_COMPLEX_TO_REAL destroys its
       input). */
    void* inp = NULL; /* address of array to transform */
    switch (op.ops->typeID) {
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_FLOAT:
    case T_DOUBLE:
      /* Convert input in a new complex array. */
      op.ops->ToComplex(&op);
      inp = op.value;
      break;
    case T_COMPLEX:
      /* If input array has references (is not temporary), make a new copy. */
      if (op.references > 0) {
        Array* array = NewArray(&complexStruct, op.type.dims);
        PushDataBlock(array);
        inp = array->value.d;
        memcpy(inp, op.value, 2*op.type.number*sizeof(double));
        PopTo(op.owner);
      } else {
        inp = op.value;
      }
      break;
    }

    if (p->real) {
      /* Push output real array and apply out-of-place complex to real
         transform, then pop output array in place of (temporary) input
         one.  */
      if (tmpDims) {
        Dimension* oldDims = tmpDims;
        tmpDims = 0;
        FreeDimension(oldDims);
      }
      for (int i = rank - 1; i >= 0; --i) {
        tmpDims = NewDimension(dims[i], 1, tmpDims);
      }
      Array* array = NewArray(&doubleStruct, tmpDims);
      PushDataBlock(array);
      rfftwnd_one_complex_to_real(p->plan, inp, array->value.d);
      PopTo(sp - 2);
    } else if (rank == 1) {
      /* Apply in-place 1-D  complex transform with scratch buffer. */
      fftw_one(p->plan, inp, p->buf);
    } else {
      /* Apply in-place n-D complex transform (no scratch buffer). */
      fftwnd_one(p->plan, inp, NULL);
    }
  }

  /* Drop FFTW plan and left result on top of the stack. */
  Drop(1);
}

/*---------------------------------------------------------------------------*/

static int get_boolean(Symbol* s)
{
  if (s->ops == &referenceSym) s = &globTab[s->index];
  if (s->ops == &intScalar)    return (s->value.i != 0);
  if (s->ops == &longScalar)   return (s->value.l != 0L);
  if (s->ops == &doubleScalar) return (s->value.d != 0.0);
  if (s->ops == &dataBlockSym) {
    Operand op;
    s->ops->FormOperand(s, &op);
    if (! op.type.dims) {
      switch (op.ops->typeID) {
      case T_CHAR:   return (*(char*  )op.value != 0);
      case T_SHORT:  return (*(short* )op.value != 0);
      case T_INT:    return (*(int*   )op.value != 0);
      case T_LONG:   return (*(long*  )op.value != 0L);
      case T_FLOAT:  return (*(float* )op.value != 0.0F);
      case T_DOUBLE: return (*(double*)op.value != 0.0);
      case T_COMPLEX:return (((double*)op.value)[0] != 0.0 ||
                             ((double*)op.value)[1] != 0.0);
      case T_STRING: return (op.value != NULL);
      case T_VOID:   return 0;
      }
    }
  }
  YError("bad non-boolean argument");
  return 0; /* avoid compiler warning */
}

/*---------------------------------------------------------------------------*/
#else /* not HAVE_FFTW */

static char* no_fftw_support = "no FFTW support in this version of Yorick";

void Y_fftw(int nargs) { YError(no_fftw_support); }
void Y_fftw_plan(int nargs) { YError(no_fftw_support); }

#endif /* not HAVE_FFTW */
