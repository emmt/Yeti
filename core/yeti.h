/*
 * yeti.h -
 *
 * Definitions for writing Yorick extensions.
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

#ifndef _YETI_H
#define _YETI_H 1

/* Yeti requires a descent C compiler.  C11 is the deafult for GCC since a long
   time. */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
# error Yeti requires a C11 compiler
#endif

#include <stdlib.h>
#include "ydata.h"
#include "defmem.h"

/*---------------------------------------------------------------------------*/

/* In recent (>=1.5.12) versions of Yorick, RefNC in defined in binio.h
   which is included by ydata.h. */
#ifndef RefNC
# define RefNC(db) (++(db)->references , (db))
#endif

/**
 * @def YOR_UNSAFE_UNREF(db)
 *
 * Unreference data-block @ db assuming @a db is non-`NULL`.
 */
#define YOR_UNSAFE_UNREF(db) do {               \
    DataBlock* __db = (db);                     \
    if (--__db->references < 0) {               \
      __db->ops->Free(__db);                    \
    }                                           \
  } while (0)

/**
 * @def YOR_UNREF(db)
 *
 * Unreference data-block @ db if non-`NULL`.
 */
#define YOR_UNREF(db) do {                              \
    DataBlock* __db = (db);                             \
    if (__db != NULL && --__db->references < 0) {        \
      __db->ops->Free(__db);                            \
    }                                                   \
  } while (0)

/*---------------------------------------------------------------------------*/
/* USEFUL MACROS */

#define YOR_OFFSET_OF(T, m) ((char*)&((T*)0)->m - (char*)0)
/*----- Yield offset of member M in structure of type T. */

#define YOR_ROUND_UP(a,b) ((((a)+(b)-1)/(b))*(b))
/*----- Return smallest multiple of integer B that is greater or equal
        integer A. */

#define YOR_VERBATIM(x) x
/*----- Yield argument without macro expansion. */

#define YOR_XSTRINGIFY(x)  YOR_STRINGIFY(x)
/*----- Wrap argument in quotes with macro expansion. */

#define YOR_STRINGIFY(x) # x
/*----- Wrap argument in quotes without macro expansion. */

#define YOR_XJOIN(a,b)   YOR_JOIN(a, b)
/*----- Join arguments A and B with macro expansion. */

#define YOR_JOIN(a,b)    a ## b
/*----- Join arguments A and B without macro expansion. */

/*---------------------------------------------------------------------------*/
/* Macro to get rid of some GCC extensions when not compiling with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#  undef __attribute__
#  define __attribute__(x) /* empty */
#endif

/*---------------------------------------------------------------------------*/
/* C++ needs to know that types and declarations are C, not C++.  */
#ifdef  __cplusplus
# define _YOR_BEGIN_DECLS  extern "C" {
# define _YOR_END_DECLS    }
#else
# define _YOR_BEGIN_DECLS
# define _YOR_END_DECLS
#endif

_YOR_BEGIN_DECLS

/*---------------------------------------------------------------------------*/
/* ERROR MESSAGES */

/* Refine definition of YError to avoid GCC warnings (about uninitialized
   variables or reaching end of non-void function): */
PLUG_API void YError(const char* msg) __attribute__ ((noreturn));

#define yor_error YError

extern void yor_unexpected_keyword_argument() __attribute__ ((noreturn));

extern void yor_bad_argument(Symbol* s) __attribute__ ((noreturn));
/*----- Trigger an error (by calling yor_error) due to a bad built-in routine
        argument *SYM.  The first reference level is assumed to have been
        resolved (see for instance YETI_SOLVE_REFERENCE). */

extern void yor_unknown_keyword(void) __attribute__ ((noreturn));
/*----- Call yor_error with the message:
        "unrecognized keyword in builtin function call". */

extern void yor_format_error(const char* str, ...) __attribute__ ((noreturn));
/*----- Build error message from a list of strings (last element of the
        list must be NULL) and call yor_error.  The maximum length of the
        final message is 127 characters; otherwise the message get,
        silently truncated. */


/*---------------------------------------------------------------------------*/
/* YORICK BASIC TYPES */

