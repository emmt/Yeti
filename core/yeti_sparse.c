/*
 * yeti_sparse.c -
 *
 * Implement sparse matrix in Yorick.
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

#include <stdlib.h>
#include <string.h>
#include <pstdlib.h>
#include <ydata.h>
#include <yio.h>

/* Debug level: 0 or undefined = none,
 *              1 = perform assertions,
 *              2 = verbose debug.
 */
#undef YETI_SPARSE_DEBUG

extern BuiltIn Y_sparse_matrix, Y_is_sparse_matrix;
extern BuiltIn Y_mvmult;

#if defined(__GNUC__) && __GNUC__ > 1
extern void YError(const char *msg) __attribute__ ((noreturn));
#endif

/*--------------------------------------------------------------------------*/
/* IMPLEMENTATION OF SPARSE MATRICES AS OPAQUE YORICK OBJECTS */

extern PromoteOp PromXX;
extern UnaryOp ToAnyX, NegateX, ComplementX, NotX, TrueX;
extern BinaryOp AddX, SubtractX, MultiplyX, DivideX, ModuloX, PowerX;
extern BinaryOp EqualX, NotEqualX, GreaterX, GreaterEQX;
extern BinaryOp ShiftLX, ShiftRX, OrX, AndX, XorX;
extern BinaryOp AssignX, MatMultX;
extern UnaryOp EvalX, SetupX, PrintX;
static MemberOp sparse_get_member;
static UnaryOp sparse_print;
static void sparse_free(void *addr);  /* ******* Use Unref(obj) ******* */
static void sparse_eval(Operand *op);

Operations sparseOps = {
  &sparse_free, T_OPAQUE, 0, /* promoteID = */T_STRING/* means illegal */,
  "sparse_matrix",
  {&PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX, &PromXX},
  &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX, &ToAnyX,
  &NegateX, &ComplementX, &NotX, &TrueX,
  &AddX, &SubtractX, &MultiplyX, &DivideX, &ModuloX, &PowerX,
  &EqualX, &NotEqualX, &GreaterX, &GreaterEQX,
  &ShiftLX, &ShiftRX, &OrX, &AndX, &XorX,
  &AssignX, &sparse_eval, &SetupX, &sparse_get_member, &MatMultX, &sparse_print
};

/*
 * Sparse matrices store a list of non-zero coefficients and two lists of
 * indices: the 'row' and 'column' indices of the non-zero coefficients.
 * The so-called 'rows' and 'colums' can have any dimension list and
 * represent respectively the output and input spaces of the matrix
 * (i.e. the matrix operates onto a 'vector' in the input space to produce
 * a 'vector' in the output space).  With Yorick dimension ordering in
 * mind, the 'rows' and 'colums' represent respectively the leading and
 * trailing dimensions of the matrix.
 *
 * The 'sparse' structure describes a sparse matrix.
 *
 * The 'index' structure describes the row/column index of a matrix.
 */

typedef struct index index_t;

typedef struct sparse sparse_t;

struct index {
  size_t  nelem;   /* number of elements in indexed array */
  size_t  ndims;   /* number of dimensions in DIMLIST */
  size_t *dimlist; /* list of dimensions */
  size_t *indices; /* indices of non-zero elements along this dimension */
};

struct sparse {
  int references;       /* reference counter */
  Operations *ops;      /* virtual function table */
  size_t    number;     /* number of non-zero elements */
  index_t   row, col;   /* indices for rows / colums of non zero elements */
  void     *coefs;      /* non-zero elements of the sparse matrix */
};

static void sparse_print(Operand *op)
{
  sparse_t *obj = (sparse_t *)op->value;
  char line[80];
  ForceNewline();
  PrintFunc("Object of type: ");
  PrintFunc(obj->ops->typeName);
  sprintf(line, " (references=%d)", obj->references);
  PrintFunc(line);
  ForceNewline();
}

static void sparse_free(void *addr)
{
  /* A sparse matrix is allocated as a single memory chunk. */
  if (addr) p_free(addr);
}

static long   *get_array_l(Symbol *s, size_t *number);
static double *get_array_d(Symbol *s, size_t *number);
static long   *get_dimlist(Symbol *s, size_t *ndims_ptr, size_t *nelem_ptr);
static unsigned int get_flags(Symbol *s, unsigned int default_value);

/** Push a new array with given dimension list.  If DIMLIST is NULL, then the
    array is a vector of length N; otherwise N is the number of dimensions
    (and the number of lements in DIMLIST). */
static Array *push_new_array(StructDef *base, size_t n,
                             const size_t dimlist[]);

