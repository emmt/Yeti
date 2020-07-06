/*
 * hash.c -
 *
 * Implement hash table objects for Yorick.
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
#include "config.h"
#include "yeti.h"
#include "play.h"
#include "yio.h"

#undef H_DEBUG
#undef H_REMOVE

/*---------------------------------------------------------------------------*/
/* DEFINITIONS FOR STRING HASH TABLES */

/* Some macros to adapt implementation. */
#define h_error(msg)     yor_error(msg)
#define h_malloc(size)   p_malloc(size)
#define h_free(addr)     p_free(addr)

#define OFFSET(type, member) ((char*)&((type*)0)->member - (char*)0)

typedef struct h_table h_table_t;
typedef struct h_entry h_entry_t;

struct h_table {
  int     references; /* reference counter */
  Operations*    ops; /* virtual function table */
  long          eval; /* index to eval method (-1L if none) */
  size_t      number; /* number of entries */
  size_t        size; /* number of elements in bucket */
  size_t    new_size; /* if > size, indicates rehash is needed */
  h_entry_t** bucket; /* dynamically malloc'ed bucket of entries */
};

struct h_entry {
  h_entry_t*       next; /* next entry or NULL */
  OpTable*      sym_ops; /* client data value = Yorick's symbol */
  SymbolValue sym_value;
  size_t           hash; /* hashed key */
  char             name[1];
                         /* entry name, actual size is large enough for whole
                            string name to fit (MUST BE LAST MEMBER) */
};

/*
 * Tests about the hashing method:
 * ---------------------------------------------------------------------------
 * Hashing code         Cost(*)  Histogram of bucket occupation
 * ---------------------------------------------------------------------------
 * HASH+=(HASH<<1)+BYTE   1.38   [1386,545,100,17]
 * HASH+=(HASH<<2)+BYTE   1.42   [1399,522,107,20]
 * HASH+=(HASH<<3)+BYTE   1.43   [1404,511,116,15, 2]
 * HASH =(HASH<<1)^BYTE   1.81   [1434,481, 99,31, 2, 0,0,0,0,0,0,0,0,0,0,0,1]
 * HASH =(HASH<<2)^BYTE   2.09   [1489,401,112,31, 9, 4,1,0,0,0,0,0,0,0,0,0,1]
 * HASH =(HASH<<3)^BYTE   2.82   [1575,310, 95,28,19,10,4,3,2,1,0,0,0,0,0,0,1]
 * ---------------------------------------------------------------------------
 * (*) cost = mean # of tests to localize an item
 * TCL randomize method is:     HASH += (HASH<<3) + BYTE
 * Yorick randomize method is:  HASH  = (HASH<<1) ^ BYTE
 */

/* Piece of code to randomize a string.  HASH and LEN must be
   rvalues (e.g., variables) of integer type. */
#define HASH_STRING(HASH, LEN, STR)                             \
  do {                                                          \
    const unsigned char* __str = (const unsigned char*)(STR);   \
    size_t __byte;                                              \
    size_t __len = 0;                                           \
    size_t __hash = 0;                                          \
    while ((__byte = __str[__len]) != 0) {                      \
      __hash += (__hash << 3) + __byte;                         \
      ++__len;                                                  \
    }                                                           \
    (LEN) = __len;                                              \
    (HASH) = __hash;                                            \
  } while (0)

/* Use this macro to check if hash table ENTRY match string NAME.
   LEN is the length of NAME and HASH the hash value computed from NAME. */
#define H_MATCH(ENTRY, HASH, NAME, LEN) \
  ((ENTRY)->hash == HASH && strncmp(NAME, (ENTRY)->name, LEN) == 0)

static h_table_t* h_new(size_t number);
/*----- Create a new empty hash table with at least NUMBER slots
        pre-allocated (rounded up to a power of 2). */

static void h_delete(h_table_t* table);
/*----- Destroy hash table TABLE and its contents. */

static h_entry_t* h_find(h_table_t* table, const char* name);
/*----- Returns the address of the entry in hash table TABLE that match NAME.
        If no entry is identified by NAME (or in case of error) NULL is
        returned. */

#ifdef H_REMOVE
static int h_remove(h_table_t* table, const char* name);
/*----- Remove entry identifed by NAME from hash table TABLE.  Return value
        is: 0 if no entry in TABLE match NAME, 1 if and entry matching NAME
        was found and unreferenced, -1 in case of error. */
