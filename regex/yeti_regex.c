/*
 * yeti_regex.c --
 *
 * Regular expressions for Yorick.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 1998, 1999, 2002, 2015: Éric Thiébaut
 * <eric.thiebaut@obs.univ-lyon1.fr>
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can use, modify
 * and/or redistribute the software under the terms of the CeCILL-C license as
 * circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
 *
 *-----------------------------------------------------------------------------
 */

#include <string.h>

/* POSIX says that <sys/types.h> must be included (by the caller) before
   <regex.h>.  */
#include <sys/types.h>

/* On some systems, limits.h sets RE_DUP_MAX to a lower value than
   GNU regex allows.  Include it before <regex.h>, which correctly
   #undefs RE_DUP_MAX and sets it to the right value.  */
#include <limits.h>

/*---------------------------------------------------------------------------*/
/* DEFINITIONS FOR POSIX REGULAR EXPRESSION ROUTINES */

#if HAVE_REGEX /* Use external regular expression library. */

# include <regex.h>

#else /* not HAVE_REGEX: use builtin regex from GLIBC */

# define HAVE_REGERROR 1 /* we de have regerror in builtin library */

# undef HAVE_ALLOCA_H
# undef HAVE_CONFIG_H
# undef HAVE_ISBLANK /* isblank() is a GNU extension */
# undef HAVE_LANGINFO_CODESET
# undef HAVE_LANGINFO_H
# undef HAVE_LIBINTL
# undef HAVE_LIBINTL_H
# undef HAVE_LOCALE_H
# undef HAVE_MBRTOWC
# undef HAVE_MEMPCPY /* mempcpy() is a GNU extension */
# undef HAVE_WCHAR_H
# undef HAVE_WCRTOMB
# undef HAVE_WCSCOLL
# undef HAVE_WCTYPE_H

# ifdef _AIX
#  pragma alloca
# else
#  ifndef allocax           /* predefined by HP cc +Olibcalls */
#   ifdef __GNUC__
#    define alloca(size) __builtin_alloca (size)
#   else
#    if HAVE_ALLOCA_H
#     include <alloca.h>
#    else
#     ifdef __hpux
         void *alloca ();
#     else
#      if !defined __OS2__ && !defined WIN32
         char *alloca ();
#      else
#       include <malloc.h>       /* OS/2 defines alloca in here */
#      endif
#     endif
#    endif
#   endif
#  endif
# endif

/* We have to keep the namespace clean. */
# define __re_error_msgid     yt___re_error_msgid
# define __re_error_msgid_idx yt___re_error_msgid_idx
# define re_comp              yt_re_comp
# define re_compile_fastmap   yt_re_compile_fastmap
# define re_compile_pattern   yt_re_compile_pattern
# define re_exec              yt_re_exec
# define re_match             yt_re_match
# define re_match_2           yt_re_match_2
# define re_search            yt_re_search
# define re_search_2          yt_re_search_2
# define re_set_registers     yt_re_set_registers
# define re_set_syntax        yt_re_set_syntax
# define re_syntax_options    yt_re_syntax_options
# define regcomp              yt_regcomp
# define regerror             yt_regerror
# define regexec              yt_regexec
# define regfree              yt_regfree

# include "./glibc/regex.h"
# include "./glibc/regex_internal.h"
# include "./glibc/regex_internal.c"
# include "./glibc/regcomp.c"
# include "./glibc/regexec.c"

# if defined(_LIBC)
#  error macro _LIBC defined
# endif

#endif /* HAVE_REGEX */

/*---------------------------------------------------------------------------*/

#include "pstdlib.h"
#include "ydata.h"
#include "yio.h"

/* Macro to get rid of some GCC extensions when not compiling with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#  undef __attribute__
#  define __attribute__(x) /* empty */
#endif

# define p_strfree       p_free            /* usage: p_strfree(STR) */
# define p_stralloc(LEN) p_malloc((LEN)+1) /* usage: p_stralloc(LEN) */

/* Redefine definition of YError to avoid GCC warnings (about uninitialized
   variables or reaching end of non-void function): */
extern void YError(const char *msg) __attribute__ ((noreturn));

/*---------------------------------------------------------------------------*/
/* MISCELLANEOUS PRIVATE ROUTINES */

#define MY_ROUND_UP(a,b) ((((a)+(b)-1)/(b))*(b))

typedef struct ws ws_t;
struct ws {
  /* Common part of all Yorick's DataBlocks: */
  int references;     /* reference counter */
  Operations *ops;    /* virtual function table */
};

