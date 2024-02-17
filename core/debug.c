/*
 * debug.c -
 *
 * Implement Yorick objects for tracking references.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "yeti.h"
#include "ydata.h"
#include "yio.h"
#include "pstdlib.h"

typedef struct {
  int  references; /* reference counter */
  Operations* ops; /* virtual function table */
  long       mark; /* mark */
} DebugRefs;

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;

static void free_debug_refs(void* addr);
static void extract_debug_refs(Operand* op, char* name);
static void print_debug_refs(Operand* op);

static Operations debug_refs_type = {
  &free_debug_refs, T_OPAQUE, 0, T_STRING/* means illegal promotion */,
  "debug_refs",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &EvalX, &SetupX, &extract_debug_refs, &MatMultX, &print_debug_refs
};

static void
free_debug_refs(void* addr)
{
    DebugRefs* dbg = (DebugRefs*)addr;
    fprintf(stderr, "DEBUG: Freeing `debug_refs` object with mark = %ld\n", dbg->mark);
    p_free(dbg);
}

static void
print_debug_refs(Operand* op)
{
    char buf[64];
    DebugRefs* dbg = (DebugRefs*)op->value;
    ForceNewline();
    PrintFunc("debug_refs (mark = ");
    sprintf(buf, "%ld", (long)dbg->mark);
    PrintFunc(buf);
    PrintFunc(", nrefs = ");
    sprintf(buf, "%ld", (long)dbg->references);
    PrintFunc(buf);
    PrintFunc(")");
    ForceNewline();
}

static void
extract_debug_refs(Operand* op, char* name)
{
    DebugRefs* dbg = op->value;
    long val = -1L;
    if (strcmp(name, "mark") == 0) {
        val = dbg->mark;
    } else if (strcmp(name, "nrefs") == 0) {
        val = dbg->references;
    } else {
        yor_error("bad member");
    }
    /* Replace top of stack by value. */
    DataBlock* old = (sp->ops == &dataBlockSym) ? sp->value.db : NULL;
    sp->value.l = val;
    sp->ops = &longScalar;
    YOR_UNREF(old);
}

void Y_debug_refs(int argc)
{
    static long mark = 0;
    if (argc != 1 || sp->ops != &dataBlockSym || sp->value.db != &nilDB) {
        yor_error("usage: debug_refs()");
    }
    size_t size = sizeof(DebugRefs);
    DebugRefs* dbg = p_malloc(size);
    memset(dbg, 0, size);
    dbg->ops = &debug_refs_type;
    dbg->mark = ++mark;
    PushDataBlock(dbg);
}