#endif

static int h_insert(h_table_t* table, const char* name, Symbol* sym);
/*----- Insert entry identifed by NAME with contents SYM in hash table
        TABLE.  Return value is: 0 if no former entry in TABLE matched NAME
        (hence a new entry was created); 1 if a former entry in TABLE matched
        NAME (which was properly unreferenced); -1 in case of error. */

/*---------------------------------------------------------------------------*/
/* PRIVATE ROUTINES */

extern BuiltIn Y_is_hash;
extern BuiltIn Y_h_new, Y_h_get, Y_h_set, Y_h_has, Y_h_pop, Y_h_stat;
extern BuiltIn Y_h_debug, Y_h_keys, Y_h_first, Y_h_next;

static h_table_t* get_table(Symbol* stack);
/*----- Returns hash table stored by symbol STACK.  STACK get replaced by
        the referenced object if it is a reference symbol. */

static void set_members(h_table_t* obj, Symbol* stack, int nargs);
/*----- Parse arguments STACK[0]..STACK[NARGS-1] as key-value pairs to
        store in hash table OBJ. */

static int get_table_and_key(int nargs, h_table_t** table,
                            const char** keystr);

static void get_member(Symbol* owner, h_table_t* table, const char* name);
/*----- Replace stack symbol OWNER by the contents of entry matching NAME
        in hash TABLE (taking care of UnRef/Ref properly). */

static int rehash(h_table_t* table);
/*----- Rehash hash TABLE (taking care of interrupts). */

/*--------------------------------------------------------------------------*/
/* IMPLEMENTATION OF HASH TABLES AS OPAQUE YORICK OBJECTS */

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
static MemberOp GetMemberH;
static UnaryOp PrintH;
static void FreeH(void* addr);  /* ******* Use Unref(hash) ******* */
static void EvalH(Operand* op);

static Operations hashOps = {
  &FreeH, YOR_OPAQUE, 0, /* promoteID = */YOR_STRING/* means illegal */,
  "hash_table",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &EvalH, &SetupX, &GetMemberH, &MatMultX, &PrintH
};

/* FreeH is automatically called by Yorick to delete an object instance
   that is no longer referenced. */
static void FreeH(void* addr) { h_delete((h_table_t*)addr); }

/* PrintH is used by Yorick's info command. */
static void PrintH(Operand* op)
{
  h_table_t* obj = (h_table_t*)op->value;
  char line[80];
  ForceNewline();
  PrintFunc("Object of type: ");
  PrintFunc(obj->ops->typeName);
  PrintFunc(" (evaluator=");
  if (obj->eval < 0L) {
    PrintFunc("(nil)");
  } else {
    PrintFunc("\"");
    PrintFunc(globalTable.names[obj->eval]);
    PrintFunc("\"");
  }
  sprintf(line, ", references=%d, number=%zu, size=%zu)",
          obj->references, obj->number, obj->size);
  PrintFunc(line);
  ForceNewline();
}

/* GetMemberH implements the de-referencing '.' operator. */
static void GetMemberH(Operand* op, char* name)
{
  get_member(op->owner, (h_table_t*)op->value, name);
}