static void FreeWS(void *addr);
static UnaryOp PrintWS;

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
extern MemberOp GetMemberX;

Operations wsOps= {
  &FreeWS, T_OPAQUE, 0, /* promoteID= */ T_STRING /* means illegal */,
  "workspace",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &EvalX, &SetupX, &GetMemberX, &MatMultX, &PrintWS
};

/* FreeRE is automatically called by Yorick to delete a regex data block
   that is no more referenced. */
static void FreeWS(void *addr)
{
  p_free(addr);
}

/* PrintRE is used by Yorick's info command. */
static void PrintWS(Operand *op)
{
  ForceNewline();
  PrintFunc("object of type: workspace");
  ForceNewline();
}

static void *my_push_workspace(size_t nbytes)
{
  /* EXTRA is the number of bytes needed to store DataBlock header rounded
     up to the size of a double (to avoid alignment errors). */
  const size_t extra = MY_ROUND_UP(sizeof(ws_t), sizeof(double));
  ws_t *ws = p_malloc(nbytes + extra);
  ws->references = 0;
  ws->ops = &wsOps;
  return (void *)((char *)PushDataBlock(ws) + extra);
}

/* tmpDims is a global temporary for Dimension lists under construction -- you
   should always use it, then just leave your garbage there when you are done
   for the next guy to clean up -- your part of the perpetual cleanup comes
   first. */
static void my_reset_dims(void)
{
  if (tmpDims) {
    Dimension *dims = tmpDims;
    tmpDims = NULL;
    FreeDimension(dims);
  }
}

#define MY_APPEND_DIMENSION(number, origin) \
        (tmpDims = NewDimension(number, origin, tmpDims))
/*----- Append a new temporary dimension (i.e. insert dimension before
        tmpDims) and return tmpDims. */

/* Suffixes in Yorick:
 *   C = char
 *   S = short
 *   I = int
 *   L = long
 *   F = float
 *   D = double
 *   Z = complex
 *   Q = string
 *   P = pointer
 */
#define MY_PUSH_NEW_ARRAY(SDEF, DIMS)((Array *)PushDataBlock(NewArray(SDEF, DIMS)))
#define MY_PUSH_NEW_ARRAY_C(DIMS) MY_PUSH_NEW_ARRAY(&charStruct,    DIMS)
#define MY_PUSH_NEW_ARRAY_S(DIMS) MY_PUSH_NEW_ARRAY(&shortStruct,   DIMS)
#define MY_PUSH_NEW_ARRAY_I(DIMS) MY_PUSH_NEW_ARRAY(&intStruct,     DIMS)
#define MY_PUSH_NEW_ARRAY_L(DIMS) MY_PUSH_NEW_ARRAY(&longStruct,    DIMS)
#define MY_PUSH_NEW_ARRAY_F(DIMS) MY_PUSH_NEW_ARRAY(&floatStruct,   DIMS)
#define MY_PUSH_NEW_ARRAY_D(DIMS) MY_PUSH_NEW_ARRAY(&doubleStruct,  DIMS)
#define MY_PUSH_NEW_ARRAY_Z(DIMS) MY_PUSH_NEW_ARRAY(&complexStruct, DIMS)
#define MY_PUSH_NEW_ARRAY_Q(DIMS) MY_PUSH_NEW_ARRAY(&stringStruct,  DIMS)
#define MY_PUSH_NEW_ARRAY_P(DIMS) MY_PUSH_NEW_ARRAY(&pointerStruct, DIMS)
/*----- These macros allocate a  new Yorick array with dimension list DIMS,
        push it  on top of  the stack and  return the address of  the array
        structure.  There  must be an element  left on top of  the stack to
        store the new array.  See also: MY_PUSH_NEW_A. */


#define MY_PUSH_NEW_A(SDEF, DIMS, MEMBER) (MY_PUSH_NEW_ARRAY(SDEF, DIMS)->value.MEMBER)
#define MY_PUSH_NEW_C(DIMS) MY_PUSH_NEW_A(&charStruct,    DIMS, c)
#define MY_PUSH_NEW_S(DIMS) MY_PUSH_NEW_A(&shortStruct,   DIMS, s)
#define MY_PUSH_NEW_I(DIMS) MY_PUSH_NEW_A(&intStruct,     DIMS, i)
#define MY_PUSH_NEW_L(DIMS) MY_PUSH_NEW_A(&longStruct,    DIMS, l)
#define MY_PUSH_NEW_F(DIMS) MY_PUSH_NEW_A(&floatStruct,   DIMS, f)
#define MY_PUSH_NEW_D(DIMS) MY_PUSH_NEW_A(&doubleStruct,  DIMS, d)
#define MY_PUSH_NEW_Z(DIMS) MY_PUSH_NEW_A(&complexStruct, DIMS, d)
#define MY_PUSH_NEW_Q(DIMS) MY_PUSH_NEW_A(&stringStruct,  DIMS, q)
#define MY_PUSH_NEW_P(DIMS) MY_PUSH_NEW_A(&pointerStruct, DIMS, p)
/*----- These macros allocate a  new Yorick array with dimension list DIMS,
        push it  on top  of the stack  and return  the base address  of the
        array  contents.   See  MY_PUSH_NEW_ARRAY  for side  effects  and
        restrictions. */

