/*
 * misc.c -
 *
 * Implement miscellaneous builtin functions for Yorick.
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include <play.h>
#include <pstdio.h>
#include <yapi.h>
#include <yio.h>

#include "config.h"
#include "yeti.h"

#if defined(__GNUC__) && __GNUC__ > 1
extern void YError(const char* msg) __attribute__ ((noreturn));
static void unexpected_keyword_argument() __attribute__ ((noreturn));
#endif

static void unexpected_keyword_argument()
{
  YError("unexpected keyword argument");
}

/* Shall we use faster complex division? (depends Yorick version) */
#if (YORICK_VERSION_MAJOR >= 2)
# define USE_FASTER_DIVIDE_Z 0
#elif (YORICK_VERSION_MAJOR == 1 && YORICK_VERSION_MINOR >= 6)
# define USE_FASTER_DIVIDE_Z 0
#elif (YORICK_VERSION_MAJOR == 1 && YORICK_VERSION_MINOR == 5 && YORICK_VERSION_MICRO >= 15)
# define USE_FASTER_DIVIDE_Z 0
#else
# define USE_FASTER_DIVIDE_Z 1
#endif

/* Built-in functions defined in this file: */
extern BuiltIn Y_yeti_init;
extern BuiltIn Y_mem_base, Y_mem_copy, Y_mem_peek;
extern BuiltIn Y_get_encoding;
extern BuiltIn Y_nrefsof;
extern BuiltIn Y_smooth3;
extern BuiltIn Y_insure_temporary;

/*---------------------------------------------------------------------------*/
/* INITIALIZATION OF YETI */

/* The order of parsing of startup files is as follows:
 *   1. Yorick startup scripts: paths.i, std.i, graph.i, matrix.i, fft.i;
 *   2. Package(s) startup scripts: yeti.i, ...;
 *   3. Yorick post-initialization: stdx.i  (just call 'set_path').
 *
 * It is therefore possible to fool Yorick post-initialization by
 * changing builtin function 'set_path' to something else.
 *
 * Until step 3, search path include the launch directory.
 * Built-in 'set_site' function is called at statup by 'std.i' to
 * define global variables:
 *   Y_LAUNCH    the directory containing the Yorick executable
 *   Y_VERSION   Yorick's version as "MAJOR.MINOR.MICRO"
 *   Y_HOME      Yorick's "site directory" with machine dependent files
 *   Y_SITE      Yorick's "site directory" with machine independent files
 */

/* Symbols defined in std0.c: */
extern char* yLaunchDir;
extern int yBatchMode;

/* Symbols defined in ops0.c: */
extern void* BuildResult2(Operand* l, Operand* r);

/* Symbols defined in ycode.c: */
extern char* yHomeDir;  /* e.g., "/usr/local/lib/yorick/1.5"   */
extern char* ySiteDir;  /* e.g., "/usr/local/share/yorick/1.5" */
extern char* yUserPath; /* e.g., ".:~/yorick:~/Yorick"         */

static void globalize_string(const char* name, const char* value);
static void globalize_long(const char* name, long value);


#if USE_FASTER_DIVIDE_Z
static void fast_DivideZ(Operand* l, Operand* r);
#endif /* USE_FASTER_DIVIDE_Z */

void Y_yeti_init(int argc)
{
  const char* version = YETI_XSTRINGIFY(YETI_VERSION_MAJOR) "." \
    YETI_XSTRINGIFY(YETI_VERSION_MINOR) "." \
    YETI_XSTRINGIFY(YETI_VERSION_MICRO) YETI_VERSION_SUFFIX;

#if USE_FASTER_DIVIDE_Z
  /* Replace complex division by faster code. */
  complexOps.Divide = fast_DivideZ;
#endif /* USE_FASTER_DIVIDE_Z */

  /* Restore global variables. */
  globalize_string("YETI_VERSION", version);
  globalize_long("YETI_VERSION_MAJOR", YETI_VERSION_MAJOR);
  globalize_long("YETI_VERSION_MINOR", YETI_VERSION_MINOR);
  globalize_long("YETI_VERSION_MICRO", YETI_VERSION_MICRO);
  globalize_string("YETI_VERSION_SUFFIX", YETI_VERSION_SUFFIX);
  if (! CalledAsSubroutine()) {
    yeti_push_string_value(version);
  }
}