#define YOR_CHAR       T_CHAR
#define YOR_SHORT      T_SHORT
#define YOR_INT        T_INT
#define YOR_LONG       T_LONG
#define YOR_FLOAT      T_FLOAT
#define YOR_DOUBLE     T_DOUBLE
#define YOR_COMPLEX    T_COMPLEX
#define YOR_STRING     T_STRING
#define YOR_POINTER    T_POINTER
#define YOR_STRUCT     T_STRUCT
#define YOR_RANGE      T_RANGE
#define YOR_LVALUE     T_LVALUE
#define YOR_VOID       T_VOID
#define YOR_FUNCTION   T_FUNCTION
#define YOR_BUILTIN    T_BUILTIN
#define YOR_STRUCTDEF  T_STRUCTDEF
#define YOR_STREAM     T_STREAM
#define YOR_OPAQUE     T_OPAQUE

/**
 * @def YOR_TYPEID(T)
 *
 * Yield the Yorick type identifier corresponding to C type T which must be a
 * basic array element type (numerical or string).
 */
#define YOR_TYPEID(T)                           \
  _Generic((T){0},                              \
           char:          YOR_CHAR,             \
           short:         YOR_SHORT,            \
           int:           YOR_INT,              \
           long:          YOR_LONG,             \
           float:         YOR_FLOAT,            \
           double:        YOR_DOUBLE,           \
           yor_complex_t: YOR_COMPLEX,          \
           yor_string_t:  YOR_STRING)
/* FIXME: Having YOR_POINTER mapped to `yor_pointer_t` would be unsafe
   considering the way pointer objects are managed by Yorick. */

/**
 * Yorick complex type.
 *
 * A Yorick complex is just a pair of `double` (the real part followed by the
 * imaginary part).  To create a complex value, use one of the following:
 *
 * ```.c
 * (yor_complex_t){ real_part, im_part }
 * (yor_complex_t){ .re = real_part, .im = im_part }
 * _YOR_COMPLEX(real_part, im_part)
 * ```
 *
 * where `real_part` and `imag_part` are expressions (they are evaluated once
 * by the macro @ref YOR_COMPLEX).
 *
 * If the members names are specified, real and imaginary parts can be given in
 * any order, and if one is omitted, its value is assumed to be zero.
 */
typedef struct _yor_complex { double re, im; } yor_complex_t;

/**
 * @def _YOR_COMPLEX(r,i)
 *
 * Yield a Yorick complex value whose real and imaginary parts are
 * the result of the expressions `r` and `i` respectively.
 */
#define _YOR_COMPLEX(r,i) ((yor_complex_t){ .re = (r), .im = (i) })

/**
 * Yorick string type.
 *
 * All Yorick strings are dynamically allocated (with `p_malloc`) or `NULL`.
 * Yorick calls `p_free` on a string when no longer needed.  There is no
 * reference counter.  this means that strings must be systematically copied.
 */
typedef char* yor_string_t;

/**
 * Yorick pointer type.
 *
 * Beware that Yorick manage pointer onjects in a very special way.
 */
typedef void* yor_pointer_t;


/*---------------------------------------------------------------------------*/

/* The following routines/macros are designed to simplify the handling of
   Yorick's symbols. */

#define YETI_IS_REFERENCE(S)    ((S)->ops == &referenceSym)
#define YETI_DEREFERENCE_SYMBOL(S) \
  (YETI_IS_REFERENCE(S) ? &globTab[(S)->index] : (S))
/*----- Return S or the referenced symbol if S is a reference. */

#define YETI_SOLVE_REFERENCE(S) do {            \
    if (YETI_IS_REFERENCE(S)) {                 \
      (S) = &globTab[(S)->index];               \
    }                                           \
  } while (0)
/*----- Solve Yorick's symbol reference(s).  This macro is intended to be used
        for symbols that the parser push on the stack as arguments of a
        built-in routine.  The argument of the macro should be an L-value
        (e.g., a variable).  Use ReplaceRef(S) to replace the stack symbol S by
        whatever it points to.  Note that there are no multiple levels of
        references in Yorick (as guessed from inspecting Yorick code). */

extern char* yor_strcpy(const char* str);
extern char* yor_strncpy(const char* str, size_t len);
/*----- Return a copy of string STR. If STR is NULL, NULL is returned;
        otherwise LEN+1 bytes get dynamically allocated for the copy.  The
        return value is intended to be managed as an element of a Yorick's
        string array, i.e. the copy must be deleted by StrFree for Yorick
        version 1.4 and p_free for newer Yorick versions. */

