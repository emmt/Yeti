/*
 * yeti.h -
 *
 * Definitions for writing Yorick extensions.
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

#ifndef _YETI_H
#define _YETI_H 1

#include <stdlib.h>
#include "ydata.h"
#include "defmem.h"

/*---------------------------------------------------------------------------*/

/* In recent (>=1.5.12) versions of Yorick, RefNC in defined in binio.h
   which is included by ydata.h. */
#ifndef RefNC
# define RefNC(db) (++(db)->references , (db))
#endif

/*---------------------------------------------------------------------------*/
/* USEFUL MACROS */

#define YETI_ROUND_UP(a,b) ((((a)+(b)-1)/(b))*(b))
/*----- Return smallest multiple of integer B that is greater or equal
        integer A. */

/*
 * Utility macros: YETI_STRINGIFY takes an argument and wraps it in "" (double
 * quotation marks), YETI_JOIN joins two arguments.  Both are capable of
 * performing macro expansion of their arguments.
 */
#define YETI_VERBATIM(x) x
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
# define YETI_STRINGIFY(x)  YETI_STRINGIFY1(x)
# define YETI_STRINGIFY1(x) # x
# define YETI_JOIN(a,b)     YETI_JOIN1(a, b)
# define YETI_JOIN1(a,b)    a ## b
#else
# define YETI_STRINGIFY(x)  "x"
# define YETI_JOIN(a,b)     YETI_VERBATIM(a)/**/YETI_VERBATIM(b)
#endif

/*---------------------------------------------------------------------------*/
/* Macro to get rid of some GCC extensions when not compiling with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#  undef __attribute__
#  define __attribute__(x) /* empty */
#endif

/*---------------------------------------------------------------------------*/
/* C++ needs to know that types and declarations are C, not C++.  */
#ifdef  __cplusplus
# define _YETI_BEGIN_DECLS  extern "C" {
# define _YETI_END_DECLS    }
#else
# define _YETI_BEGIN_DECLS
# define _YETI_END_DECLS
#endif

_YETI_BEGIN_DECLS
/*---------------------------------------------------------------------------*/

/* Refine definition of YError to avoid GCC warnings (about uninitialized
   variables or reaching end of non-void function): */
PLUG_API void YError(const char *msg) __attribute__ ((noreturn));

extern void yeti_error(const char *str, ...) __attribute__ ((noreturn));
/*----- Build error message from a list of strings (last element of the
        list must be NULL) and call YError.  The maximum length of the
        final message is 127 characters; otherwise the message get,
        silently truncated. */

/* The following routines/macros are designed to simply the handling of
   Yorick's symbols. */

#define YETI_IS_REF(S)       ((S)->ops == &referenceSym)
#define YETI_DEREF_SYMBOL(S) (YETI_IS_REF(S) ? &globTab[(S)->index] : (S))
/*----- Return S or the referenced symbol if S is a reference. */

#define YETI_SOLVE_REF(S) if (YETI_IS_REF(S)) (S) = &globTab[(S)->index]
/*----- Solve Yorick's symbol reference(s).  This macro is intended to be
        used for symbols that the parser push on the stack as arguments of
        a built-in routine.  The argument of the macro should be a variable.
        Also beware that the macro could break an if-else statement if not
        enclosed into braces.  Use ReplaceRef(S) to replace the stack
        symbol S by whatever it point to. */

extern char *yeti_strcpy(const char *str);
extern char *yeti_strncpy(const char *str, size_t len);
/*----- Return a copy of string STR. If STR is NULL, NULL is returned;
        otherwise LEN+1 bytes get dynamically allocated for the copy.  The
        return value is intended to be managed as an element of a Yorick's
        string array, i.e. the copy must be deleted by StrFree for Yorick
        version 1.4 and p_free for newer Yorick versions. */

extern int yeti_is_range(Symbol *s);
extern int yeti_is_structdef(Symbol *s);
extern int yeti_is_stream(Symbol *s);
extern int yeti_is_nil(Symbol *s);
extern int yeti_is_void(Symbol *s);
/*----- Check various properties of symbol *S.  Note 1: if S is a
        reference, the test is performed onto the referenced object but S
        does not get replaced by the referenced object (e.g. not as with
        YNotNil), so S can be outside the stack.  Note 2: yeti_is_nil and
        yeti_is_void should return the same result but, in case this
        matters, yeti_is_nil checks the datablock address while
        yeti_is_void checks the datablock Operations address. */

extern void yeti_debug_symbol(Symbol *s);
/*-----	Print-out contents of symbol *S. */

