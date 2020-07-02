/*
 * utils.c -
 *
 * Routines to make the coding of built-in Yorick's functions easier.
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

#include "config.h"
#include "yeti.h"
#include "yio.h"

/*---------------------------------------------------------------------------*/
/* PRIVATE ROUTINES */

static void unexpected_keyword_argument(void);

/*---------------------------------------------------------------------------*/

char* yeti_strcpy(const char* s)
{
  if (s != NULL) {
    size_t len = strlen(s);
    char* t = p_stralloc(len);
    memcpy(t, s, len);
    t[len] = '\0';
    return t;
  }
  return (char*)0;
}

char* yeti_strncpy(const char* s, size_t len)
{
  if (s != NULL) {
    char* t = p_stralloc(len);
    memcpy(t, s, len);
    t[len] = '\0';
    return t;
  }
  return (char*)0;
}

int yeti_is_nil(Symbol* s)
{
  YETI_SOLVE_REFERENCE(s);
  return (s->ops == &dataBlockSym && s->value.db == &nilDB);
}

#undef FUNCTION
#define FUNCTION(NAME, OPS)                                             \
  int yeti_is_##NAME(Symbol* s)                                         \
  {                                                                     \
    YETI_SOLVE_REFERENCE(s);                                            \
    return (s->ops == &dataBlockSym && s->value.db->ops == &OPS);       \
  }
FUNCTION(void, voidOps)
FUNCTION(range, rangeOps)
FUNCTION(structdef, structDefOps)
FUNCTION(stream, streamOps)

void yeti_bad_argument(Symbol* s)
{
  static char buf[80];
  char* msg;

  if (s->ops == NULL) {
    msg = "unexpected keyword argument";
  } else if (s->ops == &intScalar) {
    msg = "unexpected int scalar argument";
  } else if (s->ops == &longScalar) {
    msg = "unexpected long scalar argument";
  } else if (s->ops == &doubleScalar) {
    msg = "unexpected double scalar argument";
  } else if (s->ops == &dataBlockSym) {
    sprintf(buf, "unexpected %s%s argument", s->value.db->ops->typeName,
            (s->value.db->ops->isArray ? " array" : ""));
    msg = buf;
  } else if (s->ops == &referenceSym) {
    /* I assume that YETI_SOLVE_REFERENCE or ReplaceRef was already applied to
       that symbol. */
    msg = "***BUG*** too many reference levels";
  } else if (s->ops == &returnSym) {
    msg = "***BUG*** unexpected return symbol";
  } else {
    msg = "***BUG*** unknown symbol type";
  }
  YError(msg);
}

static void unexpected_keyword_argument(void)
{ YError("unexpected keyword argument in builtin function call"); }

void yeti_unknown_keyword(void)
{ YError("unrecognized keyword in builtin function call"); }

void yeti_debug_symbol(Symbol* s)
{
  fprintf(stderr, "yeti_debug_symbol: s = (Symbol*)0x%lx\n", (unsigned long)s);
  if (s == NULL) return;
  if (s->ops == &doubleScalar) {
    fprintf(stderr, "                s->ops = &doubleScalar\n");
    fprintf(stderr, "                s->index = (long)%ld\n", s->index);
    fprintf(stderr, "                s->value.d = (double)%g\n", s->value.d);
  } else if (s->ops == &longScalar) {
    fprintf(stderr, "                s->ops = &longScalar\n");
    fprintf(stderr, "                s->index = (long)%ld\n", s->index);
    fprintf(stderr, "                s->value.l = (long)%ld\n", s->value.l);
  } else if (s->ops == &intScalar) {
    fprintf(stderr, "                s->ops = &intScalar\n");
    fprintf(stderr, "                s->index = (long)%ld\n", s->index);
    fprintf(stderr, "                s->value.i = (int)%d\n", s->value.i);
  } else if (s->ops == &dataBlockSym) {
    fprintf(stderr, "                s->ops = &dataBlockSym\n");
    fprintf(stderr, "                s->index = (long)%ld\n", s->index);
    fprintf(stderr, "                s->value.db = (DataBlock*)0x%lx\n",
            (unsigned long)s->value.db);
  } else if (s->ops == &referenceSym) {
    /* In `ydata.h': referenceSym is not a "complete" Symbol in the sense
       that the parser ensures it never appears in a binary operation
         ops          - &referenceSym
         index        - to globTab entry
         value.offset - stack offset (for Return only)  */
    fprintf(stderr, "                s->ops = &referenceSym\n");
    fprintf(stderr, "                s->index = (long)%ld\n", s->index);
    fprintf(stderr, "                s->value.offset = (long)%ld\n",
            s->value.offset);
  } else if (s->ops == &returnSym) {
    /* In `ydata.h': returnSym is not a "complete" Symbol in the sense
       that the parser ensures it never appears in a binary operation
         ops      - &returnSym
         index    - (unused)
         value.pc - VM program counter (for Return only)  */
    fprintf(stderr, "                s->ops = &returnSym\n");
    fprintf(stderr, "                s->value.pc = (Instruction*)0x%lx\n",
            (unsigned long)s->value.pc);
  } else if (s->ops == NULL) {
    /* Keywords may also appear on the program stack--
       these are marked by ops == 0.  */
    fprintf(stderr, "                s->ops = (OpTable*)NULL (KEYWORD)\n");
  } else {
    fprintf(stderr, "                s->ops = (OpTable*)0x%lx\n",
            (unsigned long)s->ops);
  }
}