/* EvalH implements hash table used as a function or as an indexed array. */
static void EvalH(Operand* op)
{
  /* Get the hash table. */
  Symbol* owner = op->owner;
  h_table_t* table = (h_table_t*)owner->value.db;
  int nargs = sp - owner; /* number of arguments */

  if (table->eval >= 0L) {
    /* This hash table implement its own eval method. */
    Symbol* s = &globTab[table->eval];
    YETI_SOLVE_REFERENCE(s);
    Operations* oper;
    DataBlock* db = s->value.db; /* correctness checked below */
    if (s->ops != &dataBlockSym || db == NULL
        || ((oper = db->ops) != &functionOps &&
            oper != &builtinOps && oper != &auto_ops)) {
      h_error("non-function eval method");
    }

    /* Permute the stack to prepend reference to eval method.  First, make sure
       the stack is large enough.  Second, check that there are no pending
       signals before this critical operation.  Finally permute the stack. */
    long offset = owner - spBottom; /* stack may move */
    if (CheckStack(2)) {
      owner = spBottom + offset;
      op->owner = owner;
    }
    if (p_signalling) {
        p_abort();
    }
    /*** CRITICAL CODE BEGIN ***/ {
      Symbol* stack = owner;
      ++nargs; /* one more argument: the object itself */
      int i = nargs;
      stack[i].ops = &intScalar; /* set safe OpTable */
      sp = stack + i; /* it is now safe to grow the stack */
      while (--i >= 0) {
        OpTable* ops = stack[i].ops;
        stack[i].ops = &intScalar; /* set safe OpTable */
        stack[i + 1].value = stack[i].value;
        stack[i + 1].index = stack[i].index;
        stack[i + 1].ops = ops; /* set true OpTable *after* initialization */
      }
      stack->value.db = RefNC(db); /* we already know that db != NULL */
      stack->ops = &dataBlockSym;
    } /*** CRITICAL CODE END ***/

    /* Re-form operand and call Eval method. */
    op->owner = owner; /* stack may have moved */
    op->references = nargs; /* (see FormEvalOp in array.c) */
    op->ops = db->ops;
    op->value = db;
    op->ops->Eval(op);
    return;
  }

  /* If we exactly get one non-keyword argument, extract the key. */
  if (nargs == 1 && sp->ops != NULL) {
    Operand arg;
    sp->ops->FormOperand(sp, &arg);
    if (arg.ops->typeID == YOR_STRING) {
      if (arg.type.dims == NULL) {
        const char* name = *(char**)arg.value;
        h_entry_t* entry = h_find(table, name);
        Drop(1); /* discard key name (after using it) */
        DataBlock* old = (owner->ops == &dataBlockSym) ? owner->value.db : NULL;
        OpTable* ops;
        owner->ops = &intScalar; /* avoid clash in case of interrupts */
        if (entry != NULL) {
          if ((ops = entry->sym_ops) == &dataBlockSym) {
            DataBlock* db = entry->sym_value.db;
            owner->value.db = Ref(db);
          } else {
            owner->value = entry->sym_value;
          }
        } else {
          /* NULLER_DATA_BLOCK NewRange(0L, 0L, 1L, R_NULLER); */
          owner->value.db = RefNC(&nilDB);
          ops = &dataBlockSym;
        }
        Unref(old);
        owner->ops = ops; /* change ops only AFTER value updated */
        return;
      }
    } else if (arg.ops->typeID == YOR_VOID) {
      Drop(2);
      PushLongValue(table->number);
      return;
    }
  }
  h_error("expecting or a single hash key name or nil (integer indexing no longer supported)");
}

/*---------------------------------------------------------------------------*/
/* BUILTIN ROUTINES */

static int is_nil(Symbol* s);
static void push_string_value(const char* value);

static int is_nil(Symbol* s)
{
  YETI_SOLVE_REFERENCE(s);
  return (s->ops == &dataBlockSym && s->value.db == &nilDB);
}

static void push_string_value(const char* value)
{
  ((Array*)PushDataBlock(NewArray(&stringStruct, NULL)))->value.q[0] =
    (value ? p_strcpy((char*)value) : NULL);
}

void Y_is_hash(int nargs)
{
  if (nargs != 1) h_error("is_hash takes exactly one argument");
  int result;
  Symbol* s = YETI_DEREFERENCE_SYMBOL(sp);
  if (s->ops == &dataBlockSym && s->value.db->ops == &hashOps) {
    if (((h_table_t*)s->value.db)->eval >= 0L) {
      result = 2;
    } else {
      result = 1;
    }
  } else {
    result = 0;
  }
  PushIntValue(result);
}

void Y_h_debug(int nargs)
{
  for (int i = 1; i <= nargs; ++i) {
    yor_debug_symbol(sp - nargs + i);
  }
  Drop(nargs);
}

void Y_h_new(int nargs)
{
  Symbol* stack = sp - nargs + 1; /* first argument (we know that the stack
                                     will NOT be moved) */
  int got_members;
  size_t initial_size;
  if (nargs == 0 || (nargs == 1 && is_nil(sp))) {
    got_members = 0;
    initial_size = 0;
  } else {
    got_members = 1;
    initial_size = nargs/2;
  }
  const size_t min_size = 16;
  if (initial_size < min_size) initial_size = min_size;
  h_table_t* obj = h_new(initial_size);
  PushDataBlock(obj);
  if (got_members) set_members(obj, stack, nargs);
}