static char *my_strncpy(const char *s, size_t len)
{
  if (s) {
    char *t = p_stralloc(len);
    memcpy(t, s, len);
    t[len] = '\0';
    return t;
  }
  return (char *)0;
}

static int my_get_boolean(Symbol *s)
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
      case T_CHAR:   return (*(char   *)op.value != 0);
      case T_SHORT:  return (*(short  *)op.value != 0);
      case T_INT:    return (*(int    *)op.value != 0);
      case T_LONG:   return (*(long   *)op.value != 0L);
      case T_FLOAT:  return (*(float  *)op.value != 0.0F);
      case T_DOUBLE: return (*(double *)op.value != 0.0);
      case T_COMPLEX:return (((double *)op.value)[0] != 0.0 ||
                             ((double *)op.value)[1] != 0.0);
      case T_STRING: return (op.value != NULL);
      case T_VOID:   return 0;
      }
    }
  }
  YError("bad non-boolean argument");
  return 0; /* avoid compiler warning */
}

static void my_unknown_keyword(void)
{ YError("unrecognized keyword in builtin function call"); }

/*---------------------------------------------------------------------------*/
/* SUPPORT FOR DYNAMIC STRING */

static char  *ds_string = NULL;
static size_t ds_size   = 0;
static size_t ds_length = 0;

static void  ds_reset(void);
static void  ds_free(void);
static char *ds_copy(void);
static void  ds_append(const char *str, size_t len);

static void ds_reset(void)
{
  if (ds_string) ds_string[0] = 0;
  else           ds_size = 0;
  ds_length = 0;
}

static void ds_free(void)
{
  char *tmp = ds_string;
  ds_string = NULL;
  ds_length = 0;
  ds_size = 0;
  if (tmp) p_free(tmp);
}

static void ds_append(const char *str, size_t len)
{
  if (len) {
    size_t newlen;
    if (! ds_string) ds_length = ds_size = 0; /* in case of interrupts... */
    if ((newlen = ds_length + len) >= ds_size) {
      char *newstr, *oldstr = ds_string;
      size_t newsiz = 128;
      while (newlen >= newsiz) newsiz *= 2;
      newstr = p_malloc(newsiz);
      if (ds_length) memcpy(newstr, ds_string, ds_length);
      newstr[ds_length] = 0;
      ds_string = newstr;
      ds_size = newsiz;
      if (oldstr) p_free(oldstr);
    }
    ds_string[newlen] = 0;
    memcpy(ds_string + ds_length, str, len);
    ds_length = newlen;
  }
}

static char *ds_copy(void)
{
  if (! ds_string) ds_length = ds_size = 0; /* in case of interrupts... */
  return my_strncpy(ds_string, ds_length);
}

/*---------------------------------------------------------------------------*/
/* The regdb_t is a Yorick DataBlock that stores compiled regular expression
   -- when it is destroyed, all related resources are automatically freed. */
typedef struct regdb regdb_t;
struct regdb {
  /* Common part of all Yorick's DataBlocks: */
  int references;     /* reference counter */
  Operations *ops;    /* virtual function table */

  /* Specific part for this kind of object: */
  int         cflags; /* flags used to compile the regular expression */
  regex_t     regex;  /* compiled regular expression */
};

static void FreeRE(void *addr);
static UnaryOp PrintRE;

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
extern MemberOp GetMemberX;

Operations regexOps= {
  &FreeRE, T_OPAQUE, 0, /* promoteID= */ T_STRING /* means illegal */,
  "regex",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &EvalX, &SetupX, &GetMemberX, &MatMultX, &PrintRE
};

/* FreeRE is automatically called by Yorick to delete a regex data block
   that is no more referenced. */
static void FreeRE(void *addr)
{
  regfree(&(((regdb_t *)addr)->regex));
  p_free(addr);
}