int yeti_get_boolean(Symbol* s)
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
      case T_CHAR:   return (*(char*)op.value != 0);
      case T_SHORT:  return (*(short*)op.value != 0);
      case T_INT:    return (*(int*)op.value != 0);
      case T_LONG:   return (*(long*)op.value != 0L);
      case T_FLOAT:  return (*(float*)op.value != 0.0F);
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

long yeti_get_optional_integer(Symbol* s, long default_value)
{
  if (s->ops == &longScalar) return s->value.l;
  if (s->ops == &intScalar) return s->value.i;
  YETI_SOLVE_REFERENCE(s);
  if (s->ops && s->ops != &doubleScalar) {
    Operand op;
    s->ops->FormOperand(s, &op);
    if (! op.type.dims) {
      if (op.ops == &charOps) return *(char*)op.value;
      if (op.ops == &shortOps) return *(short*)op.value;
      if (op.ops == &intOps) return *(int*)op.value;
      if (op.ops == &longOps) return *(long*)op.value;
      if (op.ops == &voidOps) return default_value;
    }
  }
  yeti_bad_argument(s);
}

yeti_scalar_t* yeti_get_scalar(Symbol* s,  yeti_scalar_t* scalar)
{
  if (s->ops == &longScalar) {
    scalar->type = T_LONG;
    scalar->value.l = s->value.l;
  } else if (s->ops == &doubleScalar) {
    scalar->type = T_DOUBLE;
    scalar->value.d = s->value.d;
  } else if (s->ops == &intScalar) {
    scalar->type = T_INT;
    scalar->value.i = s->value.i;
  } else {
    Operand op;
    if (! s->ops) unexpected_keyword_argument();
    s->ops->FormOperand(s, &op);
    if (op.type.dims) YError("expecting scalar argument");
    scalar->type = op.ops->typeID;
    switch(scalar->type) {
#define _(MEMBER, TYPE) scalar->value.MEMBER = *(TYPE*)(op.value); break
    case T_CHAR:    _(c, char);
    case T_SHORT:   _(s, short);
    case T_INT:     _(i, int);
    case T_LONG:    _(l, long);
    case T_FLOAT:   _(f, float);
    case T_DOUBLE:  _(d, double);
    case T_COMPLEX:
      scalar->value.z.re = ((double*)(op.value))[0];
      scalar->value.z.im = ((double*)(op.value))[1];
      break;
    case T_STRING:  _(q, char*);
    case T_POINTER: _(p, void*);
#undef _
    default:
      scalar->value.p = op.value;
    }
  }
  return scalar;

}

