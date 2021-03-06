/*
 * symlink.c -
 *
 * Implement symbolic links for Yorick.
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

#include "yeti.h"
#include "yio.h"
#include "pstdlib.h"

/* Debug level: 0 or undefined = none,
 *              1 = perform assertions,
 *              2 = verbose debug.
 */
#define YETI_SYMLINK_DEBUG 0

extern BuiltIn Y_symlink_to_name, Y_symlink_to_variable;
extern BuiltIn Y_is_symlink;
extern BuiltIn Y_name_of_symlink, Y_value_of_symlink;

extern DataBlock* ForceToDB(Symbol* s);

/* Implement symbolic links as a foreign Yorick data type.  */
typedef struct _symlink symlink_t;
struct _symlink {
  int references;      /* reference counter */
  Operations* ops;     /* virtual function table */
  long index;          /* index into global symbol table */
};

static symlink_t* new_symlink(long index);
static void free_symlink(void* list);  /* ******* Use Unref(list) ******* */
static void dereference_symlink(Operand* op);

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;

static UnaryOp eval_symlink;
static UnaryOp print_symlink;
static MemberOp get_symlink_member;

Operations symlink_ops = {
  &free_symlink, YOR_OPAQUE, 0, YOR_STRING, "symlink",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &eval_symlink, &SetupX, &get_symlink_member, &MatMultX,
  &print_symlink
};

/* Set up a block allocator which grabs space for 64 symlink_t objects at a
   time. */
static MemryBlock symlink_block= {0, 0, sizeof(symlink_t),
                                  64*sizeof(symlink_t)};

static symlink_t* new_symlink(long index)
{
  symlink_t* lnk = NextUnit(&symlink_block);
  lnk->references = 0;
  lnk->ops = &symlink_ops;
  lnk->index = index;
  return lnk;
}

static void free_symlink(void* addr)  /* ******* Use Unref(lnk) ******* */
{
  /* symlink_t* lnk = addr; */
  FreeUnit(&symlink_block, addr);
}

static void print_symlink(Operand* op)
{
  symlink_t* lnk = op->value;
  char* name = globalTable.names[lnk->index];
  ForceNewline();
  PrintFunc("symbolic link to \"");
  PrintFunc(name);
  PrintFunc("\"");
  ForceNewline();
}

static void dereference_symlink(Operand* op)
{
  /* Replace owner by the globTab symbol which is indexed by the symbolic
     link object. */
  Symbol* owner = op->owner;
#if defined(YETI_SYMLINK_DEBUG) && YETI_SYMLINK_DEBUG >= 1
  if (! owner || (owner - sp) > 0 || (owner - spBottom) < 0) {
    /* owner should be on stack if this called from Eval or Print */
    yor_error("symbolic link object evaluated in illegal situation");
  }
#endif
  symlink_t* lnk = op->value;
  Symbol* s = &globTab[lnk->index];
  if (owner->ops == &dataBlockSym) {
    /* always take this branch, and Unref usually does free_symlink */
    owner->ops = &intScalar;
    Unref(owner->value.db);
#if defined(YETI_SYMLINK_DEBUG) && YETI_SYMLINK_DEBUG >= 1
  } else {
    yor_error("assertion failed for symbolic link object");
#endif
  }
  DataBlock* db;
  if (s->ops == &dataBlockSym) {
    db = s->value.db;
    if (db->ops == &symlink_ops) {
      yor_error("illegal symbolic link to symbolic link");
    }
    owner->value.db = Ref(db);
    owner->ops = s->ops;
  } else {
    owner->value = s->value;
    owner->ops = s->ops;
    db = ForceToDB(owner);
  }
  op->ops = db->ops;
  op->value = db;
}

static void eval_symlink(Operand* op)
{
  dereference_symlink(op);
  op->ops->Eval(op);
}

static void get_symlink_member(Operand* op, char* name)
{
  dereference_symlink(op);
  op->ops->GetMember(op, name);
}

void Y_symlink_to_variable(int argc)
{
  if (argc != 1) {
    yor_error("symlink_to_variable() takes exactly one argument");
  }
  if (sp->ops != &referenceSym) {
    yor_error("expecting simple variable reference");
  }
  PushDataBlock(new_symlink(sp->index));
}

void Y_symlink_to_name(int argc)
{
  if (argc != 1) yor_error("symlink_to_name() takes exactly one argument");
  if (sp->ops == NULL) yor_unexpected_keyword_argument();
  Operand op;
  sp->ops->FormOperand(sp, &op);
  if (op.ops->typeID != YOR_STRING || op.type.dims != NULL) {
    yor_error("expecting scalar string argument");
  }
  const char* name = *(char**)op.value;
  long i = -1;
  if (name != NULL) {
    int c;
    while ((c = name[++i]) != '\0') {
      if ((c < 'a' || c > 'z') &&
          (c < 'A' || c > 'Z') &&
          (c != '_') &&
          (i == 0 || c < '0' || c > '9')) {
        i = -1;
        break;
      }
    }
  }
  if (i <= 0) {
    yor_error("invalid symbol name");
  }
  PushDataBlock(new_symlink(Globalize(name, i)));
}

void Y_is_symlink(int argc)
{
  if (argc != 1) yor_error("is_symlink takes exactly one argument");
  Symbol* s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  int result = (s->ops == &dataBlockSym && s->value.db->ops == &symlink_ops);
  PushIntValue(result);
}

void Y_name_of_symlink(int argc)
{
  if (argc != 1) yor_error("name_of_symlink takes exactly one argument");
  Symbol* s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  if (s->ops != &dataBlockSym || s->value.db->ops != &symlink_ops) {
    yor_error("expecting a symbolic link object");
  }
  symlink_t* lnk = (symlink_t*)s->value.db;
  char* name = globalTable.names[lnk->index];
  Array* array = (Array*)PushDataBlock(NewArray(&stringStruct, NULL));
  array->value.q[0] = p_strcpy(name);
}

void Y_value_of_symlink(int argc)
{
  if (argc != 1) yor_error("value_of_symlink takes exactly one argument");
  Symbol* s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  if (s->ops != &dataBlockSym || s->value.db->ops != &symlink_ops) {
    yor_error("expecting a symbolic link object");
  }
  symlink_t* lnk = (symlink_t*)s->value.db;
  s = &globTab[lnk->index];
  if (s->ops == &dataBlockSym) {
    DataBlock* db = s->value.db;
    PushDataBlock(Ref(db));
  } else {
    Symbol* stack = sp + 1;
    stack->ops = s->ops;
    stack->value = s->value;
    sp = stack;
  }
}