/** Pop topmost stack element in place of OWNER.  If CLEANUP is true,
    drop symbols from top of the stack until OWNER is the topmost one. */
static void pop_to(Symbol *owner, int cleanup);

/* usage: sparse_matrix(coefs, row_dimlist, row_indices,
 *                             col_dimlist, col_indices)
 */
void Y_sparse_matrix(int argc)
{
  size_t off1, off2, nint, size;
  size_t i, number=0, ndims1, nelem1, len1, ndims2, nelem2, len2;
  long *dims1, *idx1, *dims2, *idx2;
  size_t *row_indices, *col_indices;
  double *coefs, *nonzero;
  sparse_t *sparse;

  /* Parse the arguments. */
  if (argc != 5) {
    YError("sparse_matrix takes exactly 5 arguments");
  }
  nonzero = get_array_d(sp - 4, &number);
  dims1   = get_dimlist(sp - 3, &ndims1, &nelem1);
  idx1    = get_array_l(sp - 2, &len1);
  dims2   = get_dimlist(sp - 1, &ndims2, &nelem2);
  idx2    = get_array_l(sp    , &len2);

  /* Check row (1st) indices. */
  if (len1 != number) {
    YError("bad number of elements for list of row indices");
  }
  for (i=0 ; i<number ; ++i) {
    if (idx1[i] <= 0 || idx1[i] > nelem1) {
      YError("out of range row index");
    }
  }

  /* Check column (2nd) indices. */
  if (len2 != number) {
    YError("bad number of elements for list of column indices");
  }
  for (i=0 ; i<number ; ++i) {
    if (idx2[i] <= 0 || idx2[i] > nelem2) {
      YError("out of range column index");
    }
  }

  /* Allocate memory for the sparse matrix. Push the opaque object as soon
     as possible onto the stack to limit memory leak in case of
     interrupt. */
#define ROUND_UP(a,b) ((((a) + (b) - 1)/(b))*(b))
  off1 = ROUND_UP(sizeof(sparse_t), sizeof(size_t));
  nint = ndims1 + number + ndims2 + number;
  off2 = ROUND_UP(off1 + nint*sizeof(size_t), sizeof(double));
  size = ROUND_UP(off2 + number*sizeof(double), sizeof(double));
  sparse = p_malloc(size);
  sparse->references = 0;
  sparse->ops = &sparseOps;
  PushDataBlock(sparse); /* early push */
  sparse->number = number;
  sparse->row.nelem = nelem1;
  sparse->row.ndims = ndims1;
  sparse->row.dimlist = (size_t *)((char *)sparse + off1);
  sparse->row.indices = sparse->row.dimlist + ndims1;
  sparse->col.nelem = nelem2;
  sparse->col.ndims = ndims2;
  sparse->col.dimlist = sparse->row.indices + number;
  sparse->col.indices = sparse->col.dimlist + ndims2;
  sparse->coefs = (double *)((char *)sparse + off2);

  /* Fill up coefficients and list of row/column indices (beware that
     Yorick uses 1-based indices). */
  for (i=0 ; i<ndims1 ; ++i) {
    sparse->row.dimlist[i] = dims1[i];
  }
  for (i=0 ; i<ndims2 ; ++i) {
    sparse->col.dimlist[i] = dims2[i];
  }
  coefs = sparse->coefs;
  row_indices = sparse->row.indices;
  col_indices = sparse->col.indices;
  for (i=0 ; i<number ; ++i) row_indices[i] = idx1[i] - 1;
  for (i=0 ; i<number ; ++i) col_indices[i] = idx2[i] - 1;
  for (i=0 ; i<number ; ++i) coefs[i] = nonzero[i];
}

void Y_is_sparse_matrix(int nargs)
{
  Symbol *s;
  int result;
  if (nargs != 1) YError("is_sparse_matrix takes exactly one argument");
  s = (sp->ops == &referenceSym ? &globTab[sp->index] : sp);
  result = (s->ops == &dataBlockSym && s->value.db->ops == &sparseOps);
  PushIntValue(result);
}

static long *get_array_l(Symbol *s, size_t *number_ptr)
{
  Operand op;
  if (! s->ops) YError("unexpected keyword argument");
  switch (s->ops->FormOperand(s, &op)->ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
    op.ops->ToLong(&op);
  case T_LONG:
    if (number_ptr) {
      size_t number = 1;
      Dimension *dims = op.type.dims;
      while (dims) {
        number *= dims->number;
        dims = dims->next;
      }
      *number_ptr = number;
    }
    return op.value;
  }
  YError("expecting array of integers");
  return 0;
}