/* PrintRE is used by Yorick's info command. */
static void PrintRE(Operand *op)
{
  regdb_t *re = (regdb_t *)op->value;
  char line[140];
  int flag = 0, cflags = re->cflags;

  sprintf(line, "compiled regular expression (%d ref.): nsub=%d; flags=",
          re->references, (int)re->regex.re_nsub);
  if (! (cflags&REG_EXTENDED)) {
    strcat(line, "basic");
    flag = 1;
  }
  if (cflags&REG_ICASE) {
    if (flag) strcat(line, "|");
    strcat(line, "icase");
    flag = 1;
  }
  if (cflags&REG_NOSUB) {
    if (flag) strcat(line, "|");
    strcat(line, "nosub");
    flag = 1;
  }
  if (cflags&REG_NEWLINE) {
    if (flag) strcat(line, "|");
    strcat(line, "newline");
    flag = 1;
  }
  if (! flag) strcat(line, "<default>");
  ForceNewline();
  PrintFunc(line);
  ForceNewline();
}

/*---------------------------------------------------------------------------*/

extern BuiltIn Y_regcomp, Y_regmatch, Y_regsub;

static const char *regex_error_message(int errcode, const regex_t *preg);
/*	Return regular expression error message stored in a static buffer. */

#define DEFAULT_CFLAGS (REG_EXTENDED)

static regdb_t *new_regdb(const char *regex, int cflags);
/*----- Compile regular expression and return new data-block. */

static regdb_t *get_regdb(Symbol *stack, int cflags);
/*----- Return compiled regular expression data-block for stack symbol STACK.
        If STACK is a scalar string, it gets compiled according to CFLAGS
        or with DEFAULT_CFLAGS if CFLAGS=-1 (see man page of regcomp);
        otherwise it must be a compiled regular expression compiled and
        CFLAGS must be -1.  If STACK is a reference or a scalar string, it
        get replaced by the result. */

/*---------------------------------------------------------------------------*/

static long id_all     = -1;
static long id_basic   = -1;
static long id_icase   = -1;
static long id_indices = -1;
static long id_newline = -1;
static long id_nosub   = -1;
static long id_notbol  = -1;
static long id_noteol  = -1;
static long id_start   = -1;
static int first_time = 1;

static void initialize(void)
{
  id_all     = Globalize("all",     3);
  id_basic   = Globalize("basic",   5);
  id_icase   = Globalize("icase",   5);
  id_indices = Globalize("indices", 7);
  id_newline = Globalize("newline", 7);
  id_nosub   = Globalize("nosub",   5);
  id_notbol  = Globalize("notbol",  6);
  id_noteol  = Globalize("noteol",  6);
  id_start   = Globalize("start",   5);
}

void Y_regcomp(int argc)
{
  Symbol *stack, *regex = NULL;
  DataBlock *db;
  Array *array;
  int cflags=DEFAULT_CFLAGS;

  /* Initialize internals as needed. */
  if (first_time) {
    initialize();
    first_time = 0;
  }

  /* Parse arguments from first to last one. */
  for (stack=sp-argc+1 ; stack<=sp ; ++stack) {
    if (stack->ops) {
      /* Normal argument. */
      if (regex) goto badNArgs;
      regex = (stack->ops == &referenceSym) ? &globTab[stack->index] : stack;
    } else {
      /* Must be a keyword: sp[i] is for keyword and sp[i+1] for
         related value. */
      long id = stack->index;
      ++stack;
      if (id == id_icase) {
        if (my_get_boolean(stack)) cflags |= REG_ICASE;
      } else if (id == id_newline) {
        if (my_get_boolean(stack)) cflags |= REG_NEWLINE;
      } else if (id == id_nosub) {
        if (my_get_boolean(stack)) cflags |= REG_NOSUB;
      } else if (id == id_basic) {
        if (my_get_boolean(stack)) cflags &= ~REG_EXTENDED;
      } else {
        my_unknown_keyword();
      }
    }
  }
  if (! regex) {
  badNArgs:
    YError("regcomp takes exactly 1 non-keyword argument");
  }
  if (regex->ops == &referenceSym) regex = &globTab[regex->index];
  if (regex->ops != &dataBlockSym ||
      (db = regex->value.db)->ops != &stringOps ||
      (array = (Array *)db)->type.dims != NULL) {
    YError("expecting scalar string");
  }
  db = (DataBlock *)new_regdb(array->value.q[0], cflags);
  PushDataBlock(db);
}

/*---------------------------------------------------------------------------*/