extern int yor_is_range(Symbol* s);
extern int yor_is_structdef(Symbol* s);
extern int yor_is_stream(Symbol* s);
extern int yor_is_nil(Symbol* s);
extern int yor_is_void(Symbol* s);
/*----- Check various properties of symbol *S.  Note 1: if S is a
        reference, the test is performed onto the referenced object but S
        does not get replaced by the referenced object (e.g. not as with
        YNotNil), so S can be outside the stack.  Note 2: yor_is_nil and
        yor_is_void should return the same result but, in case this
        matters, yor_is_nil checks the datablock address while
        yor_is_void checks the datablock Operations address. */

extern void yor_debug_symbol(Symbol* s);
/*----- Print-out contents of symbol *S. */

extern DataBlock* yor_get_datablock(Symbol* s, const Operations* ops);
/*----- Get data block from symbol S.  If OPS is non-NULL, the
        virtual function table (Operations) of the data block symbol must
        match OPS.  If S is a reference the referenced symbol is considered
        instead of S and get popped onto the stack to replace S (as with
        ReplaceRef); this is required to avoid returning a temporary
        data block that could be unreferenced elsewhere. */

extern Array* yor_get_array(Symbol* s, int nil_ok);
/*----- Get array from symbol S.  If S is a reference the referenced symbol
        is considered instead of S and get popped onto the stack to replace
        S (as with ReplaceRef); this is required to avoid returning a
        temporary array that could be unreferenced elsewhere.  If the
        considered object, is void, then if NIL_OK is non-zero NULL is
        returned; otherwise, yor_error is called and the function does not
        returns. */

extern int yor_get_boolean(Symbol* s);
/*----- Return 1/0 according to the value of symbol S.  The result should
        be the same as the statement (s?1:0) in Yorick. */

extern long yor_get_optional_integer(Symbol* s, long defaultValue);
/*----- Return the value of symbol `*s' if it is a scalar (or 1 element array)
        integer (char, short, int or long) and `defaultValue' is symbol is
        void.  Call yor_bad_argument otherwise. */

typedef struct _yor_scalar yor_scalar_t;

extern yor_scalar_t* yor_get_scalar(Symbol* s, yor_scalar_t* scalar);
/*----- Fetch scalar value stored in Yorick symbol S and fill SCALAR
        accordingly.  The return value is SCALAR. */

struct _yor_scalar {
  int type; // One of: YOR_CHAR, YOR_SHORT, YOR_INT, YOR_LONG, YOR_FLOAT,
            //         YOR_DOUBLE, YOR_COMPLEX, YOR_POINTER, YOR_STRUCT,
            //         YOR_RANGE, YOR_VOID, YOR_FUNCTION, YOR_BUILTIN,
            //         YOR_STRUCTDEF, YOR_STREAM, YOR_OPAQUE.
            // Never:  YOR_LVALUE.
  union {
    char          c;
    int           i;
    short         s;
    long          l;
    float         f;
    double        d;
    yor_complex_t z;
    char*         q;
    void*         p;
  } value;
};

/*---------------------------------------------------------------------------*/
/* STACK FUNCTIONS/MACROS */

extern void yor_pop_and_reduce_to(Symbol* s);
/*----- Pop topmost stack element in-place of S and drop all elements above
        S.  S must belong to the stack.  This routine is useful to pop the
        result of a builtin routine.  The call is equivalent to: PopTo(S);
        Drop(N); with N = sp - S (sp taken before PopTo). */

#define yor_get_integer  YGetInteger
#define yor_get_real     YGetReal
#define yor_get_string   YGetString

extern void** yor_get_pointer(Symbol* s);
/*----- Funtions to get a scalar value from Yorick stack element S. */


extern void yor_push_char_value(char val);
extern void yor_push_short_value(short val);
#define     yor_push_int_value      PushIntValue
#define     yor_push_long_value     PushLongValue
extern void yor_push_float_value(float val);
#define     yor_push_double_value   PushDoubleValue
extern void yor_push_string_value(const char* str);
extern void yor_push_complex_value(yor_complex_t val);
/*----- These functions push a new scalar value of a particular type on top
        of the stack.  String STR can be NULL. */

/**
 * @def yor_push_value(x)
 *
 * Push a scalar value on top of Yorick's stack calling the right function
 * according to the argument type.  For types (like unsigned/signed char,
 * unsigned int, etc.) not directly supported by Yorick, argument must be cast
 * to one of the supported types.
 */