extern void yeti_bad_argument(Symbol *s) __attribute__ ((noreturn));
/*----- Trigger an error (by calling YError) due to a bad built-in routine
        argument *SYM.  The first reference level is assumed to have been
        resolved (see for instance YETI_SOLVE_REF). */

extern void yeti_unknown_keyword(void) __attribute__ ((noreturn));
/*-----	Call YError with the message:
          "unrecognized keyword in builtin function call". */

extern DataBlock *yeti_get_datablock(Symbol *s, const Operations *ops);
/*----- Get data block from symbol S.  If OPS is non-NULL, the
        virtual function table (Operations) of the data block symbol must
        match OPS.  If S is a reference the referenced symbol is considered
        instead of S and get popped onto the stack to replace S (as with
        ReplaceRef); this is required to avoid returning a temporary
        data block that could be unreferenced elsewhere. */

extern Array *yeti_get_array(Symbol *s, int nil_ok);
/*----- Get array from symbol S.  If S is a reference the referenced symbol
        is considered instead of S and get popped onto the stack to replace
        S (as with ReplaceRef); this is required to avoid returning a
        temporary array that could be unreferenced elsewhere.  If the
        considered object, is void, then if NIL_OK is non-zero NULL is
        returned; otherwise, YError is called and the function does not
        returns. */

extern int yeti_get_boolean(Symbol *s);
/*-----	Return 1/0 according to the value of symbol S.  The result should
        be the same as the statement (s?1:0) in Yorick. */

extern long yeti_get_optional_integer(Symbol *s, long defaultValue);
/*-----	Return the value of symbol `*s' if it is a scalar (or 1 element array)
        integer (char, short, int or long) and `defaultValue' is symbol is
        void.  Call yeti_bad_argument otherwise. */

typedef struct yeti_scalar yeti_scalar_t;
extern yeti_scalar_t *yeti_get_scalar(Symbol *s, yeti_scalar_t *scalar);
/*-----	Fetch scalar value stored in Yorick symbol S and fill SCALAR
        accordingly.  The return value is SCALAR. */

struct yeti_scalar {
  int type; /* One of: T_CHAR, T_SHORT, T_INT, T_LONG, T_FLOAT, T_DOUBLE,
                       T_COMPLEX, T_POINTER, T_STRUCT, T_RANGE, T_VOID,
                       T_FUNCTION, T_BUILTIN, T_STRUCTDEF, T_STREAM, T_OPAQUE.
               Never:  T_LVALUE. */
  union {
    char   c;
    int    i;
    short  s;
    long   l;
    float  f;
    double d;
    struct {double re, im;} z;
    char  *q;
    void  *p;
  } value;
};

/*---------------------------------------------------------------------------*/
/* STACK FUNCTIONS/MACROS */

extern void yeti_pop_and_reduce_to(Symbol *s);
/*----- Pop topmost stack element in-place of S and drop all elements above
        S.  S must belong to the stack.  This routine is useful to pop the
        result of a builtin routine.  The call is equivalent to: PopTo(S);
        Drop(N); with N = sp - S (sp taken before PopTo). */


#define       yeti_get_integer(SYMBOL)  YGetInteger(SYMBOL)
#define       yeti_get_real(SYMBOL)     YGetReal(SYMBOL)
#define       yeti_get_string(SYMBOL)   YGetString(SYMBOL)
extern void **yeti_get_pointer(Symbol *s);
/*----- Funtions to get a scalar value from Yorick stack element S. */


extern void yeti_push_char_value(int value);
extern void yeti_push_short_value(int value);
extern void yeti_push_float_value(double value);
extern void yeti_push_complex_value(double re, double im);
extern void yeti_push_string_value(const char *value);
#define     yeti_push_int_value(value)    PushIntValue(value)
#define     yeti_push_long_value(value)   PushLongValue(value)
#define     yeti_push_double_value(value) PushDoubleValue(value)
/*----- These functions push a new scalar value of a particular type on top
        of the stack.  String VALUE can be NULL. */