void Y_h_set(int nargs)
{
  if (nargs < 1 || nargs%2 != 1)
    h_error("usage: h_set,table,\"key\",value,... -or- h_set,table,key=value,...");
  h_table_t* table = get_table(sp - nargs + 1);
  if (nargs > 1) {
    set_members(table, sp - nargs + 2, nargs - 1);
    Drop(nargs-1); /* just left the target object on top of the stack */
  }
}

void Y_h_get(int nargs)
{
  /* Get hash table object and key name, then replace first argument (the
     hash table object) by entry contents. */
  h_table_t* table;
  const char* name;
  if (get_table_and_key(nargs, &table, &name)) {
    h_error("usage: h_get(table, \"key\") -or- h_get(table, key=)");
  }
  Drop(nargs - 1);             /* only left hash table on top of stack */
  get_member(sp, table, name); /* replace top of stack by entry contents */
}

void Y_h_has(int nargs)
{
  h_table_t* table;
  const char* name;
  if (get_table_and_key(nargs, &table, &name)) {
    h_error("usage: h_has(table, \"key\") -or- h_has(table, key=)");
  }
  int result = (h_find(table, name) != NULL);
  Drop(nargs);
  PushIntValue(result);
}

void Y_h_pop(int nargs)
{
  /* Parse arguments. */
  h_table_t* table;
  const char* name;
  if (get_table_and_key(nargs, &table, &name)) {
    h_error("usage: h_pop(table, \"key\") -or- h_pop(table, key=)");
  }

  /* Ensure that there are no pending signals before critical operation. */
  if (p_signalling) {
    p_abort();
  }

  /* *** Code more or less stolen from 'h_remove' *** */

  if (name != NULL) {
    /* Hash key. */
    size_t hash, len;
    HASH_STRING(hash, len, name);

    /* Find the entry. */
    h_entry_t* prev = NULL;
    size_t index = (hash % table->size);
    h_entry_t* entry = table->bucket[index];
    while (entry != NULL) {
      if (H_MATCH(entry, hash, name, len)) {
        /* Delete the entry: (1) remove entry from chained list of entries in
           its bucket, (2) pop contents of entry, (3) free entry memory. */
        /*** CRITICAL CODE BEGIN ***/ {
          if (prev) prev->next = entry->next;
          else table->bucket[index] = entry->next;
          Symbol* stack = sp + 1; /* location to put new element */
          stack->ops   = entry->sym_ops;
          stack->value = entry->sym_value;
          h_free(entry);
          --table->number;
          sp = stack; /* sp updated AFTER new stack element finalized */
        } /*** CRITICAL CODE END ***/
        return; /* entry found and popped */
      }
      prev = entry;
      entry = entry->next;
    }
  }
  PushDataBlock(RefNC(&nilDB)); /* entry not found */
}

void Y_h_number(int nargs)
{
  if (nargs != 1) h_error("h_number takes exactly one argument");
  Symbol* s = YETI_DEREFERENCE_SYMBOL(sp);
  if (s->ops != &dataBlockSym || s->value.db->ops != &hashOps) {
    h_error("inexpected non-hash table argument");
  }
  long result = ((h_table_t*)s->value.db)->number;
  PushLongValue(result);
}

void Y_h_keys(int nargs)
{
  if (nargs != 1) h_error("h_keys takes exactly one argument");
  h_table_t* table = get_table(sp);
  size_t number = table->number;
  if (number > 0) {
    char** result = YOR_PUSH_NEW_ARRAY(char*, yor_start_dimlist(number));
    size_t j = 0;
    for (size_t i = 0; i < table->size; ++i) {
      for (h_entry_t* entry = table->bucket[i];
           entry != NULL; entry = entry->next) {
        if (j >= number) h_error("corrupted hash table");
        result[j++] = p_strcpy(entry->name);
      }
    }
  } else {
    PushDataBlock(RefNC(&nilDB));
  }
}

void Y_h_first(int nargs)
{
  if (nargs != 1) h_error("h_first takes exactly one argument");
  h_table_t* table = get_table(sp);
  char* name = NULL;
  h_entry_t** bucket = table->bucket;
  size_t n = table->size;
  for (size_t j = 0; j < n; ++j) {
    if (bucket[j] != NULL) {
      name = bucket[j]->name;
      break;
    }
  }
  push_string_value(name);
}