void Y_regmatch(int argc)
{
  void **outPtr;     /* array of pointers to outputs */
  long  *outIndex;   /* array of output index */
  long   nmatch;     /* number of required outputs */
  regoff_t start_option=1; /* starting index in matching string */
  regoff_t start=1;        /* actual starting index in matching string */
  regoff_t len;            /* length of input string */
  regmatch_t *pmatch;
  Dimension *dims = NULL;
  Symbol *stack, *last_arg, *regexSymbol= NULL;
  regdb_t *re;
  regex_t *regex;
  char **input=NULL, *str;
  int indices=0, eflags=0, tflags=DEFAULT_CFLAGS, cflags=-1;
  long i, number, j;
  int status, *match;

  /* Initialize internals as needed. */
  if (first_time) {
    initialize();
    first_time = 0;
  }

  /* First pass on argument list to parse keywords and figure out the number
     of outputs requested. */
  last_arg = sp;
  nmatch = -2;
  for (stack = last_arg+1-argc ; stack <= last_arg ; ++stack) {
    if (stack->ops) {
      /* Normal argument. */
      ++nmatch;
    } else {
      /* Keyword argument: sp[i] is for keyword name and sp[i+1] for
         its value. */
      long id = stack->index;
      ++stack;
      if (id == id_icase) {
        if (my_get_boolean(stack)) tflags |= REG_ICASE;
        cflags = tflags;
      } else if (id == id_newline) {
        if (my_get_boolean(stack)) tflags |= REG_NEWLINE;
        cflags = tflags;
      } else if (id == id_nosub) {
        if (my_get_boolean(stack)) tflags |= REG_NOSUB;
        cflags = tflags;
      } else if (id == id_basic) {
        if (my_get_boolean(stack)) tflags &= ~REG_EXTENDED;
        cflags = tflags;
      } else if (id == id_notbol) {
        if (my_get_boolean(stack)) eflags |= REG_NOTBOL;
      } else if (id == id_noteol) {
        if (my_get_boolean(stack)) eflags |= REG_NOTEOL;
      } else if (id == id_indices) {
        indices = my_get_boolean(stack);
      } else if (id == id_start) {
        start_option = YGetInteger(stack);
      } else {
        my_unknown_keyword();
      }
    }
  }
  if (nmatch < 0) YError("regmatch takes at least 2 non-keyword arguments");

  /* Allocate enough workspace for outputs. */
  if (nmatch > 0) {
    CheckStack(nmatch + 4);
    last_arg = sp; /* in case stack was relocated */
#define ALLOC_WS(PTR, NUMBER) PTR=my_push_workspace((NUMBER)*sizeof(*(PTR)))
    ALLOC_WS(outPtr,   nmatch);
    ALLOC_WS(outIndex, nmatch);
    ALLOC_WS(pmatch,   nmatch);
    /*cflags|= REG_EXTENDED;*/
#undef ALLOC_WS
  } else {
    outPtr   = NULL;
    outIndex = NULL;
    pmatch   = NULL;
    /*cflags|= REG_NOSUB;*/
  }

  /* Parse non-keyword arguments from first to last one (must be done
     _after_ call to CheckStack). */
  j = 0;
  for (stack = last_arg+1-argc ; stack <= last_arg ; ++stack) {
    if (stack->ops) {
      /* Normal argument. */
      if (! regexSymbol) {
        regexSymbol = stack;
      } else if (! input) {
        input = YGet_Q(stack, 0, &dims);
      } else {
        outPtr[j] = stack;
        outIndex[j] = (stack->ops == &referenceSym) ? stack->index : -1;
        ++j;
      }
    } else {
      /* Keyword argument (skip it). */
      ++stack;
    }
  }

  /* Get/compile regular expression. */
  re = get_regdb(regexSymbol, cflags);
  regex = &re->regex;

  /* Push result on top of the stack (must be done *BEFORE* other
     outputs). */
  match = MY_PUSH_NEW_I(dims);

  /* Prepare output arrays. */
  number = TotalNumber(dims);
  if (indices) {
    Dimension *ptr;
    my_reset_dims();
    MY_APPEND_DIMENSION(2, 1);
    for (ptr=dims ; ptr!=NULL ; ptr=ptr->next) {
      MY_APPEND_DIMENSION(ptr->number, ptr->origin);
    }
    for (j=0 ; j<nmatch ; ++j) {
      outPtr[j] = MY_PUSH_NEW_L(tmpDims);
    }
  } else {
    for (j=0 ; j<nmatch ; ++j) {
      outPtr[j] = MY_PUSH_NEW_Q(dims);
    }
  }

  /* Match regular expression against input string(s). */
# define OUTPUT(TYPE, I1, I2) ((TYPE*)(outPtr[I1]))[I2]
# define OUTPUT_L(I1, I2)     OUTPUT(long, I1,I2)
# define OUTPUT_Q(I1, I2)     OUTPUT(char*,I1,I2)
  for (i=0 ; i<number ; i++) {
    str = input[i];
    if (str && start_option != 1) {
      len = strlen(str);
      if (start_option >= 1) {
        if ((start = start_option) <= len) {
          str += start - 1;
        } else {
          str = NULL;
        }
      } else {
        if ((start = len - start_option) >= 1) {
          str += start - 1;
        } else {
          str = NULL;
        }
      }
    }
    if (! str || (status = regexec(regex, str, nmatch,
                                   pmatch, eflags)) == REG_NOMATCH) {
      /* No match. */
      match[i] = 0;
      if (indices) {
        for (j=0 ; j<nmatch ; ++j) {
          OUTPUT_L(j, 2*i)   = -1;
          OUTPUT_L(j, 2*i+1) = -1;
        }
      }
    } else if (! status) {
      /* Match (yorick: add START to indices). */
      match[i] = 1;
      if (indices) {
        for (j=0 ; j<nmatch ; ++j) {
          OUTPUT_L(j, 2*i)   = pmatch[j].rm_so + start;
          OUTPUT_L(j, 2*i+1) = pmatch[j].rm_eo + start;
        }
      } else {
        for (j=0 ; j<nmatch ; ++j) {
          if (pmatch[j].rm_eo > pmatch[j].rm_so) {
            OUTPUT_Q(j, i) = my_strncpy(str + pmatch[j].rm_so,
                                        pmatch[j].rm_eo - pmatch[j].rm_so);
          }
        }
      }
    } else {
      YError(regex_error_message(status, regex));
    }
  }
# undef OUTPUT
# undef OUTPUT_L
# undef OUTPUT_Q

  /* Pop outputs in place (from last to first one) and left result on top
     of the stack. */
  for (j=nmatch-1 ; j>=0 ; --j) {
    if (outIndex[j]<0) Drop(1);
    else PopTo(&globTab[outIndex[j]]);
  }
}