static void globalize_string(const char* name, const char* value)
{
  long index = Globalize(name, 0L);
  DataBlock* old = (globTab[index].ops == &dataBlockSym ?
                    globTab[index].value.db : 0);
  Array* obj = NewArray(&stringStruct, (Dimension*)0);
  globTab[index].ops = &intScalar; /* in case of interrupt */
  globTab[index].value.db = (DataBlock*)obj;
  globTab[index].ops = &dataBlockSym;
  Unref(old);
  obj->value.q[0] = p_strcpy(value);
}

static void globalize_long(const char* name, long value)
{
  long index = Globalize(name, 0L);
  DataBlock* old = (globTab[index].ops == &dataBlockSym ?
                    globTab[index].value.db : 0);
  globTab[index].ops = &longScalar; /* in case of interrupt */
  globTab[index].value.l = value;
  Unref(old);
}

#if USE_FASTER_DIVIDE_Z
/* Faster code for complex division (save 1 division out of 3 with
   respect to original Yorick DivideZ code resulting in ~33% faster
   code). */
static void fast_DivideZ(Operand* l, Operand* r)
{
  const double one = 1.0;
  double* dst = BuildResult2(l, r);
  if (dst == NULL) YError("operands not conformable in binary /");
  size_t n = l->type.number;
  const double* lv = l->value;
  const double* rv = r->value;
  for (size_t i = 0; i < n; ++i) {
    /* extract values (watch out for dst == lv or rv) */
    double lr = lv[2*i];
    double li = lv[2*i+1];
    double rr = rv[2*i];
    double ri = rv[2*i+1];
    if ((rr > 0 ? rr : -rr) > (ri > 0 ? ri : -ri)) { /* be careful about overflow... */
      ri /= rr;
      rr = one/((one + ri*ri)*rr);
      dst[2*i] = (lr + li*ri)*rr;
      dst[2*i+1] = (li - lr*ri)*rr;
    } else {
      rr /= ri; /* do not care of division by zero here, since Yorick
                   catches floating point exceptions */
      ri = one/((one + rr*rr)*ri);
      dst[2*i] = (lr*rr + li)*ri;
      dst[2*i+1] = (li*rr - lr)*ri;
    }
  }
  PopTo(l->owner);
}
#endif /* USE_FASTER_DIVIDE_Z */

/*---------------------------------------------------------------------------*/
/* MEMORY HACKING ROUTINES */

static void* get_address(Symbol* s);
static void build_dimlist(Symbol* stack, int nArgs);
static Operand* form_operand_db(Symbol* owner, Operand* op);

void Y_mem_base(int argc)
{
  if (argc != 1) YError("mem_base takes exactly 1 argument");

  /*** based on Address() in ops3.c ***/

  /* Taking the address of a variable X, where X is a scalar constant,
     causes X to be replaced by an Array.  This is obscure, but there is no
     other obvious way to get both the efficiency of the scalar Symbols,
     AND the reference-count safety of Yorick pointers.  Notice that if the
     address of a scalar is taken, the efficient representation is lost.  */
  if (sp->ops != &referenceSym) {
  bad_arg:
    YError("expected a reference to an array object");
  }
  Array* array;
  Symbol* s = &globTab[sp->index];
  OpTable* ops = s->ops;
  if (ops == &dataBlockSym) {
    array = (Array*)s->value.db;
  } else if (ops == &doubleScalar) {
    array = NewArray(&doubleStruct, (Dimension*)0);
    array->value.d[0] = s->value.d;
    s->value.db = (DataBlock*)array;
    s->ops = &dataBlockSym;
  } else if (ops == &longScalar) {
    array = NewArray(&longStruct, (Dimension*)0);
    array->value.l[0] = s->value.l;
    s->value.db = (DataBlock*)array;
    s->ops = &dataBlockSym;
  } else if (ops == &intScalar) {
    array = NewArray(&intStruct, (Dimension*)0);
    array->value.i[0] = s->value.i;
    s->value.db = (DataBlock*)array;
    s->ops = &dataBlockSym;
  } else {
    goto bad_arg;
  }
  if (! array->ops->isArray) goto bad_arg;
  long value = (long)array->value.c;
  Drop(2);
  PushLongValue(value);
}