static double *get_array_d(Symbol *s, size_t *number_ptr)
{
  Operand op;
  if (! s->ops) YError("unexpected keyword argument");
  switch (s->ops->FormOperand(s, &op)->ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
  case T_LONG:
  case T_FLOAT:
    op.ops->ToDouble(&op);
  case T_DOUBLE:
    if (number_ptr) {
      size_t number = 1;
      Dimension *dims = op.type.dims;
      while (dims) {
        number *= dims->number;
        dims = dims->next;
      }
      *number_ptr = number;
    }
    return op.value;
  }
  YError("expecting array of reals");
  return 0;
}

static long *get_dimlist(Symbol *s, size_t *ndims_ptr, size_t *nelem_ptr)
{
  Operand op;
  size_t i, ndims, nelem;
  long *dimlist;
  Dimension *dims;

  if (! s->ops) {
    goto bad_dimlist;
  }
  switch (s->ops->FormOperand(s, &op)->ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
    op.ops->ToLong(&op);
  case T_LONG:
    dims = op.type.dims;
    if (! dims) {
      ndims = 1;
      dimlist = (long *)op.value;
    } else if (! dims->next &&
               (ndims = *(long *)op.value) == dims->number - 1) {
      dimlist = (long *)op.value + 1;
    } else {
      goto bad_dimlist;
    }
    nelem = 1;
    for (i=0 ; i<ndims ; ++i) {
      if (dimlist[i] <= 0) goto bad_dimlist;
      nelem *= dimlist[i];
    }
    if (ndims_ptr) {
      *ndims_ptr = ndims;
    }
    if (nelem_ptr) {
      *nelem_ptr = nelem;
    }
    return dimlist;
  }

 bad_dimlist:
  YError("bad dimension list");
  return NULL; /* avoids compiler warnings */
}

static unsigned int get_flags(Symbol *s, unsigned int default_value)
{
  Operand op;
  if (s->ops==&longScalar) return s->value.l;
  if (s->ops==&intScalar) return s->value.i;
  if (! s->ops->FormOperand(s, &op)->type.dims) {
    switch (op.ops->typeID) {
    case T_CHAR:   return *(char*)op.value;;
    case T_SHORT:  return *(short*)op.value;
    case T_INT:    return *(int*)op.value;
    case T_LONG:   return *(long*)op.value;
    case T_VOID:   return default_value;
    }
  }
  YError("expecting nil or integer scalar argument");
  return 0; /* avoids compiler warnings */
}

/* sparse_eval implements sparse matrix used as a function (or as an indexed
   array). */
static void sparse_eval(Operand *op0)
{
  Operand op;
  size_t k, number;
  Symbol *sym, *stack = op0->owner;
  Dimension *dims;
  sparse_t *sparse;
  const size_t *j, *i;
  const index_t *inp, *out;
  const double *a, *x;
  double *y;
  unsigned int flags;

  if (sp - stack > 2) {
    YError("sparse matrix operator takes 1 or 2 arguments");
  }

  /* Get the 'matrix'. */
#if defined(YETI_SPARSE_DEBUG) && YETI_SPARSE_DEBUG >= 1
  sym = (stack->ops == &referenceSym) ? &globTab[stack->index] : stack;
  if (sym->ops != &dataBlockSym || sym->value.db->ops != &sparseOps)
    YError("unexpected non-sparse matrix object (must be a BUG!)");
  sparse = (sparse_t *)sym->value.db;
#else
  sparse = (sparse_t *)stack->value.db;
#endif

  /* Get the flags. */
  if (sp - stack == 2) {
    flags = get_flags(sp, 0);
  } else {
    flags = 0;
  }
  if (! flags) {
    inp = &sparse->col;
    out = &sparse->row;
  } else if (flags == 1) {
    inp = &sparse->row;
    out = &sparse->col;
  } else {
    inp = 0; /* avoids compiler warning */
    out = 0; /* avoids compiler warning */
    YError("unsupported job value (should be 0 or 1)");
  }

  /* Get the input 'vector'. */
  sym = stack + 1;
  if (! sym->ops) YError("unexpected keyword argument");
  switch (sym->ops->FormOperand(sym, &op)->ops->typeID) {
  case T_CHAR:
  case T_SHORT:
  case T_INT:
  case T_LONG:
  case T_FLOAT:
    op.ops->ToDouble(&op);
    break;
  case T_DOUBLE:
    break;
  default:
    YError("bad data type for input 'vector'");
    return;
  }
  number = 1;
  dims = op.type.dims;
  while (dims) {
    number *= dims->number;
    dims = dims->next;
  }
  if ((dims = op.type.dims) != NULL && dims->next) {
    /* Check the dimension list. */
    k = inp->ndims;
    while (k-- >= 1) {
      if (! dims || dims->number != inp->dimlist[k]) {
        YError("bad dimension list for input 'vector'");
      }
      dims = dims->next;
    }
  } else if (number != inp->nelem) {
    YError("bad number of elements for input 'vector'");
  }
  x = op.value;

  /* Create the output 'vector' and perform the matrix multiplication. */
  y = push_new_array(&doubleStruct, out->ndims, out->dimlist)->value.d;
  memset(y, 0, out->nelem*sizeof(*y));
  i = out->indices;
  j = inp->indices;
  number = sparse->number;
  a = sparse->coefs;
  for (k=0 ; k<number ; ++k) {
    y[i[k]] += a[k]*x[j[k]];
  }

  /* Pop result in place of sparse matrix and cleanup the stack. */
  pop_to(op0->owner, 1);
}