#define yor_push_value(x)                               \
  _Generic(x,                                           \
           char:           yor_push_char_value,         \
           short:          yor_push_short_value,        \
           int:            yor_push_int_value,          \
           long:           yor_push_long_value,         \
           float:          yor_push_float_value,        \
           double:         yor_push_double_value,       \
           yor_complex_t:  yor_push_complex_value,      \
           yor_string_t:   yor_push_string_value)(x)

#define YOR_STRUCT_DEF(T)                       \
  _Generic((T){0},                              \
           char:          charStruct,           \
           short:         shortStruct,          \
           int:           intStruct,            \
           long:          longStruct,           \
           float:         floatStruct,          \
           double:        doubleStruct,         \
           yor_complex_t: complexStruct,        \
           yor_string_t:  stringStruct,         \
           yor_pointer_t: pointerStruct)

#define YOR_PUSH_NEW_ARRAY(T, DIMS)                                      \
  _Generic((T){0},                                                       \
           char:           _YOR_PUSH_NEW_ARRAY(charStruct,    DIMS, c),  \
           short:          _YOR_PUSH_NEW_ARRAY(shortStruct,   DIMS, s),  \
           int:            _YOR_PUSH_NEW_ARRAY(intStruct,     DIMS, i),  \
           long:           _YOR_PUSH_NEW_ARRAY(longStruct,    DIMS, l),  \
           float:          _YOR_PUSH_NEW_ARRAY(floatStruct,   DIMS, f),  \
           double:         _YOR_PUSH_NEW_ARRAY(doubleStruct,  DIMS, d),  \
           yor_complex_t: ((yor_complex_t*)                              \
                           _YOR_PUSH_NEW_ARRAY(complexStruct, DIMS, d)), \
           yor_string_t:   _YOR_PUSH_NEW_ARRAY(stringStruct,  DIMS, q),  \
           yor_pointer_t:  _YOR_PUSH_NEW_ARRAY(pointerStruct, DIMS, p))
/*----- Call this macro to allocate a new Yorick array with dimension list
        DIMS, push it on top of the stack and get the base address of the array
        contents.  See YOR_PUSH_NEW_ARRAY_OBJECT for side effects and
        restrictions. */

// Helper macro for YOR_PUSH_NEW_ARRAY.
#define _YOR_PUSH_NEW_ARRAY(SDEF, DIMS, MEMB) \
  (((Array*)PushDataBlock(NewArray(&(SDEF), DIMS)))->value.MEMB)

#define YOR_NEW_ARRAY_OBJECT(T, DIMS) \
  NewArray(&YOR_STRUCT_DEF(T), DIMS)

#define YOR_PUSH_NEW_ARRAY_OBJECT(T, DIMS) \
  ((Array*)PushDataBlock(YOR_NEW_ARRAY_OBJECT(T, DIMS)))
/*----- Call this macro to allocate a new Yorick array with dimension list
        DIMS, push it on top of the stack and get the address of the array
        structure.  There must be an element left on top of the stack to store
        the new array.  See also: YOR_PUSH_NEW_ARRAY. */


/*---------------------------------------------------------------------------*/
/* DIMENSIONS OF ARRAYS. */

// FIXME: private?
PLUG_API Dimension* tmpDims;
/*----- tmpDims is a global temporary for Dimension lists under
        construction -- you should always use it, then just leave your
        garbage there when you are done for the next guy to clean up --
        your part of the perpetual cleanup comes first. */

extern void yor_reset_dimlist(void);
/*----- Prepares global variable tmpDims for the building of a new
        dimension list (i.e. takes care of freeing old dimension
        list if any). */

extern Dimension* yor_grow_dimlist(long number);
/*----- Appends dimension of length NUMBER to tmpDims and returns new
        value of tmpDims.  Note that tmpDims must have been properly
        initialized by yor_reset_dimlist or yor_start_dimlist (which
        to see).  For instance to build the dimension list of a NCOLS
        by NROWS array, do:
          yor_reset_dimlist();
          yor_grow_dimlist(ncols);
          yor_grow_dimlist(nrows); */

extern Dimension* yor_start_dimlist(long number);
/*----- Initializes global temporary tmpDims with a single first
        dimension with length NUMBER and returns tmpDims.  Same as:
           yor_reset_dimlist();
           yor_grow_dimlist(number); */

extern Dimension* yor_make_dims(const long number[], const long origin[],
                                 size_t ndims);
