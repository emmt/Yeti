/*
 * yeti-compat.h -
 *
 * Definitions for maintaining backward compatibility.
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

#include "yeti.h"

#ifndef _YETI_COMPAT_H
#define _YETI_COMPAT_H 1

#define _YETI_BEGIN_DECLS _YOR_BEGIN_DECLS
#define _YETI_END_DECLS   _YOR_END_DECLS

/*---------------------------------------------------------------------------*/
/* USEFUL MACROS */

#define YETI_OFFSET_OF(T, m) ((char*)&((T*)0)->m - (char*)0)
/*----- Yield offset of member M in structure of type T. */

#define YETI_ROUND_UP(a,b) ((((a)+(b)-1)/(b))*(b))
/*----- Return smallest multiple of integer B that is greater or equal
        integer A. */

#define YETI_VERBATIM(x) x
/*----- Yield argument without macro expansion. */

#define YETI_XSTRINGIFY(x)  YETI_STRINGIFY(x)
/*----- Wrap argument in quotes with macro expansion. */

#define YETI_STRINGIFY(x) # x
/*----- Wrap argument in quotes without macro expansion. */

#define YETI_XJOIN(a,b)   YETI_JOIN(a, b)
/*----- Join arguments A and B with macro expansion. */

#define YETI_JOIN(a,b)    a ## b
/*----- Join arguments A and B without macro expansion. */

/*---------------------------------------------------------------------------*/
/* ERROR MESSAGES */

#define yeti_error                          yor_format_error
#define yeti_bad_argument(sym)              yor_bad_argument(sym)
#define	yeti_unknown_keyword(nil)           yor_unknown_keyword(nil)

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

#define yeti_strcpy(str)                    yor_strcpy(str)   // # FIXME: p_strcpy?
#define yeti_strncpy(str)                   yor_strncpy(str)

#define yeti_is_range(sym)                  yor_is_range(sym)
#define yeti_is_structdef(sym)              yor_is_structdef(sym)
#define yeti_is_stream(sym)                 yor_is_stream(sym)
#define yeti_is_nil(sym)                    yor_is_nil(sym)
#define yeti_is_void(sym)                   yor_is_void(sym)
#define	yeti_debug_symbol(sym)	            yor_debug_symbol(sym)
#define yeti_get_datablock(sym, ops)        yor_get_datablock(sym, ops)
#define yeti_get_array(sym, flg)            yor_get_array(sym, flg)
#define yeti_get_boolean(sym)               yor_get_boolean(sym)
#define	yeti_get_optional_integer(sym, def) yor_get_optional_integer(sym, def)

#define yeti_scalar   _yor_scalar
#define yeti_scalar_t  yor_scalar_t

#define yeti_get_scalar(sym, sclptr)            \
  do {                                          \
    *(sclptr) = yor_get_scalar(sym);            \
  } while (0)

/*---------------------------------------------------------------------------*/
/* STACK FUNCTIONS/MACROS */

#define yeti_pop_and_reduce_to(sym)  yor_pop_and_reduce_to(sym)

#define yeti_get_integer(sym)        yor_get_integer(sym)
#define yeti_get_real(sym)           yor_get_real(sym)
#define yeti_get_string(sym)         yor_get_string(sym)
#define yeti_get_pointer(sym)        yor_get_pointer(sym)

#define yeti_push_char_value(val)    yor_push_char_value(val)
#define yeti_push_short_value(val)   yor_push_short_value(val)
#define yeti_push_int_value(val)     yor_push_int_value(val)
#define yeti_push_long_value(val)    yor_push_long_value(val)
#define yeti_push_float_value(val)   yor_push_float_value(val)
#define yeti_push_double_value(val)  yor_push_double_value(val)
#define yeti_push_complex_value(re, im) \
  yor_push_complex_value(YOR_COMPLEX(re, im))
#define yeti_push_string_value(str)  yor_push_string_value(str)

#define YETI_PUSH_NEW_ARRAY(SDEF, DIMS) ((Array*)PushDataBlock(NewArray(SDEF, DIMS)))
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

PLUG_API Dimension* tmpDims;
/*----- tmpDims is a global temporary for Dimension lists under
        construction -- you should always use it, then just leave your
        garbage there when you are done for the next guy to clean up --
        your part of the perpetual cleanup comes first. */

extern void yeti_reset_dimlist(void);
/*----- Prepares global variable tmpDims for the building of a new
        dimension list (i.e. takes care of freeing old dimension
        list if any). */

extern Dimension* yeti_grow_dimlist(long number);
/*----- Appends dimension of length NUMBER to tmpDims and returns new
        value of tmpDims.  Note that tmpDims must have been properly
        initialized by yeti_reset_dimlist or yeti_start_dimlist (which
        to see).  For instance to build the dimension list of a NCOLS
        by NROWS array, do:
          yeti_reset_dimlist();
          yeti_grow_dimlist(ncols);
          yeti_grow_dimlist(nrows); */

extern Dimension* yeti_start_dimlist(long number);
/*----- Initializes global temporary tmpDims with a single first
        dimension with length NUMBER and returns tmpDims.  Same as:
           yeti_reset_dimlist();
           yeti_grow_dimlist(number); */

extern Dimension* yeti_make_dims(const long number[], const long origin[],
                                 size_t ndims);
/*----- Build and store a dimension list in tmpDims, NDIMS is the number of
        dimensions, NUMBER[i] and ORIGIN[i] are the length and starting
        index along the i-th dimension (if ORIGIN is NULL, all origins are
        set to 1).  The new value of tmpDims is returned. */

extern size_t yeti_get_dims(const Dimension* dims, long number[],
                            long origin[], size_t maxdims);
/*----- Store dimensions along chained list DIMS in arrays NUMBER and
        ORIGIN (if ORIGIN is NULL, it is not used).  There must be no more
        than MAXDIMS dimensions.  Returns the number of dimensions. */

extern int yeti_same_dims(const Dimension* dims1, const Dimension* dims2);
/*----- Check that two dimension lists are identical (also see Conform in
        ydata.c or yeti_total_number_2).  Returns 1 (true) or 0 (false). */

extern void yeti_assert_same_dims(const Dimension* dims1,
                                  const Dimension* dims2);
/*-----	Assert that two dimension lists are identical, raise an error (see
        YError) if this not the case. */

extern long yeti_total_number(const Dimension* dims);
/*----- Returns number of elements of the array with dimension list DIMS. */

extern long yeti_total_number_2(const Dimension* dims1,
                                const Dimension* dims2);
/*----- Check that two dimension lists are identical and return the number
        of elements of the corresponding array.  An error is raised (via
        YError) if dimension lists are not identical. */

/*---------------------------------------------------------------------------*/
/* WORKSPACE */

#define yeti_push_workspace yor_push_workspace

/*---------------------------------------------------------------------------*/
/* OPAQUE OBJECTS */

#define yeti_opaque        _yor_opaque
#define yeti_opaque_t       yor_opaque_t
#define yeti_opaque_type   _yor_opaque_type
#define yeti_opaque_type_t  yor_opaque_type_t
#define yeti_new_opaque     yor_new_opaque_object
#define yeti_get_opaque     yor_get_opaque_object
#define yeti_get_opaque_data(STACK, TYPE) (yeti_get_opaque(STACK,TYPE,1)->data)

#endif /* _YETI_COMPAT_H */
