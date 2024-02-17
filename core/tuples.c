/*
 * tuples.c -
 *
 * Implement tuple-like objects for Yorick.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define USE_OLD_API

#include "yeti.h"
#include "yio.h"
#include "pstdlib.h"

typedef struct Item      Item;
typedef enum   ItemType  ItemType;
typedef struct Tuple     Tuple;

enum ItemType { SCALAR_INT = 0, SCALAR_LONG, SCALAR_DOUBLE, OBJECT };

struct Item {
  union {
    int i;
    long l;
    double d;
    DataBlock* db;
  } value;
  ItemType type;
};

struct Tuple {
  int  references;    /* reference counter */
  Operations* ops;    /* virtual function table */
  long       numb;    /* number of objects in tuple */
  Item      items[1]; /* contents */
};

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;

static void free_tuple(void* addr);
static void eval_tuple(Operand* op);
static void extract_tuple(Operand* op, char* name);
static void print_tuple(Operand* op);

static Operations tuple_type = {
  &free_tuple, YOR_OPAQUE, 0, YOR_STRING/* means illegal promotion */,
  "tuple",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &eval_tuple, &SetupX, &extract_tuple, &MatMultX, &print_tuple
};

static void
free_tuple(void* addr)
{
    Tuple* tuple = (Tuple*)addr;
    for (long i = 0; i < tuple->numb; ++i) {
        Item* item = &tuple->items[i];
        if (item->type == OBJECT) {
            YOR_UNREF(item->value.db);
        }
    }
    p_free(tuple);
}

static void
print_tuple(Operand* op)
{
    char buf[32];
    Tuple* tuple = (Tuple*)op->value;
    sprintf(buf, "%ld", tuple->numb);
    ForceNewline();
    PrintFunc("tuple (");
    PrintFunc(buf);
    PrintFunc((tuple->numb > 1 ? " elements)" : " element)"));
    ForceNewline();
}

static void
eval_tuple(Operand *op)
{
    /* Check syntax (argc is also in op->references). */
    if (sp != op->owner + 1) {
        yor_error("expecting exactly one argument");
    }

    /* Parse argument. */
    Symbol* arg = (sp->ops == &referenceSym) ? &globTab[sp->index] : sp;
    long index = 0;
    int flag = -1; /* -1 means bad argument, 1 means idex, 0 means nil */
    if (arg->ops == &longScalar) {
        index = arg->value.l;
        flag = 1;
    } else if (arg->ops == &dataBlockSym) {
        if (arg->value.db == &nilDB) {
            flag = 0;
        } else {
            Operand op1;
            arg->ops->FormOperand(arg, &op1);
            if (op1.type.dims == (Dimension*)0) {
                if (op1.ops->typeID == YOR_LONG) {
                    index = *(long*)op1.value;
                    flag = 1;
                } else if (op1.ops->typeID == YOR_INT) {
                    index = *(int*)op1.value;
                    flag = 1;
                } else if (op1.ops->typeID == YOR_SHORT) {
                    index = *(short*)op1.value;
                    flag = 1;
                } else if (op1.ops->typeID == YOR_CHAR) {
                    index = *(char*)op1.value;
                    flag = 1;
                }
            }
        }
    } else if (arg->ops == &intScalar) {
        index = arg->value.i;
        flag = 1;
    }
    if (flag == -1) {
        yor_error("expecting a scalar integer or nil");
    }

    /* Drop argument. */
    DataBlock* old = (sp->ops == &dataBlockSym) ? sp->value.db : NULL;
    --sp;
    YOR_UNREF(old);

    /* Replace called object by result. */
    Tuple* tuple = (Tuple*)op->value;
    old = (sp->ops == &dataBlockSym) ? sp->value.db : NULL;
    if (flag == 1) {
        if (index <= 0) {
            index += tuple->numb;
        }
        if (index < 1 || index > tuple->numb) {
            yor_error("out of range tuple index");
        }
        Item* item = &tuple->items[index - 1];
        if (item->type == SCALAR_INT) {
            sp->value.i = item->value.i;
            sp->ops = &intScalar;
        } else if (item->type == SCALAR_LONG) {
            sp->value.l = item->value.l;
            sp->ops = &longScalar;
        } else if (item->type == SCALAR_DOUBLE) {
            sp->value.d = item->value.d;
            sp->ops = &doubleScalar;
        } else {
            DataBlock* db = item->value.db;
            if (db != NULL) {
                ++db->references;
            }
            sp->value.db = db;
            sp->ops = &dataBlockSym;
        }
    } else {
        sp->value.l = tuple->numb;
        sp->ops = &longScalar;
    }
    YOR_UNREF(old);
}

static void
extract_tuple(Operand* op, char* name)
{
    Tuple* tuple = op->value;
    if (strcmp(name, "number") == 0) {
        /* Replace top of stack by value. */
        DataBlock* old = (sp->ops == &dataBlockSym) ? sp->value.db : NULL;
        sp->value.l = tuple->numb;
        sp->ops = &longScalar;
        YOR_UNREF(old);
    } else {
        yor_error("bad member");
    }
}

void Y_is_tuple(int argc)
{
    if (argc != 1) yor_error("is_tuple() takes exactly one argument");
    Symbol* arg = (sp->ops == &referenceSym) ? &globTab[sp->index] : sp;
    DataBlock* old = (sp->ops == &dataBlockSym) ? sp->value.db : NULL;
    sp->value.i = (arg->ops == &dataBlockSym && arg->value.db != NULL
                   && arg->value.db->ops == &tuple_type);
    sp->ops = &intScalar;
    YOR_UNREF(old);
}

void Y_tuple(int argc)
{
    size_t size = offsetof(Tuple, items) + argc*sizeof(Item);
    Tuple* tup = p_malloc(size);
    memset(tup, 0, size);
    tup->ops = &tuple_type;
    tup->numb = argc;
    PushDataBlock(tup);
    for (int i = 0; i < argc; ++i) {
        Symbol* arg = (sp - argc) + i;
        if (arg->ops == &referenceSym) {
            arg = &globTab[arg->index];
        }
        if (arg->ops == &intScalar) {
            tup->items[i].value.i = arg->value.i;
            tup->items[i].type = SCALAR_INT;
        } else if (arg->ops == &longScalar) {
            tup->items[i].value.l = arg->value.l;
            tup->items[i].type = SCALAR_LONG;
        } else if (arg->ops == &doubleScalar) {
            tup->items[i].value.d = arg->value.d;
            tup->items[i].type = SCALAR_DOUBLE;
        } else if (arg->ops == &dataBlockSym) {
            DataBlock* db = arg->value.db;
            if (db != NULL) {
                ++db->references;
            }
            tup->items[i].value.db = db;
            tup->items[i].type = OBJECT;
        } else if (arg->ops == NULL) {
            yor_unexpected_keyword_argument();
        } else {
            yor_bad_argument(arg);
        }
    }
}