void Y_h_next(int nargs)
{
  if (nargs != 2) h_error("h_next takes exactly two arguments");
  h_table_t* table = get_table(sp - 1);

  /* Get scalar string argument. */
  if (sp->ops == NULL) {
  bad_arg:
    h_error("expecting a scalar string");
  }
  Operand arg;
  sp->ops->FormOperand(sp, &arg);
  if (arg.type.dims != NULL || arg.ops->typeID != YOR_STRING) {
    goto bad_arg;
  }
  const char* name = *(char**)arg.value;
  if (name == NULL) {
    /* Left nil string as result on top of stack. */
    return;
  }

  /* Hash key. */
  size_t hash, len;
  HASH_STRING(hash, len, name);

  /* Locate matching entry. */
  size_t j = (hash % table->size);
  h_entry_t** bucket = table->bucket;
  for (h_entry_t* entry = bucket[j]; entry != NULL; entry = entry->next) {
    if (H_MATCH(entry, hash, name, len)) {
      /* Get 'next' hash entry. */
      const char* next_name;
      if (entry->next != NULL) {
        next_name = entry->next->name;
      } else {
        next_name = NULL;
        size_t n = table->size;
        while (++j < n) {
          entry = bucket[j];
          if (entry) {
            next_name = entry->name;
            break;
          }
        }
      }
      push_string_value(next_name);
      return;
    }
  }
  h_error("hash entry not found");
}

void Y_h_stat(int nargs)
{
  if (nargs != 1) h_error("h_stat takes exactly one argument");
  h_table_t* table = get_table(sp);
  size_t number = table->number;
  h_entry_t** bucket = table->bucket;
  long* result = YOR_PUSH_NEW_ARRAY(long, yor_start_dimlist(number + 1));
  for (size_t i = 0; i <= number; ++i) {
    result[i] = 0L;
  }
  size_t max_count = 0;
  size_t sum_count = 0;
  size_t size = table->size;
  for (size_t i = 0; i < size; ++i) {
    size_t count = 0;
    for (h_entry_t* entry = bucket[i]; entry != NULL; entry = entry->next) {
      ++count;
    }
    if (count <= number) {
      ++result[count];
    }
    if (count > max_count) {
      max_count = count;
    }
    sum_count += count;
  }
  if (sum_count != number) {
    table->number = sum_count;
    h_error("corrupted hash table");
  }
}

#if YETI_MUST_DEFINE_AUTOLOAD_TYPE
typedef struct autoload_t autoload_t;
struct autoload_t {
  int   references; /* reference counter */
  Operations*  ops; /* virtual function table */
  long       ifile; /* index into table of autoload files */
  long     isymbol; /* global symtab index */
  autoload_t* next; /* linked list for each ifile */
};
#endif /* YETI_MUST_DEFINE_AUTOLOAD_TYPE */

void Y_h_evaluator(int nargs)
{
  /* Initialization of internals (digits must have lowest values). */
  static long default_eval_index = -1; /* index of default eval method in
                                          globTab */
  static unsigned char type[256];      /* array of integers to check
                                          consistency of a symbol's name */
  if (default_eval_index < 0L) {
    int i;
    unsigned char value = 0;
    for (i = 0; i < 256; ++i) {
      type[i] = value;
    }
    for (i = '0'; i <= '9'; ++i) {
      type[i] = ++value;
    }
    for (i = 'A'; i <= 'Z'; ++i) {
      type[i] = ++value;
    }
    type['_'] = ++value;
    for (i = 'a'; i <= 'z'; ++i) {
      type[i] = ++value;
    }
    default_eval_index = Globalize("*hash_evaluator*", 0L);
  }

  if (nargs < 1 || nargs > 2) h_error("h_evaluator takes 1 or 2 arguments");
  int push_result =  ! yarg_subroutine();
  h_table_t* table = get_table(sp - nargs + 1);
  long old_index = table->eval;

  if (nargs == 2) {
    long new_index = -1L;
    Symbol* s = sp;
    YETI_SOLVE_REFERENCE(s);
    if (s->ops == &dataBlockSym) {
      Operations* ops = s->value.db->ops;
      if (ops == &functionOps) {
        new_index = ((Function*)s->value.db)->code[0].index;
      } else if (ops == &builtinOps) {
        new_index = ((BIFunction*)s->value.db)->index;
      } else if (ops == &auto_ops) {
        new_index = ((autoload_t*)s->value.db)->isymbol;
      } else if (ops == &stringOps) {
        Array* a = (Array*)s->value.db;
        if (a->type.dims == NULL) {
          /* got a scalar string */
          unsigned char* q = (unsigned char*)a->value.q[0];
          if (q == NULL) {
            /* nil symbol's name corresponds to default value */
            new_index = default_eval_index;
          } else {
            /* symbol's name must not have a zero length, nor start with
               an invalid character nor a digit */
            if (type[q[0]] > 10) {
              int c, i = 0;
              for (;;) {
                if ((c = q[++i]) == 0) {
                  new_index = Globalize((char*)q, i);
                  break;
                }
                if (! type[c]) {
                  /* symbol's must not contain an invalid character */
                  break;
                }
              }
            }
          }
        }
      } else if (ops == &voidOps) {
        /* void symbol corresponds to default value */
        new_index = default_eval_index;
      }
    }
    if (new_index < 0L) {
      h_error("evaluator must be a function or a valid symbol's name");
    }
    if (new_index == default_eval_index) {
      table->eval = -1L;
    } else {
      table->eval = new_index;
    }
  }
  if (push_result) {
    char* str = (old_index >= 0L && old_index != default_eval_index) ?
      globalTable.names[old_index] : NULL;
    push_string_value(str);
  }
}