/*---------------------------------------------------------------------------*/

void Y_regsub(int argc)
{
  regmatch_t *match;
  Symbol *stack, *regexSymbol=NULL;
  regdb_t *re;
  regex_t *regex;
  Dimension *dims= NULL;
  const char *substr= NULL, *src, *end;
  char **input= NULL, **output, *dst;
  int all=0, eflags=0, tflags=DEFAULT_CFLAGS, cflags=-1;
  long len, number, nsub, srclen, index;
  int i, j, c, status, nnodes, argnum;
  size_t part1;
  struct Node {
    /* This structure describes a node in the substitution string.  An
       array of such nodes provides some sort of compiled version of the
       substitution string. */
    const char *p; /* non-NULL means textual string
                      NULL means index of sub-expression */
    long l;        /* length of textual string or index of sub-expression */
  } *node;

  /* Initialize internals as needed. */
  if (first_time) {
    initialize();
    first_time = 0;
  }

  /* Parse arguments from first to last one. */
  argnum = 0;
  for (stack=sp+1-argc ; stack<=sp ; ++stack) {
    if (stack->ops) {
      /* Normal argument. */
      switch (++argnum) {
      case 1: regexSymbol = stack; break;
      case 2: input = YGet_Q(stack, 0, &dims); break;
      case 3: substr = YGetString(stack); break;
      default: goto badNArgs;
      }
    } else {
      /* Keyword argument: sp[i] is for keyword name and sp[i+1] for
         its value. */
      long id = stack->index;
      ++stack;
      if (id == id_icase) {
        if (my_get_boolean(stack)) tflags |= REG_ICASE;
        cflags = tflags;
      } else if (id == id_newline) {
        if (my_get_boolean(stack)) tflags |= REG_NEWLINE;
        cflags = tflags;
      } else if (id == id_nosub) {
        if (my_get_boolean(stack)) tflags |= REG_NOSUB;
        cflags = tflags;
      } else if (id == id_basic) {
        if (my_get_boolean(stack)) tflags &= ~REG_EXTENDED;
        cflags = tflags;
      } else if (id == id_notbol) {
        if (my_get_boolean(stack)) eflags |= REG_NOTBOL;
      } else if (id == id_noteol) {
        if (my_get_boolean(stack)) eflags |= REG_NOTEOL;
      } else if (id == id_all) {
        all = my_get_boolean(stack);
      } else {
        my_unknown_keyword();
      }
    }
  }
  if (argnum < 2 || argnum > 3) {
  badNArgs:
    YError("regsub takes 2 or 3 non-keyword arguments");
  }

  /* Get/compile regular expression. */
  re = get_regdb(regexSymbol, cflags);
  regex = &re->regex;

  /* Make sure the stack can holds 2 more items: a temporary workspace and
     the result of the call. */
  CheckStack(2);

  /* Allocate workspace:   NSUB+1  regmatch_t    for MATCH array
   *                     + LEN     struct Node   for NODE array
   *			 + LEN+1   char          for literal parts of SUBSTR.
   * Notes: 1. Allocate as many nodes as characters in SUBSTR since this is
   *           the maximum possible number of nodes; furthermore the
   *	       allocation is done in one chunk.
   *        2. Round up sizes of different parts to avoid alignment errors.
   */
  len = (substr ? strlen(substr) : 0);
  nsub = regex->re_nsub; /* number of subexpressions in compiled regex */
  part1 = (nsub + 1)*sizeof(match[0]);
  part1 = MY_ROUND_UP(part1, sizeof(node[0]));
  match = my_push_workspace(part1 + len*(1 + sizeof(node[0])) + 1);
  node = (void *)((char *)match + part1);
  dst  = (char *)(node + len);

  /* Compile substitution string SUBSTR by splitting it into nodes. */
  nnodes = 0;
  if (len) {
#define ADD_NODE(P,L) node[nnodes].p=(P);  node[nnodes++].l=(L)
    int l = 0;
    for (i=0 ; ; ) {
      c = substr[i++];
#if 0
      if (c == '&') {
        if (l) { ADD_NODE(dst,l); dst[l]=0; dst+=l+1; l=0; }
        ADD_NODE(NULL, 0);
        continue;
      }
#endif
      if (c == '\\') {
        c = substr[i++];
        index = c - '0';
        if (index >= 0 && index <= 9) {
          if (index > nsub)
            YError("sub-expression index overreach number of sub-expressions");
          if (l) { ADD_NODE(dst,l); dst[l]=0; dst+=l+1; l=0; }
          ADD_NODE(NULL, index);
          continue;
        }
        if (c == 0) YError("bad final backslash in substitution string");
      } else if (c==0) {
        /* End of string. */
        if (l) { ADD_NODE(dst,l); dst[l]=0; }
        break;
      }
      /* Literal character. */
      dst[l++] = c;
    }
#undef ADD_NODE
  }

  /* Allocate output string array and enough workspace. */
  number = TotalNumber(dims);
  output = MY_PUSH_NEW_Q(dims);

  /* Match regular expression against input string(s). */
  for (i=0 ; i<number ; ++i) {
    if (! (src = input[i])) {
      output[i] = NULL; /* FIXME: not needed? */
      continue;
    }

    srclen = strlen(src);
    end = src + srclen;
    tflags = eflags;
    ds_reset();
    for (;;) {
      status = regexec(regex, src, nsub+1, match, tflags);
      if (status) {
        /* No match or error. */
        if (status == REG_NOMATCH) break;
        YError(regex_error_message(status, regex));
      }

      /* Copy the head of the source string that didn't match the regular
         expression. */
      len = match[0].rm_so;
      if (len > 0) ds_append(src, len);

      /* Substitute each nodes. */
      for (j=0 ; j<nnodes ; ++j) {
        if (node[j].p) {
          /* Copy literal part. */
          ds_append(node[j].p, node[j].l);
        } else {
          /* Substitute subexpression. */
          index = node[j].l;
          if (match[index].rm_eo > match[index].rm_so) {
            ds_append(src + match[index].rm_so,
                      match[index].rm_eo - match[index].rm_so);
          }
        }
      }

      /* Skip the part of the source string that matched the entire
         regular expression (advance source pointer by at least 1
         character to avoid infinite loop). */
      if (match[0].rm_eo > match[0].rm_so) src += match[0].rm_eo;
      else                                 src += match[0].rm_so + 1;
      if (! all || src >= end) break;
      tflags |= REG_NOTBOL; /* since SRC is advanced, we are no longer
                               at the beginning of the string. */
    }

    /* Copy the tail of the source string that didn't match the regular
       expression. */
    len = srclen - (src - input[i]);
    if (len > 0) ds_append(src, len);

    /* Stores the substituted string into the output array. */
    output[i] = ds_copy();
  }
  ds_free();
}