DataBlock* yeti_get_datablock(Symbol* stack, const Operations* ops)
{
#if 0
  if (stack->ops == &referenceSym) ReplaceRef(stack);
  if (stack->ops != &dataBlockSym || (ops && stack->value.db->ops != ops))
    yeti_bad_argument(stack);
  return stack->value.db;
#else
  DataBlock* db;
  Symbol* sym = YETI_DEREFERENCE_SYMBOL(stack);
  if (sym->ops != &dataBlockSym || (ops && sym->value.db->ops != ops))
    yeti_bad_argument(sym);
  db = sym->value.db;
  if (sym != stack) {
    /* Replace reference onto the stack (equivalent to the statement
       ReplaceRef(stack); see ydata.c for actual code of this routine). */
    stack->value.db = Ref(db);
    stack->ops = &dataBlockSym;     /* change ops only AFTER value updated */
  }
  return db;
#endif
}

Array* yeti_get_array(Symbol* s, int nil_ok)
{
#if 0
  ReplaceRef(s);
  if (s->ops == &dataBlockSym) {
    if (s->ops->isArray) return (Array*)s->value.db;
    if (nil_ok && s->value.db == &nilDB) return NULL;
  }
#else
  Symbol* ref = YETI_DEREFERENCE_SYMBOL(s);
  if (ref->ops == &dataBlockSym) {
    DataBlock* db = ref->value.db;
    if (db->ops->isArray) {
      if (ref != s) {
        /* Replace reference onto the stack (equivalent to the statement
           ReplaceRef(stack); see ydata.c for actual code of this routine). */
        s->value.db = Ref(db);
        s->ops = &dataBlockSym;     /* change ops only AFTER value updated */
      }
      return (Array*)db;
    }
    if (nil_ok && db == &nilDB) return NULL;
  }
#endif
  YError("unexpected non-array argument");
  return NULL; /* avoid compiler warning */
}

/*---------------------------------------------------------------------------*/
/* STACK MANAGEMENT */

/* Pop topmost stack element in-place of S and drop all elements above S. */
void yeti_pop_and_reduce_to(Symbol* s)
{
  if (s < sp) {
    DataBlock* old = s->ops == &dataBlockSym? s->value.db : 0;
    Symbol* stack = sp--;
    s->value = stack->value;
    s->ops = stack->ops;
    Unref(old); /* old unreferenced AFTER stack element is replaced */
    while (sp > s) {
      stack = sp--; /* sp decremented BEFORE stack element is deleted */
      if (stack->ops == &dataBlockSym) {
        DataBlock* db = stack->value.db;
        Unref(db);
      }
    }
  } else if (s > sp) {
    YError("attempt to pop outside the stack");
  }
}

/*---------------------------------------------------------------------------*/
/* PUSH A SCALAR ON TOP OF THE STACK */

void yor_push_char_value(char val)
{
  YOR_PUSH_NEW_ARRAY(char, NULL)[0] = val;
}

void yor_push_short_value(short val)
{
  YOR_PUSH_NEW_ARRAY(short, NULL)[0] = val;
}

void yor_push_float_value(float val)
{
  YOR_PUSH_NEW_ARRAY(float, NULL)[0] = val;
}

void yor_push_complex_value(yor_complex_t val)
{
  YOR_PUSH_NEW_ARRAY(yor_complex_t, NULL)[0] = val;
}

void yor_push_string_value(const char* str)
{
  YOR_PUSH_NEW_ARRAY(yor_string_t, NULL)[0] =
    (str != NULL ? p_strcpy((char*)str) : NULL);
}

/*---------------------------------------------------------------------------*/
/* GET A SCALAR FROM THE STACK */

void** yeti_get_pointer(Symbol* s)
{
  Operand op;
  if (!s->ops) unexpected_keyword_argument();
  s->ops->FormOperand(s, &op);
  if (op.ops->typeID != T_POINTER || op.type.dims)
    YError("expecting scalar pointer argument");
  return *(void**)op.value;
}

/*---------------------------------------------------------------------------*/
/* ERROR MANAGEMENT */

#define MSG_MAX_LEN 127
void yeti_error(const char* str, ...)
{
  unsigned int msglen = 0;
  char msg[MSG_MAX_LEN+1];
  va_list ap;

  va_start(ap, str);
  while (str) {
    unsigned int len = strlen(str);
    if (len + msglen > MSG_MAX_LEN) len = MSG_MAX_LEN - msglen;
    if (len > 0) memcpy(msg+msglen, str, len);
    msglen += len;
    str = va_arg(ap, char*);
  }
  va_end(ap);
  msg[msglen] = 0;
  YError(msg);
}
#undef MSG_MAX_LEN