/*---------------------------------------------------------------------------*/

static void get_member(Symbol* owner, h_table_t* table, const char* name)
{
  h_entry_t* entry = h_find(table, name);
  DataBlock* old = (owner->ops == &dataBlockSym) ? owner->value.db : NULL;
  OpTable* ops;
  owner->ops = &intScalar;     /* avoid clash in case of interrupts */
  if (entry != NULL) {
    if ((ops = entry->sym_ops) == &dataBlockSym) {
      DataBlock* db = entry->sym_value.db;
      owner->value.db = Ref(db);
    } else {
      owner->value = entry->sym_value;
    }
  } else {
    owner->value.db = RefNC(&nilDB);
    ops = &dataBlockSym;
  }
  owner->ops = ops;            /* change ops only AFTER value updated */
  Unref(old);
}

/* get args from the top of the stack: first arg is hash table, second arg
   should be key name or keyword followed by third nil arg */
static int get_table_and_key(int nargs, h_table_t** table,
                             const char** keystr)
{
  Symbol* stack = sp - nargs + 1;
  if (nargs == 2) {
    /* e.g.: foo(table, "key") */
    Symbol* s = stack + 1; /* symbol for key */
    if (s->ops) {
      Operand op;
      s->ops->FormOperand(s, &op);
      if (! op.type.dims && op.ops->typeID == YOR_STRING) {
        *table = get_table(stack);
        *keystr = *(char**)op.value;
        return 0;
      }
    }
  } else if (nargs == 3) {
    /* e.g.: foo(table, key=) */
    if (! (stack + 1)->ops && is_nil(stack + 2)) {
      *table = get_table(stack);
      *keystr = globalTable.names[(stack + 1)->index];
      return 0;
    }
  }
  return -1;
}

static h_table_t* get_table(Symbol* stack)
{
  Symbol* sym = YETI_DEREFERENCE_SYMBOL(stack);
  if (sym->ops != &dataBlockSym || sym->value.db->ops != &hashOps)
    h_error("expected hash table object");
  DataBlock* db = sym->value.db;
  if (sym != stack) {
    /* Replace reference onto the stack (equivalent to the statement
       ReplaceRef(s); see ydata.c for actual code of this routine). */
    stack->value.db = Ref(db);
    stack->ops = &dataBlockSym; /* change ops only AFTER value updated */
  }
  return (h_table_t*)db;
}

static void set_members(h_table_t* table, Symbol* stack, int nargs)
{
  if (nargs%2 != 0) h_error("last key has no value");
  for (int i = 0; i < nargs; i += 2, stack += 2) {
    /* Get key name. */
    const char* name;
    if (stack->ops) {
      Operand op;
      stack->ops->FormOperand(stack, &op);
      if (! op.type.dims && op.ops == &stringOps) {
        name = *(char**)op.value;
      } else {
        name = NULL;
      }
    } else {
      name = globalTable.names[stack->index];
    }
    if (! name) {
      h_error("bad key, expecting a non-nil scalar string name or a keyword");
    }

    /* Replace value. */
    h_insert(table, name, stack + 1);
  }
}

/*---------------------------------------------------------------------------*/
/* The following code implement management of hash tables with string keys and
   aimed at the storage of Yorick DataBlock.  The hashing algorithm is taken
   from Tcl (which is 25-30% more efficient than Yorick's algorithm). */