/*----- Build and store a dimension list in tmpDims, NDIMS is the number of
        dimensions, NUMBER[i] and ORIGIN[i] are the length and starting
        index along the i-th dimension (if ORIGIN is NULL, all origins are
        set to 1).  The new value of tmpDims is returned. */

extern size_t yor_get_dims(const Dimension* dims, long number[],
                           long origin[], size_t maxdims);
/*----- Store dimensions along chained list DIMS in arrays NUMBER and
        ORIGIN (if ORIGIN is NULL, it is not used).  There must be no more
        than MAXDIMS dimensions.  Returns the number of dimensions. */

extern int yor_same_dims(const Dimension* dims1, const Dimension* dims2);
/*----- Check that two dimension lists are identical (also see Conform in
        ydata.c or yor_total_number_2).  Returns 1 (true) or 0 (false). */

extern void yor_assert_same_dims(const Dimension* dims1,
                                  const Dimension* dims2);
/*----- Assert that two dimension lists are identical, raise an error (see
        yor_error) if this not the case. */

extern long yor_total_number(const Dimension* dims);
/*----- Returns number of elements of the array with dimension list DIMS. */

extern long yor_total_number_2(const Dimension* dims1,
                                const Dimension* dims2);
/*----- Check that two dimension lists are identical and return the number
        of elements of the corresponding array.  An error is raised (via
        yor_error) if dimension lists are not identical. */

/*---------------------------------------------------------------------------*/
/* WORKSPACE */

extern void* yor_push_workspace(size_t nbytes);
/*----- Return temporary worspace of NBYTES bytes.  In case of error
        (insufficient memory), yor_error get called; the routine therefore
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
   principle but necessitates much more code...).

   FIXME: Yorick's user objects are more powerful. */

typedef struct yor_opaque      yor_opaque_t;
typedef struct yor_opaque_type yor_opaque_type_t;

struct yor_opaque {
  /* The yor_opaque_t structure stores information about an instance of an
     object.  The two first members (references and ops) are common to any
     Yorick's DataBlock. */
  int                 references; /* reference counter */
  Operations*                ops; /* virtual function table */
  const yor_opaque_type_t*  type; /* opaque type definition */
  void*                     data; /* opaque object client data. */
};

struct yor_opaque_type {
  /* This structure provides the object type definition (type name and
     methods).  For each type there must be a unique such structure so that
     its address can be used as an identifier to identify the object type. */
  const char* name;      /* Object type name. */
  void (*delete)(void*); /* Method called when object is being deleted (the
                            argument is the object client data).  If the
                            "delete" method is NULL, nothing particular is done
                            when the object is deleted. */
  void (*print)(void*);  /* Method used to print object information (the
                            argument is the object client data).  If the
                            "print" method is NULL, a default one is
                            supplied. */
};

extern yor_opaque_t* yor_new_opaque(void* data,
                                     const yor_opaque_type_t* type);
/*----- Create a new Yorick data block to store an instance of an opaque
        object.  Since Yeti's implementation of opaque objects does not keep
        track of object client data references, a single client data instance
        cannot be referenced by several object data blocks (unless the "delete"
        method provided by the type takes care of that); it is nevertheless
        still possible that several Yorick's symbols share the same data
        block. */

extern yor_opaque_t* yor_get_opaque(Symbol* stack,
                                    const yor_opaque_type_t* type,
                                    int fatal);
/*----- Returns a pointer to the object referenced by Yorick's symbol STACK.
        If TYPE is non-NULL the object must be of that type.  If FATAL is
        non-zero, any error result in calling yor_error (i.e. the routine never
        return on error); otherwise, NULL is returned on error.  Note: STACK
        must belong to the stack and, if it is a reference, it gets replaced by
        the referenced object (as by calling ReplaceRef); this is needed to
        avoid using a temporary object that may be unreferenced elsewhere. */

#define yor_get_opaque_data(STACK, TYPE) \
                            (yor_get_opaque(STACK, TYPE, 1)->data)
/*----- Get the client data of the stack opaque object referenced by symbol
        *STACK which must be a Yeti object type and, if TYPE is not NULL, must
        *be an instance of this type.  An error is issued if symbol STACK does
        *not reference an opaque object or if TYPE is not NULL and is not that
        *of the object.  See yor_get_opaque for side effects. */

/*---------------------------------------------------------------------------*/
_YOR_END_DECLS
#endif /* _YETI_H */