/*---------------------------------------------------------------------------*/
/* DIMENSIONS OF ARRAYS */

void yeti_reset_dimlist(void)
{
  /* tmpDims is a global temporary for Dimension lists under construction
     -- you should always use it, then just leave your garbage there when
     you are done for the next guy to clean up -- your part of the
     perpetual cleanup comes first. */
  Dimension* dims = tmpDims;
  tmpDims = NULL;
  if (dims) FreeDimension(dims);
}

Dimension* yeti_start_dimlist(long number)
{
  Dimension* dims = tmpDims;
  tmpDims = NULL;
  if (dims) FreeDimension(dims);
  return tmpDims = NewDimension(number, 1L, NULL);
}

Dimension* yeti_grow_dimlist(long number)
{
  return tmpDims = NewDimension(number, 1L, tmpDims);
}

static void not_same_dims(void);

int yeti_same_dims(const Dimension* dims1,
                   const Dimension* dims2)
{
  while (dims1 != dims2) {
    if (dims1 == NULL || dims2 == NULL || dims1->number != dims2->number) {
      return 0;
    }
    dims1 = dims1->next;
    dims2 = dims2->next;
  }
  return 1;
}

void yeti_assert_same_dims(const Dimension* dims1,
                           const Dimension* dims2)
{
  while (dims1 != dims2) {
    if (dims1 == NULL || dims2 == NULL || dims1->number != dims2->number) {
      not_same_dims();
    }
    dims1 = dims1->next;
    dims2 = dims2->next;
  }
}

long yeti_total_number(const Dimension* dims)
{
  long number = 1;
  while (dims != NULL) {
    number *= dims->number;
    dims = dims->next;
  }
  return number;
}

long yeti_total_number_2(const Dimension* dims1,
                         const Dimension* dims2)
{
  long number = 1;
  while (dims1 != NULL) {
    if (dims1 == NULL || dims2 == NULL || dims1->number != dims2->number) {
      not_same_dims();
    }
    number *= dims1->number;
    dims1 = dims1->next;
    dims2 = dims2->next;
  }
  if (dims2 != NULL) {
    not_same_dims();
  }
  return number;
}

static void not_same_dims(void)
{
  YError("input arrays must have same dimensions");
}

Dimension* yeti_make_dims(const long number[], const long origin[],
                          size_t ndims)
{
  Dimension* dims = tmpDims;
  tmpDims = NULL;
  if (dims != NULL) {
    FreeDimension(dims);
  }
  if (origin != NULL) {
    for (size_t i = 0; i < ndims; ++i) {
      tmpDims = NewDimension(number[i], origin[i], tmpDims);
    }
  } else {
    for (size_t i = 0; i < ndims; ++i) {
      tmpDims = NewDimension(number[i], 1L, tmpDims);
    }
  }
  return tmpDims;
}

size_t yeti_get_dims(const Dimension* dims, long number[], long origin[],
                     size_t maxdims)
{
  size_t ndims = 0;
  for (const Dimension* ptr = dims; ptr != NULL ; ptr = ptr->next) {
    ++ndims;
  }
  if (ndims > maxdims) {
    YError("too many dimensions");
  }
  size_t i = ndims;
  if (origin != NULL) {
    while (i-- >= 1) {
      number[i] = dims->number;
      origin[i] = dims->origin;
      dims = dims->next;
    }
  } else {
    while (i-- >= 1) {
      number[i] = dims->number;
      dims = dims->next;
    }
  }
  return ndims;
}

/*---------------------------------------------------------------------------*/
/* OPAQUE OBJECTS */

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
extern MemberOp GetMemberX;

static void free_opaque(void* addr);
static UnaryOp print_opaque;
static void bad_opaque_type(const yeti_opaque_type_t* type);

static Operations opaque_ops = {
  &free_opaque, T_OPAQUE, 0, T_STRING, "opaque_object",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &EvalX, &SetupX, &GetMemberX, &MatMultX, &print_opaque
};