#define YETI_PUSH_NEW_ARRAY(SDEF, DIMS) ((Array *)PushDataBlock(NewArray(SDEF, DIMS)))
#define YETI_PUSH_NEW_ARRAY_C(DIMS) YETI_PUSH_NEW_ARRAY(&charStruct,    DIMS)
#define YETI_PUSH_NEW_ARRAY_S(DIMS) YETI_PUSH_NEW_ARRAY(&shortStruct,   DIMS)
#define YETI_PUSH_NEW_ARRAY_I(DIMS) YETI_PUSH_NEW_ARRAY(&intStruct,     DIMS)
#define YETI_PUSH_NEW_ARRAY_L(DIMS) YETI_PUSH_NEW_ARRAY(&longStruct,    DIMS)
#define YETI_PUSH_NEW_ARRAY_F(DIMS) YETI_PUSH_NEW_ARRAY(&floatStruct,   DIMS)
#define YETI_PUSH_NEW_ARRAY_D(DIMS) YETI_PUSH_NEW_ARRAY(&doubleStruct,  DIMS)
#define YETI_PUSH_NEW_ARRAY_Z(DIMS) YETI_PUSH_NEW_ARRAY(&complexStruct, DIMS)
#define YETI_PUSH_NEW_ARRAY_Q(DIMS) YETI_PUSH_NEW_ARRAY(&stringStruct,  DIMS)
#define YETI_PUSH_NEW_ARRAY_P(DIMS) YETI_PUSH_NEW_ARRAY(&pointerStruct, DIMS)
/*----- These macros allocate a  new Yorick array with dimension list DIMS,
        push it  on top of  the stack and  return the address of  the array
        structure.  There  must be an element  left on top of  the stack to
        store the new array.  See also: YETI_PUSH_NEW_. */


#define YETI_PUSH_NEW_(SDEF, DIMS, MEMBER) (YETI_PUSH_NEW_ARRAY(SDEF, DIMS)->value.MEMBER)
#define YETI_PUSH_NEW_C(DIMS) YETI_PUSH_NEW_(&charStruct,    DIMS, c)
#define YETI_PUSH_NEW_S(DIMS) YETI_PUSH_NEW_(&shortStruct,   DIMS, s)
#define YETI_PUSH_NEW_I(DIMS) YETI_PUSH_NEW_(&intStruct,     DIMS, i)
#define YETI_PUSH_NEW_L(DIMS) YETI_PUSH_NEW_(&longStruct,    DIMS, l)
#define YETI_PUSH_NEW_F(DIMS) YETI_PUSH_NEW_(&floatStruct,   DIMS, f)
#define YETI_PUSH_NEW_D(DIMS) YETI_PUSH_NEW_(&doubleStruct,  DIMS, d)
#define YETI_PUSH_NEW_Z(DIMS) YETI_PUSH_NEW_(&complexStruct, DIMS, d)
#define YETI_PUSH_NEW_Q(DIMS) YETI_PUSH_NEW_(&stringStruct,  DIMS, q)
#define YETI_PUSH_NEW_P(DIMS) YETI_PUSH_NEW_(&pointerStruct, DIMS, p)
/*----- These macros allocate a  new Yorick array with dimension list DIMS,
        push it  on top  of the stack  and return  the base address  of the
        array  contents.   See  YETI_PUSH_NEW_ARRAY  for side  effects  and
        restrictions. */

/*---------------------------------------------------------------------------*/
/* DIMENSIONS OF ARRAYS. */

PLUG_API Dimension *tmpDims;
/*----- tmpDims is a global temporary for Dimension lists under
        construction -- you should always use it, then just leave your
        garbage there when you are done for the next guy to clean up --
        your part of the perpetual cleanup comes first. */

extern void yeti_reset_dimlist(void);
/*----- Prepares global variable tmpDims for the building of a new
        dimension list (i.e. takes care of freeing old dimension
        list if any). */

extern Dimension *yeti_grow_dimlist(long number);
/*----- Appends dimension of length NUMBER to tmpDims and returns new
        value of tmpDims.  Note that tmpDims must have been properly
        initialized by yeti_reset_dimlist or yeti_start_dimlist (which
        to see).  For instance to build the dimension list of a NCOLS
        by NROWS array, do:
          yeti_reset_dimlist();
          yeti_grow_dimlist(ncols);
          yeti_grow_dimlist(nrows); */

extern Dimension *yeti_start_dimlist(long number);
/*----- Initializes global temporary tmpDims with a single first
        dimension with length NUMBER and returns tmpDims.  Same as:
           yeti_reset_dimlist();
           yeti_grow_dimlist(number); */

extern Dimension *yeti_make_dims(const long number[], const long origin[],
                                 size_t ndims);
/*----- Build and store a dimension list in tmpDims, NDIMS is the number of
        dimensions, NUMBER[i] and ORIGIN[i] are the length and starting
        index along the i-th dimension (if ORIGIN is NULL, all origins are
        set to 1).  The new value of tmpDims is returned. */

extern size_t yeti_get_dims(const Dimension *dims, long number[],
                            long origin[], size_t maxdims);