static h_table_t* h_new(size_t number)
{
  /* Member SIZE of a hash table is always a power of 2, greater or
     equal 2*NUMBER (twice the number of entries in the table). */
  size_t size = 1;
  while (size < number) {
    size <<= 1;
  }
  size <<= 1;
  size_t nbytes = size*sizeof(h_entry_t*);
  h_table_t* table = h_malloc(sizeof(h_table_t));
  if (table == NULL) {
  enomem:
    h_error("insufficient memory for new hash table");
    return NULL;
  }
  table->bucket = h_malloc(nbytes);
  if (table->bucket == NULL) {
    h_free(table);
    goto enomem;
  }
  memset(table->bucket, 0, nbytes);
  table->references = 0;
  table->ops = &hashOps;
  table->eval = -1L;
  table->number = 0;
  table->size = size;
  table->new_size = size;
  return table;
}

static void h_delete(h_table_t* table)
{
  if (table != NULL) {
    if (table->new_size > table->size) {
      rehash(table);
    }
    size_t size = table->size;
    h_entry_t** bucket = table->bucket;
    for (size_t i = 0; i < size; ++i) {
      h_entry_t* entry = bucket[i];
      while (entry) {
        void* addr = entry;
        if (entry->sym_ops == &dataBlockSym) {
          DataBlock* db = entry->sym_value.db;
          Unref(db);
        }
        entry = entry->next;
        h_free(addr);
      }
    }
    h_free(bucket);
    h_free(table);
  }
}

static h_entry_t* h_find(h_table_t* table, const char* name)
{
  if (name != NULL) {
    /* Compute hash value. */
    size_t hash, len;
    HASH_STRING(hash, len, name);

    /* Ensure consistency of the bucket. */
    if (table->new_size > table->size) {
      rehash(table);
    }

    /* Locate matching entry. */
    for (h_entry_t* entry = table->bucket[hash % table->size];
         entry != NULL; entry = entry->next) {
      if (H_MATCH(entry, hash, name, len)) {
        return entry;
      }
    }
  }

  /* Not found. */
  return NULL;
}

#ifdef H_REMOVE
static int h_remove(h_table_t* table, const char* name)
{
  if (name != NULL) {
    /* Compute hash value. */
    size_t hash, len;
    HASH_STRING(hash, len, name);

    /* Ensure consistency of the bucket. */
    if (table->new_size > table->size) {
      rehash(table);
    }

    /* Find the entry. */
    h_entry_t* prev = NULL;
    size_t index = hash % table->size;
    h_entry_t* entry = table->bucket[index];
    while (entry != NULL) {
      if (H_MATCH(entry, hash, name, len)) {
        /* Delete the entry: (1) remove entry from chained list of entries in
           its bucket, (2) unreference contents of entry, (3) free entry
           memory. */
        /*** CRITICAL CODE BEGIN ***/ {
          if (prev != NULL) {
            prev->next = entry->next;
          } else {
            table->bucket[index] = entry->next;
          }
          if (entry->sym_ops == &dataBlockSym) {
            DataBlock* db = entry->sym_value.db;
            Unref(db);
          }
          h_free(entry);
          --table->number;
        } /*** CRITICAL CODE END ***/
        return 1; /* entry found and deleted */
      }
      prev = entry;
      entry = entry->next;
    }
  }

  /* Not found. */
  return 0;
}
#endif