static void push_indices(const index_t *p, size_t number);

static void push_dimlist(const index_t *p);

static void sparse_get_member(Operand *op, char *name)
{
  static long row_dimlist_id = -1L;
  static long row_indices_id = -1L;
  static long col_dimlist_id = -1L;
  static long col_indices_id = -1L;
  static long coefs_id = -1L;
  sparse_t *this = (sparse_t *)op->value;

  if (coefs_id < 0) {
    row_dimlist_id = Globalize("row_dimlist", 0L);
    row_indices_id = Globalize("row_indices", 0L);
    col_dimlist_id = Globalize("col_dimlist", 0L);
    col_indices_id = Globalize("col_indices", 0L);
    coefs_id = Globalize("coefs", 0L);
  }
  if (name) {
    long id = Globalize(name, 0L);
    int ok = 0;
    CheckStack(1);
    if (id == coefs_id) {
      memcpy(push_new_array(&doubleStruct, this->number, NULL)->value.d,
             this->coefs, this->number*sizeof(double));
      ok = 1;
    } else if (id == row_dimlist_id) {
      push_dimlist(&this->row);
      ok = 1;
    } else if (id == row_indices_id) {
      push_indices(&this->row, this->number);
      ok = 1;
    } else if (id == col_dimlist_id) {
      push_dimlist(&this->col);
      ok = 1;
    } else if (id == col_indices_id) {
      push_indices(&this->col, this->number);
      ok = 1;
    }
    if (ok) {
      /* Pop result in place of owner symbol. */
      pop_to(op->owner, 0);
      return;
    }
  }
  YError("illegal sparse matrix member");
}

static void pop_to(Symbol *owner, int cleanup)
{
  DataBlock *old = (owner->ops == &dataBlockSym) ? owner->value.db : NULL;
  Symbol *stack;
  owner->ops = &intScalar; /* avoid clash in case of interrupts */
  stack = sp--; /* sp decremented BEFORE stack element is moved */
  owner->value = stack->value;
  owner->ops = stack->ops;
  Unref(old);
  if (cleanup) {
    while (sp - owner > 0) {
      stack = sp--; /* sp decremented BEFORE stack element is deleted */
      if (stack->ops == &dataBlockSym) Unref(stack->value.db);
    }
  }
}

static void push_dimlist(const index_t *p)
{
  size_t i, ndims = p->ndims;
  const size_t *dimlist = p->dimlist;
  long *ptr = push_new_array(&longStruct, ndims + 1, NULL)->value.l;
  *ptr++ = ndims;
  for (i=0 ; i<ndims ; ++i) {
    ptr[i] = dimlist[i];
  }
}

static void push_indices(const index_t *p, size_t number)
{
  size_t i;
  long *ptr = push_new_array(&longStruct, number, NULL)->value.l;
  const size_t *index = p->indices;
  for (i=0 ; i<number ; ++i) {
    ptr[i] = index[i] + 1;
  }
}

static Array *push_new_array(StructDef *base, size_t n,
                             const size_t dimlist[])
{
  size_t i;
  Dimension *dims = tmpDims;
  tmpDims = NULL;
  if (dims) FreeDimension(dims);
  if (dimlist) {
    for (i=0 ; i<n ; ++i) {
      tmpDims = NewDimension(dimlist[i], 1L, tmpDims);
    }
  } else {
    tmpDims = NewDimension(n, 1L, tmpDims);
  }
  return (Array *)PushDataBlock(NewArray(base, tmpDims));
}

/*---------------------------------------------------------------------------*/

static size_t pack_dimlist(const Dimension *dims, size_t dimlist[],
                           size_t maxdims);