/*----- Store dimensions along chained list DIMS in arrays NUMBER and
        ORIGIN (if ORIGIN is NULL, it is not used).  There must be no more
        than MAXDIMS dimensions.  Returns the number of dimensions. */

extern int yeti_same_dims(const Dimension *dims1, const Dimension *dims2);
/*----- Check that two dimension lists are identical (also see Conform in
        ydata.c or yeti_total_number_2).  Returns 1 (true) or 0 (false). */

extern void yeti_assert_same_dims(const Dimension *dims1,
                                  const Dimension *dims2);
/*-----	Assert that two dimension lists are identical, raise an error (see
        YError) if this not the case. */

extern long yeti_total_number(const Dimension *dims);
/*----- Returns number of elements of the array with dimension list DIMS. */

extern long yeti_total_number_2(const Dimension *dims1,
                                const Dimension *dims2);
/*----- Check that two dimension lists are identical and return the number
        of elements of the corresponding array.  An error is raised (via
        YError) if dimension lists are not identical. */

/*---------------------------------------------------------------------------*/
/* WORKSPACE */

extern void *yeti_push_workspace(size_t nbytes);
/*----- Return temporary worspace of NBYTES bytes.  In case of error
        (insufficient memory), YError get called; the routine therefore
        always returns a valid address.  An opaque workspace object get
        pushed onto Yorick's stack so that it is automatically deleted but
        the caller has to make sure that the stack is large enough (see
        CheckStack). */

/*---------------------------------------------------------------------------*/
/* OPAQUE OBJECTS */
/* Very simple implementation of "opaque" objects in Yorick.  The main
   purpose is to let Yorick handle object reference counts and free object
   resources as soon as the object is no longer referenced.  The main lack is
   that no operator overloading is providing (although it is possible in
   principle but necessitates much more code...) */

typedef struct yeti_opaque        yeti_opaque_t;
typedef struct yeti_opaque_class  yeti_opaque_class_t;

struct yeti_opaque {
  /* The yeti_opaque_t structure stores information about an instance of an
     object.  The two first members (references and ops) are common to any
     Yorick's DataBlock. */
  int references;                   /* reference counter */
  Operations *ops;                  /* virtual function table */
  const yeti_opaque_class_t *class; /* opaque class definition */
  void *data;                       /* opaque object client data. */
};

struct yeti_opaque_class {
  /* This structure provides the object class definition (class name and
     methods).  For each class there must be a unique such structure so that
     its address can be used as an identifier to identify the object class. */
  const char *name;       /* Object class name. */
  void (*delete)(void *); /* Method called when object is being deleted (the
                             argument is the object client data).  If the
                             "delete" method is NULL, nothing particular is
                             done when the object is deleted. */
  void (*print)(void *);  /* Method used to print object information (the
                             argument is the object client data).  If the
                             "print" method is NULL, a default one is
                             supplied. */
};

extern yeti_opaque_t *yeti_new_opaque(void *data,
                                      const yeti_opaque_class_t *class);
/*----- Create a new Yorick data block to store an instance of an opaque
        object.  Since Yeti's implementation of opaque objects does not keep
        track of object client data references, a single client data
        instance cannot be referenced by several object data blocks (unless
        the "delete" method provided by the class takes care of that); it
        is nevertheless still possible that several Yorick's symbols share
        the same data block. */

extern yeti_opaque_t *yeti_get_opaque(Symbol *stack,
                                      const yeti_opaque_class_t *class,
                                      int fatal);
/*----- Returns a pointer to the object referenced by Yorick's symbol STACK.
        If CLASS is non-NULL the object must be of that class.  If FATAL is
        non-zero, any error result in calling YError (i.e. the routine
        never return on error); otherwise, NULL is returned on error.  Note:
        STACK must belong to the stack and, if it is a reference, it gets
        replaced by the referenced object (as by calling ReplaceRef); this
        is needed to avoid using a temporary object that may be unreferenced
        elsewhere. */

#define yeti_get_opaque_data(STACK, CLASS) \
                            (yeti_get_opaque(STACK, CLASS, 1)->data)
/*----- Get the client data of the stack opaque object referenced by symbol
        *STACK which must be a Yeti object type and, if CLASS is not NULL,
        must be an instance of this class.  An error is issued if symbol
        *STACK does not reference an opaque object or if CLASS is not NULL
        and is not that of the object.  See yeti_get_opaque for side
        effects. */

/*---------------------------------------------------------------------------*/
_YETI_END_DECLS
#endif /* _YETI_H */