void Y_mem_copy(int argc)
{
  if (argc != 2) YError("mem_copy takes exactly 2 arguments");
  void* address = get_address(sp - 1);
  Symbol* s = (sp->ops == &referenceSym) ? &globTab[sp->index] : sp;
  if (s->ops == &doubleScalar) {
    (void)memcpy(address, &(s->value.d), sizeof(double));
  } else if (s->ops == &longScalar) {
    (void)memcpy(address, &(s->value.l), sizeof(long));
  } else if (s->ops == &intScalar) {
    (void)memcpy(address, &(s->value.i), sizeof(int));
  } else if (s->ops == &dataBlockSym && s->value.db->ops->isArray) {
    Array* array = (Array*)s->value.db;
    (void)memcpy(address, array->value.c,
                 array->type.number*array->type.base->size);
  } else {
    YError("unexpected non-array data");
  }
}

void Y_mem_peek(int argc)
{
  if (argc < 2) YError("mem_peek takes at least 2 arguments");
  Symbol* stack = sp - argc + 1;
  void* address = get_address(stack);
  Symbol* s = stack + 1;
  if (s->ops == &referenceSym) s = &globTab[s->index];
  if (s->ops != &dataBlockSym || s->value.db->ops != &structDefOps)
    YError("expected type definition as second argument");
  StructDef* base = (StructDef*)s->value.db;
  if (base->dataOps->typeID < T_CHAR || base->dataOps->typeID > T_COMPLEX)
    YError("only basic data types are supported");
  build_dimlist(stack + 2, argc - 2);
  Array* array = PushDataBlock(NewArray(base, tmpDims));
  memcpy(array->value.c, address, array->type.number*array->type.base->size);
}

static void* get_address(Symbol* s)
{
  if (s->ops == NULL) unexpected_keyword_argument();
  Operand op;
  s->ops->FormOperand(s, &op);
  if (op.type.dims == (Dimension*)0) {
    if (op.ops->typeID == T_LONG) return (void*)*(long*)op.value;
    if (op.ops->typeID == T_POINTER) return (void*)*(void**)op.value;
  }
  YError("bad address (expecting long integer or pointer scalar)");
  return (void*)0; /* avoid compiler warning */
}

/* The following function is a pure copy of BuildDimList in 'ops3.c' of
   Yorick source code -- required to avoid plugin clash. */