/*---------------------------------------------------------------------------*/

static regdb_t *new_regdb(const char *regex, int cflags)
{
  regdb_t *re;
  int status;
  if (! regex) YError("unexpected nil string");
  re = p_malloc(sizeof(regdb_t));
  re->references = 0;
  re->ops = &regexOps;
  re->cflags = cflags;
  status = regcomp(&re->regex, regex, cflags);
  if (status) {
    const char *msg = regex_error_message(status, &re->regex);
    FreeRE(re);
    YError(msg);
  }
  return re;
}

static regdb_t *get_regdb(Symbol *stack, int cflags)
{
  Symbol *s = (stack->ops == &referenceSym) ? &globTab[stack->index] : stack;
  if (s->ops == &dataBlockSym) {
    DataBlock *db = s->value.db;
    if (db->ops == &regexOps) {
      if (cflags != -1)
        YError("attempt to modify flags in compiled regular expression");
      if (s != stack) {
        /* Replace reference (we already know that S is a DataBlock). */
        stack->value.db = Ref(db);
        stack->ops = &dataBlockSym; /* change ops only AFTER value updated */
      }
      return (regdb_t *)db;
    }
    if (db->ops == &stringOps) {
      Array *array = (Array *)db;
      if (! array->type.dims) {
        /* Compile regular expression and store it into a new
           data block that is returned. */
        regdb_t *re = new_regdb(array->value.q[0],
                                (cflags == -1 ? DEFAULT_CFLAGS : cflags));
        db = (stack->ops==&dataBlockSym) ? stack->value.db : NULL;
        stack->value.db = (DataBlock *)re;
        stack->ops = &dataBlockSym;
        if (db) Unref(db);
        return re;
      }
    }
  }
  YError("expecting scalar string or compiled regular expression");
#if ! (defined(__GNUC__) && __GNUC__ > 1)
  return NULL; /* This is to avoid compiler warnings. */
#endif
}

