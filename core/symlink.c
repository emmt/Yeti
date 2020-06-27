/*
 * symlink.c -
 *
 * Implement symbolic links for Yorick.
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

#include <string.h>

#include "plugin.h"
#include "ydata.h"
#include "yio.h"
#include "defmem.h"
#include "pstdlib.h"

/* Debug level: 0 or undefined = none,
 *              1 = perform assertions,
 *              2 = verbose debug.
 */
#define YETI_SYMLINK_DEBUG 0

extern BuiltIn Y_symlink_to_name, Y_symlink_to_variable;
extern BuiltIn Y_is_symlink;
extern BuiltIn Y_name_of_symlink, Y_value_of_symlink;

extern DataBlock *ForceToDB(Symbol *s);

/* Implement symbolic links as a foreign Yorick data type.  */
typedef struct _symlink symlink_t;
struct _symlink {
  int references;      /* reference counter */
  Operations *ops;     /* virtual function table */
  long index;          /* index into global symbol table */
};

static symlink_t *new_symlink(long index);
static void free_symlink(void *list);  /* ******* Use Unref(list) ******* */
static void dereference_symlink(Operand *op);

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
  &free_symlink, T_OPAQUE, 0, T_STRING, "symlink",
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

static symlink_t *new_symlink(long index)
{
  symlink_t *lnk = NextUnit(&symlink_block);
  lnk->references = 0;
  lnk->ops = &symlink_ops;
  lnk->index = index;
  return lnk;
}

static void free_symlink(void *addr)  /* ******* Use Unref(lnk) ******* */
{
  /* symlink_t *lnk = addr; */
  FreeUnit(&symlink_block, addr);
}

static void print_symlink(Operand *op)
{
  symlink_t *lnk = op->value;
  char *name = globalTable.names[lnk->index];
  ForceNewline();
  PrintFunc("symbolic link to \"");
  PrintFunc(name);
  PrintFunc("\"");
  ForceNewline();
}

static void dereference_symlink(Operand *op)
{
  Symbol *s, *owner;
  symlink_t *lnk;
  DataBlock *db;

  /* Replace owner by the globTab symbol which is indexed by the symbolic
     link object. */
  owner = op->owner;
#if defined(YETI_SYMLINK_DEBUG) && YETI_SYMLINK_DEBUG >= 1
  if (! owner || (owner - sp) > 0 || (owner - spBottom) < 0) {
    /* owner should be on stack if this called from Eval or Print */
    YError("symbolic link object evaluated in illegal situation");
  }
#endif
  lnk = op->value;
  s = &globTab[lnk->index];
  if (owner->ops == &dataBlockSym) {
    /* always take this branch, and Unref usually does free_symlink */
    owner->ops = &intScalar;
    Unref(owner->value.db);
#if defined(YETI_SYMLINK_DEBUG) && YETI_SYMLINK_DEBUG >= 1
  } else {
    YError("assertion failed for symbolic link object");
#endif
  }
  if (s->ops == &dataBlockSym) {
    db = s->value.db;
    if (db->ops == &symlink_ops) {
      YError("illegal symbolic link to symbolic link");
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

static void eval_symlink(Operand *op)
{
  dereference_symlink(op);
  op->ops->Eval(op);
}

static void get_symlink_member(Operand *op, char *name)
{
  dereference_symlink(op);
  op->ops->GetMember(op, name);
}

void Y_symlink_to_variable(int nargs)
{
  if (nargs != 1) {
    YError("symlink_to_variable takes exactly one argument");
  }
  if (sp->ops != &referenceSym) {
    YError("expecting simple variable reference");
  }
  PushDataBlock(new_symlink(sp->index));
}

void Y_symlink_to_name(int nargs)
{
  Operand op;
  const char *name;
  int i, c;

  if (nargs != 1) {
    YError("symlink_to_name takes exactly one argument");
  }
  if (! sp->ops) {
    YError("unexpected keyword argument");
  }
  sp->ops->FormOperand(sp, &op);
  if (op.ops->typeID != T_STRING || op.type.dims) {
    YError("expecting scalar string argument");
  }
  name = *(char **)op.value;
  i = -1;
  if (name) {
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
    YError("invalid symbol name");
  }
  PushDataBlock(new_symlink(Globalize(name, i)));
}

void Y_is_symlink(int nargs)
{
  Symbol *s;
  int result;

  if (nargs != 1) YError("is_symlink takes exactly one argument");
  s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  result = (s->ops == &dataBlockSym && s->value.db->ops == &symlink_ops);
  PushIntValue(result);
}

void Y_name_of_symlink(int nargs)
{
  Symbol *s;
  char *name;
  symlink_t *lnk;
  Array *array;

  if (nargs != 1) YError("name_of_symlink takes exactly one argument");
  s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  if (s->ops != &dataBlockSym || s->value.db->ops != &symlink_ops) {
    YError("expecting a symbolic link object");
  }
  lnk = (symlink_t *)s->value.db;
  name = globalTable.names[lnk->index];
  array = (Array *)PushDataBlock(NewArray(&stringStruct, NULL));
  array->value.q[0] = p_strcpy(name);
}

void Y_value_of_symlink(int nargs)
{
  Symbol *s, *stack;
  DataBlock *db;
  symlink_t *lnk;

  if (nargs != 1) YError("value_of_symlink takes exactly one argument");
  s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  if (s->ops != &dataBlockSym || s->value.db->ops != &symlink_ops) {
    YError("expecting a symbolic link object");
  }
  lnk = (symlink_t *)s->value.db;
  s = &globTab[lnk->index];
  if (s->ops == &dataBlockSym) {
    db = s->value.db;
    PushDataBlock(Ref(db));
  } else {
    stack = sp + 1;
    stack->ops = s->ops;
    stack->value = s->value;
    sp = stack;
  }
}