void Y_mvmult(int argc)
{
  const double zero = 0.0;
  double s;
  Operand op;
  unsigned int flags;
  Symbol *stack;
  Dimension *dims;
  const double *a, *x;
  double *y;
  size_t i, j, nx, ny;
  size_t ndims_a, ndims_x, ndims_y;
#define MAXDIMS 32
  size_t dimlist_a[MAXDIMS], dimlist_x[MAXDIMS];

  if (argc < 2 || argc > 3) YError("mvmult takes 2 or 3 arguments");
  stack = sp - argc + 1;
  if (! stack->ops) YError("unexpected keyword argument");
  stack->ops->FormOperand(stack, &op);
  if (op.ops == &sparseOps) {
    sparse_eval(&op); /* that's all folks! */
  } else {
    /* Get the optional flags. */
    flags = (argc == 3 ? get_flags(sp, 0) : 0);
    if ((unsigned int)flags > 1U) {
      YError("unsupported job value (should be 0 or 1)");
    }

    /* Get the 'matrix' A. */
    switch (op.ops->typeID) {
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_FLOAT:
      op.ops->ToDouble(&op);
    case T_DOUBLE:
      ndims_a = pack_dimlist(op.type.dims, dimlist_a, MAXDIMS);
      a = op.value;
      break;
    default:
      YError("expecting array of reals for the 'matrix'");
      return; /* avoid compiler warnings */
    }

    /* Get the 'vector' X. */
    ++stack;
    if (! stack->ops) YError("unexpected keyword argument");
    stack->ops->FormOperand(stack, &op);
    switch (op.ops->typeID) {
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_FLOAT:
      op.ops->ToDouble(&op);
    case T_DOUBLE:
      ndims_x = pack_dimlist(op.type.dims, dimlist_x, MAXDIMS);
      x = op.value;
      break;
    default:
      YError("expecting array of reals for the 'vector'");
      return; /* avoid compiler warnings */
    }

    /* Cleanup temporary dimension list. */
    dims = tmpDims;
    tmpDims = NULL;
    if (dims) FreeDimension(dims);

    /* Check dimension lists and build dimension list of the result. */
    if (ndims_a < ndims_x) {
    bad_dim_list:
      YError("incompatible dimension lists");
      return; /* avoid compiler warnings */
    }
    ndims_y = ndims_a - ndims_x;
    nx = ny = 1;
    if (flags) {
      /* Leading dimensions of A must match dimensions of X, trailing
         dimensions of A are the dimensions of Y. */
      for (i = 0 ; i < ndims_x ; ++i) {
        if (dimlist_x[i] != dimlist_a[i]) goto bad_dim_list;
        nx *= dimlist_x[i];
      }
      for (i = ndims_x ; i < ndims_a ; ++i) {
        tmpDims = NewDimension(dimlist_a[i], 1L, tmpDims);
        ny *= dimlist_a[i];
      }
    } else {
      /* Trailing dimensions of A must match dimensions of X, leading
         dimensions of A are the dimensions of Y. */
      for (i = 0 ; i < ndims_x ; ++i) {
        if (dimlist_x[i] != dimlist_a[i + ndims_y]) goto bad_dim_list;
        nx *= dimlist_x[i];
      }
      for (i = 0 ; i < ndims_y ; ++i) {
        tmpDims = NewDimension(dimlist_a[i], 1L, tmpDims);
        ny *= dimlist_a[i];
      }
    }

    /* Allocate output array and perform matrix multiplication. */
    y = ((Array *)PushDataBlock(NewArray(&doubleStruct, tmpDims)))->value.d;
    if (flags) {
      for (i = 0 ; i < ny ; ++i, a += nx) {
        s = zero;
        for (j = 0 ; j < nx ; ++j) {
          s += a[j]*x[j];
        }
        y[i] = s;
      }
    } else {
      memset(y, 0, ny*sizeof(*y));
      for (j = 0 ; j < nx ; ++j, a += ny) {
        if ((s = x[j]) != zero) {
          for (i = 0 ; i < ny ; ++i) {
            y[i] += a[i]*s;
          }
        }
      }
    }
  }
}

static size_t pack_dimlist(const Dimension *dims, size_t dimlist[],
                           size_t maxdims)
{
  const Dimension *ptr = dims;
  size_t i, n = 0;
  while (ptr) {
    ++n;
    ptr = ptr->next;
  }
  if (n > maxdims) YError("too many dimensions");
  i = n;
  while (i-- >= 1) {
    dimlist[i] = dims->number;
    dims = dims->next;
  }
  return n;
}