static void build_dimlist(Symbol* stack, int nArgs)
{
  Dimension* tmp= tmpDims;
  tmpDims= 0;
  FreeDimension(tmp);
  while (nArgs--) {
    if (stack->ops==&referenceSym) ReplaceRef(stack);
    if (stack->ops==&longScalar) {
      if (stack->value.l<=0) goto badl;
      tmpDims= NewDimension(stack->value.l, 1L, tmpDims);
    } else if (stack->ops==&intScalar) {
      if (stack->value.i<=0) goto badl;
      tmpDims= NewDimension(stack->value.i, 1L, tmpDims);

    } else if (stack->ops==&dataBlockSym) {
      Operand op;
      form_operand_db(stack, &op);
      if (op.ops==&rangeOps) {
        Range* range= op.value;
        long len;
        if (range->rf || range->nilFlags || range->inc!=1)
          YError("only min:max ranges allowed in dimension list");
        len= range->max-range->min+1;
        if (len<=0) goto badl;
        tmpDims= NewDimension(len, range->min, tmpDims);

      } else if (op.ops->promoteID<=T_LONG &&
                 (!op.type.dims || !op.type.dims->next)) {
        long len;
        op.ops->ToLong(&op);
        if (!op.type.dims) {
          len= *(long*)op.value;
          if (len<=0) goto badl;
          tmpDims= NewDimension(len, 1L, tmpDims);
        } else {
          long* dim= op.value;
          long n= *dim++;
          if (n>10 || n>=op.type.number)
            YError("dimension list format [#dims, len1, len2, ...]");
          while (n--) {
            len= *dim++;
            if (len<=0) goto badl;
            tmpDims= NewDimension(len, 1L, tmpDims);
          }
        }

      } else if (op.ops!=&voidOps) {
        goto badl;
      }
    } else {
    badl:
      YError("bad dimension list");
    }
    stack++;
  }
}

/* The following function is a pure copy of FormOperandDB in 'ops0.c' of
   Yorick source code -- required to avoid plugin clash. */
static Operand* form_operand_db(Symbol* owner, Operand* op)
{
  DataBlock* db = owner->value.db;
  Operations* ops = db->ops;
  op->owner = owner;
  if (ops->isArray) {
    Array* array= (Array*)db;
    op->ops         = ops;
    op->references  = array->references;
    op->type.base   = array->type.base;
    op->type.dims   = array->type.dims;
    op->type.number = array->type.number;
    op->value       = array->value.c;
  } else if (ops == &lvalueOps) {
    LValue* lvalue = (LValue*)db;
    StructDef* base = lvalue->type.base;
    if (lvalue->strider || base->model) {
      Array* array = FetchLValue(lvalue, owner);
      op->ops         = array->ops;
      op->references  = array->references;
      op->type.base   = array->type.base;
      op->type.dims   = array->type.dims;
      op->type.number = array->type.number;
      op->value       = array->value.c;
    } else {
      op->ops         = base->dataOps;
      op->references  = 1;     /* NEVER try to use this as result */
      op->type.base   = base;
      op->type.dims   = lvalue->type.dims;
      op->type.number = lvalue->type.number;
      op->value       = lvalue->address.m;
    }
  } else {
    op->ops         = ops;
    op->references  = db->references;
    op->type.base   = 0;
    op->type.dims   = 0;
    op->type.number = 0;
    op->value       = db;
  }
  return op;
}

/*---------------------------------------------------------------------------*/
/* DATA ENCODING */

#include "prmtyp.h"