/*---------------------------------------------------------------------------*/

static const char *regex_error_message(int errcode, const regex_t *preg)
{
#if HAVE_REGERROR
  static char errbuf[128];
  regerror(errcode, preg, errbuf, sizeof(errbuf));
  return errbuf;
#else /* HAVE_REGERROR */
  switch (errcode) {
#ifdef REG_BADRPT
  case REG_BADRPT:
    return "regex: Invalid use of repetition operators such as using `*' as the first character.";
#endif /* REG_BADRPT */
#ifdef REG_BADBR
  case REG_BADBR:
    return "regex: Invalid use of back reference operator.";
#endif /* REG_BADBR */
#ifdef REG_EBRACE
  case REG_EBRACE:
    return "regex: Un-matched brace interval operators.";
#endif /* REG_EBRACE */
#ifdef REG_EBRACK
  case REG_EBRACK:
    return "regex: Un-matched bracket list operators.";
#endif /* REG_EBRACK */
#ifdef REG_ERANGE
  case REG_ERANGE:
    return "regex: Invalid  use  of the range operator, eg. the ending point of the range occurs prior to the starting point.";
#endif /* REG_ERANGE */
#ifdef REG_ECTYPE
  case REG_ECTYPE:
    return "regex: Unknown character class name.";
#endif /* REG_ECTYPE */
#ifdef REG_EPAREN
  case REG_EPAREN:
    return "regex: Un-matched parenthesis group operators.";
#endif /* REG_EPAREN */
#ifdef REG_ESUBREG
  case REG_ESUBREG:
    return "regex: Invalid back reference to a subexpression.";
#endif /* REG_ESUBREG */
#ifdef REG_EEND
  case REG_EEND:
    return "regex: Non specific error.";
#endif /* REG_EEND */
#ifdef REG_ESCAPE
  case REG_ESCAPE:
    return "regex: Invalid escape sequence.";
#endif /* REG_ESCAPE */
#ifdef REG_BADPAT
  case REG_BADPAT:
    return "regex: Invalid  use  of pattern operators such as group or list";
#endif /* REG_BADPAT */
#ifdef REG_ESIZE
  case REG_ESIZE:
    return "regex: Compiled regular expression requires a pattern buffer larger than 64Kb.";
#endif /* REG_ESIZE */
#ifdef REG_ESPACE
  case REG_ESPACE:
    return "The regex routines ran out of memory.";
#endif /* REG_ESPACE */
  default:
    return "regex: Unknown error.";
  }
#endif /* HAVE_REGERROR */
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * fill-column: 79
 * coding: utf-8
 * End:
 */