/* See `defmem.h' for explanations about meaning and usage of MemoryBlock
   structure. */
static MemryBlock obj_block = {NULL, NULL, sizeof(yeti_opaque_t),
                               64*sizeof(yeti_opaque_t)};

yeti_opaque_t* yeti_new_opaque(void* data, const yeti_opaque_type_t* type)
{
  yeti_opaque_t* obj = NextUnit(&obj_block);
  obj->references = 0;
  obj->ops = &opaque_ops;
  obj->type = type;
  obj->data = data;
  return obj;
}

/* free_opaque is automatically called by Yorick to delete an opaque object
   instance that is no longer referenced */
static void free_opaque(void* addr)
{
  yeti_opaque_t* obj = addr;
  if (obj->type && obj->type->delete) obj->type->delete(obj->data);
  FreeUnit(&obj_block, addr);
}

/* print_opaque is used by Yorick's info command */
static void print_opaque(Operand* op)
{
  yeti_opaque_t* obj = (yeti_opaque_t*)op->value;

  if (obj->type->print) {
    obj->type->print(obj->data);
  } else {
    char line[80];
    ForceNewline();
    PrintFunc("Opaque object instance of type: ");
    PrintFunc(obj->type && obj->type->name ? obj->type->name : "<UNKNOWN>");
    sprintf(line, ", references = %d", obj->references);
    PrintFunc(line);
    ForceNewline();
  }
}

yeti_opaque_t* yeti_get_opaque(Symbol* stack, const yeti_opaque_type_t* type,
                               int fatal)
{
  yeti_opaque_t* obj;
  Symbol* s = (stack->ops == &referenceSym) ? &globTab[stack->index] : stack;
  if (s->ops != &dataBlockSym || s->value.db->ops != &opaque_ops) {
    if (fatal) YError("not an opaque object");
    return NULL;
  }
  obj = (yeti_opaque_t*)s->value.db;
  if (type && obj->type != type) {
    if (fatal) bad_opaque_type(type);
    return NULL;
  }
  if (s != stack) {
    /* Replace reference onto the stack (equivalent to the statement
       ReplaceRef(stack); see ydata.c for actual code of this routine). */
    stack->value.db = Ref(s->value.db);
    stack->ops = &dataBlockSym;     /* change ops only AFTER value updated */
  }
  return obj;
}

static void bad_opaque_type(const yeti_opaque_type_t* type)
{
#undef MAXLEN
#define MAXLEN 40
  char msg[(60+MAXLEN)];
  int len;

  strcpy(msg, "bad object (not instance of ");
  if (type == NULL || type->name == NULL) {
    strcat(msg, "<UNKNOWN>");
  } else if ((len = strlen(type->name)) > MAXLEN) {
    strncat(msg, type->name, (len-MAXLEN));
    strcat(msg, "[...]");
  } else {
    strcat(msg, type->name);
  }
  strcat(msg, " type)");
  YError(msg);
}

/*---------------------------------------------------------------------------*/
/* WORKSPACE */

typedef struct ws ws_t;
struct ws {
  /* Common part of all Yorick's DataBlocks: */
  int  references; /* reference counter */
  Operations* ops; /* virtual function table */
};

static void FreeWS(void* addr);
static UnaryOp PrintWS;

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
extern MemberOp GetMemberX;

Operations wsOps = {
  &FreeWS, T_OPAQUE, 0, /* promoteID = */ T_STRING /* means illegal */,
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
static void FreeWS(void* addr)
{
  p_free(addr);
}

/* PrintRE is used by Yorick's info command. */
static void PrintWS(Operand* op)
{
  ForceNewline();
  PrintFunc("object of type: workspace");
  ForceNewline();
}

void* yeti_push_workspace(size_t nbytes)
{
  /* EXTRA is the number of bytes needed to store DataBlock header rounded
     up to the size of a double (to avoid alignment errors). */
  const size_t extra = YOR_ROUND_UP(sizeof(ws_t), sizeof(double));
  ws_t* ws = p_malloc(nbytes + extra);
  ws->references = 0;
  ws->ops = &wsOps;
  return (void*)((char*)PushDataBlock(ws) + extra);
}