void Y_get_encoding(int argc)
{
  static struct {
    const char* name;
    long        layout[32];
  } db[] = {
    {"alpha", {1,1,-1, 2,2,-1, 4,4,-1, 8,8,-1, 4,4,-1, 8,8,-1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"cray",  {1,1,1, 8,8,1, 8,8,1, 8,8,1, 8,8,1, 8,8,1,
               0,1,15,16,48,1,16384, 0,1,15,16,48,1,16384}},
    {"dec",   {1,1,-1, 2,2,-1, 4,4,-1, 4,4,-1, 4,4,-1, 8,8,-1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"i86",   {1,1,-1, 2,2,-1, 4,4,-1, 4,4,-1, 4,4,-1, 8,4,-1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"ibmpc", {1,1,-1, 2,2,-1, 2,2,-1, 4,2,-1, 4,2,-1, 8,2,-1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"mac",   {1,1,1, 2,2,1, 2,2,1, 4,2,1, 4,2,1, 8,2,1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"macl",  {1,1,1, 2,2,1, 2,2,1, 4,2,1, 4,2,1, 12,2,1,
               0,1,8,9,23,0,127, 0,1,15,32,64,1,16382}},
    {"sgi64", {1,1,1, 2,2,1, 4,4,1, 8,8,1, 4,4,1, 8,8,1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"sun",   {1,1,1, 2,2,1, 4,4,1, 4,4,1, 4,4,1, 8,8,1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"sun3",  {1,1,1, 2,2,1, 4,2,1, 4,2,1, 4,2,1, 8,2,1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"vax",   {1,1,-1, 2,1,-1, 4,1,-1, 4,1,-1, 4,1,2, 8,1,2,
               0,1,8,9,23,0,129, 0,1,8,9,55,0,129}},
    {"vaxg",  {1,1,-1, 2,1,-1, 4,1,-1, 4,1,-1, 4,1,2, 8,1,2,
               0,1,8,9,23,0,129, 0,1,11,12,52,0,1025}},
    {"xdr",   {1,1,1, 2,2,1, 4,4,1, 4,4,1, 4,4,1, 8,4,1,
               0,1,8,9,23,0,127, 0,1,11,12,52,0,1023}},
    {"native", {sizeof(char),   P_STRUCT_ALIGN, 0,
                sizeof(short),  P_SHORT_ALIGN,  P_SHORT_ORDER,
                sizeof(int),    P_INT_ALIGN,    P_INT_ORDER,
                sizeof(long),   P_LONG_ALIGN,   P_LONG_ORDER,
                sizeof(float),  P_FLOAT_ALIGN,  P_FLOAT_ORDER,
                sizeof(double), P_DOUBLE_ALIGN, P_DOUBLE_ORDER,
                P_FLOAT_LAYOUT, P_DOUBLE_LAYOUT}}
  };
  const int ndb = sizeof(db)/sizeof(db[0]);
  if (argc != 1) YError("get_encoding takes exactly one argument");
  const char* name = YGetString(sp);
  if (name != NULL) {
    long* result = YETI_PUSH_NEW_L(yeti_start_dimlist(32));
    int c = name[0];
    for (int i = 0; i < ndb; ++i) {
      if (db[i].name[0] == c && strcmp(db[i].name, name) == 0) {
        long* layout = db[i].layout;
        for (int j = 0; j < 32; ++j) {
          result[j] = layout[j];
        }
        return;
      }
    }
  }
  YError("unknown encoding name");
}

/*---------------------------------------------------------------------------*/
/* MACHINE DEPENDENT CONSTANTS */

void Y_machine_constant(int argc)
{
  if (argc!=1) YError("machine_constant: takes exactly one argument");
  const char* name = YGetString(sp);
  double dval;
  float fval;
  long lval;
  if (name != NULL && name[0] == 'D' && name[1] == 'B' && name[2] == 'L'
      && name[3] == '_') {
#define _(S,V) do {                             \
    if (strcmp(#S, name + 4) == 0) {            \
      V = DBL_##S;                              \
      goto push_##V;                            \
    }                                           \
  } while (0)
#if defined(DBL_EPSILON)
    _(EPSILON, dval);
#endif
#if defined(DBL_MIN)
    _(MIN, dval);
#endif
#if defined(DBL_MAX)
    _(MAX, dval);
#endif
#if defined(DBL_MIN_EXP)
    _(MIN_EXP, lval);
#endif
#if defined(DBL_MAX_EXP)
    _(MAX_EXP, lval);
#endif
#if defined(DBL_MIN_10_EXP)
    _(MIN_10_EXP, lval);
#endif
#if defined(DBL_MAX_10_EXP)
    _(MAX_10_EXP, lval);
#endif
#if defined(DBL_MANT_DIG)
    _(MANT_DIG, lval);
#endif
#if defined(DBL_DIG)
    _(DIG, lval);
#endif
#undef _
  } else if (name != NULL && name[0] == 'F' && name[1] == 'L' && name[2] == 'T'
             && name[3] == '_') {
#define _(S,V) do {                             \
    if (strcmp(#S, name + 4) == 0) {            \
      V = FLT_##S;                              \
      goto push_##V;                            \
    }                                           \
  } while (0)
#if defined(FLT_EPSILON)
    _(EPSILON, fval);
#endif
#if defined(FLT_MIN)
    _(MIN, fval);
#endif
#if defined(FLT_MAX)
    _(MAX, fval);
#endif
#if defined(FLT_MIN_EXP)
    _(MIN_EXP, lval);
#endif
#if defined(FLT_MAX_EXP)
    _(MAX_EXP, lval);
#endif
#if defined(FLT_MIN_10_EXP)
    _(MIN_10_EXP, lval);
#endif
#if defined(FLT_MAX_10_EXP)
    _(MAX_10_EXP, lval);
#endif
#if defined(FLT_RADIX)
    _(RADIX, lval);
#endif
#if defined(FLT_MANT_DIG)
    _(MANT_DIG, lval);
#endif
#if defined(FLT_DIG)
    _(DIG, lval);
#endif
#undef _
  }
  YError("unknown name of machine constant");
  return;

 push_dval:
  PushDoubleValue(dval);
  return;
 push_fval:
  *YETI_PUSH_NEW_F(NULL) = fval;
  return;
 push_lval:
  PushLongValue(lval);
  return;
}

/*---------------------------------------------------------------------------*/
/* SYMBOLS */

void Y_nrefsof(int argc)
{
  if (argc != 1) YError("nrefsof takes exactly one argument");
  if (! sp->ops) YError("unexpected keyword argument");
  Operand op;
  PushLongValue(sp->ops->FormOperand(sp, &op)->references);
}

void Y_insure_temporary(int argc)
{
  if (argc < 1 || ! CalledAsSubroutine()) {
    YError("insure_temporary must be called as a subroutine");
  }
  for (int i = 1 - argc; i <= 0; ++i) {
    Symbol* stack = sp + i;
    if (stack->ops != &referenceSym) {
      YError("insure_temporary expects variable reference(s)");
    }
    Symbol* glob = &globTab[stack->index];
    OpTable* ops = glob->ops;
    if (ops == &doubleScalar) {
      Array* copy = NewArray(&doubleStruct, (Dimension*)0);
      copy->value.d[0] = glob->value.d;
      glob->value.db = (DataBlock*)copy;
      glob->ops = &dataBlockSym;
    } else if (ops == &longScalar) {
      Array* copy = NewArray(&longStruct, (Dimension*)0);
      copy->value.l[0] = glob->value.l;
      glob->value.db = (DataBlock*)copy;
      glob->ops = &dataBlockSym;
    } else if (ops == &intScalar) {
      Array* copy = NewArray(&intStruct, (Dimension*)0);
      copy->value.i[0] = glob->value.i;
      glob->value.db = (DataBlock*)copy;
      glob->ops = &dataBlockSym;
    } else if (ops == &dataBlockSym) {
      Array* array = (Array*)glob->value.db;
      if (array->references >= 1 && array->ops->isArray) {
        /* make a fresh copy */
        Array* copy = NewArray(array->type.base, array->type.dims);
        glob->value.db = (DataBlock*)copy;
        --array->references;
        array->type.base->Copy(array->type.base, copy->value.c,
                               array->value.c, array->type.number);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
/* SMOOTHING */

static void smooth_single(double* x, double p25, double p50, double p75,
                          long n1, long n2, long n3);

void Y_smooth3(int argc)
{
  Operand op;
  double* x = NULL;
  long n1, n2, n3;
  int single = 0, is_complex;
  long which = 0; /* avoid compiler warning */
  Symbol* stack;
  Dimension* dims;
  int nparsed = 0;
  double p25 = 0.25, p50 = 0.50, p75 = 0.75;

  for (stack=sp-argc+1 ; stack<=sp ; ++stack) {
    if (stack->ops) {
      /* non-keyword argument */
      if (++nparsed == 1) {
        stack->ops->FormOperand(stack, &op);
      } else {
        YError("too many arguments");
      }
    } else {
      /* keyword argument */
      const char* keyword = globalTable.names[stack->index];
      ++stack;
      if (keyword[0] == 'c' && keyword[1] == 0) {
        if (YNotNil(stack)) {
          p50 = YGetReal(stack);
          p25 = 0.5*(1.0 - p50);
          p75 = 0.5*(1.0 + p50);
        }
      } else if (keyword[0] == 'w' && ! strcmp(keyword, "which")) {
        if (YNotNil(stack)) {
          which = YGetInteger(stack);
          single = 1;
        }
      } else {
        YError("unknown keyword");
      }
    }
  }
  if (nparsed != 1) YError("bad number of arguments");

  /* Get input array. */
  is_complex = (op.ops->typeID == T_COMPLEX);
  n1 = (is_complex ? 2*op.type.number : op.type.number);
  stack = op.owner;
  switch (op.ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
  case T_LONG:
  case T_FLOAT:
    /* Convert input in a new array of double's. */
    op.ops->ToDouble(&op);
    x = op.value;
    dims = op.type.dims;
    break;

  case T_DOUBLE:
  case T_COMPLEX:
    /* If input array has references (is not temporary), make a new copy. */
    if (op.references) {
      Array* array = NewArray((is_complex ? &complexStruct : &doubleStruct),
                              op.type.dims);
      PushDataBlock(array);
      x = array->value.d;
      dims = array->type.dims;
      memcpy(x, op.value, n1*sizeof(double));
      PopTo(stack);
    } else {
      x = op.value;
      dims = op.type.dims;
    }
    break;

  default:
    YError("bad data type for input array");
  }
  while (sp != stack) Drop(1);  /* left result on top of the stack */

  /* Apply operator. */
  n3 = 1; /* product of dimensions after current one */
  if (single) {
    /* Apply operator along a single dimension. */
    Dimension* tmp = dims;
    long rank=0;
    while (tmp) {
      ++rank;
      tmp = tmp->next;
    }
    if (which <= 0) which += rank;
    if (which <= 0 || which > rank) YError("WHICH is out of range");
    while (dims) {
      n2 = dims->number;
      n1 /= n2;
      if (rank-- == which) {
        smooth_single(x, p25, p50, p75, n1, n2, n3);
        break;
      }
      n3 *= n2;
      dims = dims->next;
    }
  } else {
    /* Apply operator to every dimensions. */
    while (dims) {
      n2 = dims->number;
      n1 /= n2;
      smooth_single(x, p25, p50, p75, n1, n2, n3);
      n3 *= n2;
      dims = dims->next;
    }
  }
}

static void smooth_single(double* x, double p25, double p50, double p75,
                          long n1, long n2, long n3)
{
  if (n2 >= 2) {
    long i, stride = n1, n = n1*n2;
    double x1, x2, x3;
    if (stride == 1) {
      for ( ; --n3 >= 0; x+=n) {
        x2 = x[0];
        x3 = x[1];
        x[0] = p75*x2 + p25*x3;
        for (i = 2; i < n; ++i) {
          x1 = x2;
          x2 = x3;
          x3 = x[i];
          x[i - 1] = p50*x2 + p25*(x1 + x3);
        }
        x[n - 1] = p75*x3 + p25*x2;
      }
    } else {
      long p = n - stride;
      for ( ; --n3 >= 0; x += p) {
        for (n1 = stride; --n1 >= 0; ++x) {
          x2 = x[0];
          x3 = x[stride];
          x[0] = p75*x2 + p25*x3;
          for (i = 2*stride; i < n; i += stride) {
            x1 = x2;
            x2 = x3;
            x3 = x[i];
            x[i - stride] = p50*x2 + p25*(x1 + x3);
          }
          x[n - stride] = p75*x3 + p25*x2;
        }
      }
    }
  }
}