static int h_insert(h_table_t* table, const char* name, Symbol* sym)
{
  /* Check key string. */
  if (name == NULL) {
    h_error("invalid nil key name");
    return -1; /* error */
  }

  /* Hash key. */
  size_t hash, len;
  HASH_STRING(hash, len, name);

  /* Ensure consistency of the bucket. */
  if (table->new_size > table->size) {
    rehash(table);
  }

  /* Prepare symbol for storage. */
  YETI_SOLVE_REFERENCE(sym);
  if (sym->ops == &dataBlockSym && sym->value.db->ops == &lvalueOps) {
    /* Symbol is an LValue, e.g. part of an array, we fetch (make a private
       copy of) the data to release the link on the total array. */
    FetchLValue(sym->value.db, sym);
  }

  /* Ensure that there are no pending signals before critical operation. */
  if (p_signalling) {
    p_abort();
  }

  /* Replace contents of the entry with same key name if it already exists. */
  for (h_entry_t* entry = table->bucket[hash % table->size];
       entry != NULL; entry = entry->next) {
    if (H_MATCH(entry, hash, name, len)) {
      /*** CRITICAL CODE BEGIN ***/ {
        DataBlock* db = (entry->sym_ops == &dataBlockSym) ?
          entry->sym_value.db : NULL;
        entry->sym_ops = &intScalar; /* avoid clash in case of interrupts */
        Unref(db);
        if (sym->ops == &dataBlockSym) {
          db = sym->value.db;
          entry->sym_value.db = Ref(db);
        } else {
          entry->sym_value = sym->value;
        }
        entry->sym_ops = sym->ops;   /* change ops only AFTER value updated */
      } /*** CRITICAL CODE END ***/
      return 1; /* old entry replaced */
    }
  }

  /* Must create a new entry. */
  if (((table->number + 1) << 1) > table->size) {
    /* Must grow hash table bucket, i.e. "re-hash".  This is done in such a way
       that the bucket is always consistent. This is needed to be robust in
       case of interrupts (at most one entry could be lost in this case). */
    size_t size = table->size;
    size_t nbytes = size*sizeof(h_entry_t*);
    h_entry_t** old_bucket = table->bucket;
    h_entry_t** new_bucket = h_malloc(2*nbytes);
    if (new_bucket == NULL) {
    not_enough_memory:
      h_error("insufficient memory to store new hash entry");
      return -1;
    }
    memcpy(new_bucket, old_bucket, nbytes);
    memset((char*)new_bucket + nbytes, 0, nbytes);
    /*** CRITICAL CODE BEGIN ***/ {
      table->bucket = new_bucket;
      table->new_size = 2*table->size;
      h_free(old_bucket);
    } /*** CRITICAL CODE END ***/
    rehash(table);
  }

  /* Create new entry. */
  h_entry_t* entry = h_malloc(OFFSET(h_entry_t, name) + 1 + len);
  if (entry == NULL) goto not_enough_memory;
  memcpy(entry->name, name, len+1);
  entry->hash = hash;
  if (sym->ops == &dataBlockSym) {
    DataBlock* db = sym->value.db;
    entry->sym_value.db = Ref(db);
  } else {
    entry->sym_value = sym->value;
  }
  entry->sym_ops = sym->ops;

  /* Insert new entry. */
  size_t index = hash % table->size;
  /*** CRITICAL CODE BEGIN ***/ {
    entry->next = table->bucket[index];
    table->bucket[index] = entry;
    ++table->number;
  } /*** CRITICAL CODE END ***/
  return 0; /* a new entry was created */
}

/* This function rehash a recently grown hash table.  The complications come
   from the needs to be robust with respect to interruptions so that the task
   can be interrupted at (almost) any time and resumed later with a minimun
   risk to loose entries.  In fact since rehashing can be done in-place, the
   only critical part is when an entry is moved. This function may be called to
   check whether all entries were in correct position (0 is returned in that
   case).  So for most usages, it is important to only call this function if it
   is detected that rehashing is needed (e.g. by comparing the current and new
   size of the bucket). */
static int rehash(h_table_t* table)
{
  /* Ensure that there are no pending signals before this critical
     operation. */
  if (p_signalling) {
    p_abort();
  }
  int flag = 0;
  h_entry_t** bucket = table->bucket;
  size_t old_size = table->size;
  size_t new_size = table->new_size;
  for (size_t i = 0; i < old_size; ++i) {
    h_entry_t* prev = NULL;
    h_entry_t* entry = bucket[i];
    while (entry != NULL) {
      /* Compute index of the entry in the new bucket. */
      size_t j = entry->hash % new_size;
      if (j == i) {
        /* No change in entry location, just move to next entry in bucket. */
        prev = entry;
        entry = entry->next;
      } else {
        /*** CRITICAL CODE BEGIN ***/ {
          /* Remove entry from its bucket. */
          if (prev == NULL) {
            bucket[i] = entry->next;
          } else {
            prev->next = entry->next;
          }
          /* Insert entry in its new bucket. */
          entry->next = bucket[j];
          bucket[j] = entry;
        } /*** CRITICAL CODE END ***/
          /* Move to next entry in former bucket. */
        entry = ((prev == NULL) ? bucket[i] : prev->next);
        flag = 1;
      }
    }
  }
  table->size = new_size;
  return flag;
}
