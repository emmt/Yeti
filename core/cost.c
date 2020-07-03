/*
 * cost.c -
 *
 * Implement various cost functions for solving inverse problems
 * in Yorick.
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

#include <stdlib.h>
#include <math.h>
#include "yeti.h"

extern BuiltIn Y_cost_l2;
extern BuiltIn Y_cost_l2l1;
extern BuiltIn Y_cost_l2l0;

typedef double cost_worker_t(const double hyper[],
                             const double x[], double g[], size_t number,
                             int choice);
static cost_worker_t cost_l2;
static cost_worker_t cost_l2l1;
static cost_worker_t cost_l2l0;

static void cost_wrapper(int argc, const char* name,
                         cost_worker_t* worker);

void Y_cost_l2(int argc)
{
  cost_wrapper(argc, "l2", cost_l2);
}

void Y_cost_l2l1(int argc)
{
  cost_wrapper(argc, "l2-l1", cost_l2l1);
}

void Y_cost_l2l0(int argc)
{
  cost_wrapper(argc, "l2-l0", cost_l2l0);
}

static void cost_wrapper(int argc, const char* name,
                         cost_worker_t* worker)
{

  if (argc < 2 || argc > 3) yor_error("expecting 2 or 3 arguments");

  /* Get the hyper-parameters. */
  Symbol* s = sp - argc + 1;
  Operand op;
  size_t number;
  const double *hyp;
  if (s->ops && s->ops->FormOperand(s, &op)->ops->isArray) {
    number = op.type.number;
    if (number < 1 || number > 3) {
      yor_error("expecting 1, 2 or 3 hyper-parameters");
      return;
    }
    switch (op.ops->typeID) {
    case YOR_CHAR:
    case YOR_SHORT:
    case YOR_INT:
    case YOR_LONG:
    case YOR_FLOAT:
      op.ops->ToDouble(&op);
    case YOR_DOUBLE:
      hyp = (const double*)op.value;
      break;
    default:
      yor_error("bad data type for the hyper-parameters");
      return;
    }
  } else {
    yor_error("hyper-parameters must be an array");
    return;
  }
  double mu, tpos, tneg;
  if (number == 1) {
    mu = hyp[0];
    tneg = 0.0;
    tpos = 0.0;
  } else if (number == 2) {
    mu = hyp[0];
    tneg = -hyp[1];
    tpos = +hyp[1];
  } else {
    mu = hyp[0];
    tneg = hyp[1];
    tpos = hyp[2];
  }
  int choice = 0;
  if (tneg < 0.0) choice |= 1;
  else if (tneg != 0.0) yor_error("lower threshold must be negative");
  if (tpos > 0.0) choice |= 2;
  else if (tpos != 0.0) yor_error("upper threshold must be positive");

  /* Get the parameters. */
  ++s;
  const double* x = (double*)0;
  int temporary = 0;
  if (s->ops && s->ops->FormOperand(s, &op)->ops->isArray) {
    switch (op.ops->typeID) {
    case YOR_CHAR:
    case YOR_SHORT:
    case YOR_INT:
    case YOR_LONG:
    case YOR_FLOAT:
      op.ops->ToDouble(&op);
    case YOR_DOUBLE:
      x = (const double*)op.value;
      temporary = (! op.references);
      number = op.type.number;
    }
  }
  if (x == NULL) {
    yor_error("invalid input array");
    return;
  }

  /* Get the symbol for the gradient.  If gradient is required and input
     array X is a temporary one, re-use X as the output gradient; otherwise,
     create a new array from scratch for G (see BuildResultU in ops0.c). */
  long index = -1L;
  double* g = NULL;
  if (argc == 3) {
    ++s;
    if (s->ops!=&referenceSym)
      yor_error("needs simple variable reference to store the gradient");
    index = s->index;
    Drop(1);
    if (temporary) {
      g = (double*)x;
    } else {
      g = ((Array*)PushDataBlock(NewArray(&doubleStruct,
                                          op.type.dims)))->value.d;
    }
  }

  double hyper[3] = {mu, tneg, tpos};
  double result = worker(hyper, x, g, number, choice);
  if (index >= 0L) PopTo(&globTab[index]);
  yor_push_value(result);
}

static double cost_l2(const double hyper[],
                      const double x[], double g[], size_t number,
                      int choice)
{
  double mu, result, gscl, t;
  size_t i;

  result = 0.0;
  mu = hyper[0];
  gscl = mu + mu;
  if (g != NULL) {
    for (i = 0; i < number; ++i) {
      t = x[i];
      g[i] = gscl*t;
      result += mu*t*t;
    }
  } else {
    for (i = 0; i < number; ++i) {
      t = x[i];
      result += mu*t*t;
    }
  }
  return result;
}

static double cost_l2l1(const double hyper[],
                        const double x[], double g[], size_t number,
                        int choice)
{
  const double ZERO = 0.0;
  const double ONE = 1.0;
  double mu, result, qneg, qpos, fneg, fpos, gscl, t, q;
  size_t i;

  result = ZERO;
  mu = hyper[0];
  gscl = mu + mu;
  switch (choice) {
  case 0:
    /* L2 norm for all residuals. */
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        t = x[i];
        g[i] = gscl*t;
        result += mu*t*t;
      }
    } else {
      for (i = 0; i < number; ++i) {
        t = x[i];
        result += mu*t*t;
      }
    }
    break;

  case 1:
    /* L2-L1 norm for negative residuals, L2 norm for positive residuals. */
    qneg = ONE/hyper[1];
    fneg = gscl*hyper[1]*hyper[1];
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        if ((t = x[i]) < ZERO) {
          q = qneg*t;
          g[i] = gscl*t/(ONE + q);
          result += fneg*(q - log(ONE + q));
        } else {
          g[i] = gscl*t;
          result += mu*t*t;
        }
      }
    } else {
      for (i = 0; i < number; ++i) {
        if ((t = x[i]) < ZERO) {
          q = qneg*t;
          result += fneg*(q - log(ONE + q));
        } else {
          result += mu*t*t;
        }
      }
    }
    break;

  case 2:
    /* L2 norm for negative residuals, L2-L1 norm for positive residuals. */
    qpos = ONE/hyper[2];
    fpos = gscl*hyper[2]*hyper[2];
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        if ((t = x[i]) > ZERO) {
          q = qpos*t;
          g[i] = gscl*t/(ONE + q);
          result += fpos*(q - log(ONE + q));
        } else {
          g[i] = gscl*t;
          result += mu*t*t;
        }
      }
    } else {
      for (i = 0; i < number; ++i) {
        if ((t = x[i]) > ZERO) {
          q = qpos*t;
          result += fpos*(q - log(ONE + q));
        } else {
          result += mu*t*t;
        }
      }
    }
    break;

  case 3:
    /* L2-L1 norm for all residuals. */
    qneg = ONE/hyper[1];
    fneg = gscl*hyper[1]*hyper[1];
    qpos = ONE/hyper[2];
    fpos = gscl*hyper[2]*hyper[2];
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        if ((t = x[i]) < ZERO) {
          q = qneg*t;
          g[i] = gscl*t/(ONE + q);
          result += fneg*(q - log(ONE + q));
        } else {
          q = qpos*t;
          g[i] = gscl*t/(ONE + q);
          result += fpos*(q - log(ONE + q));
        }
      }
    } else {
      for (i = 0; i < number; ++i) {
        if ((t = x[i]) < ZERO) {
          q = qneg*t;
          result += fneg*(q - log(ONE + q));
        } else {
          q = qpos*t;
          result += fpos*(q - log(ONE + q));
        }
      }
    }
    break;
  }

  return result;
}

static double cost_l2l0(const double hyper[],
                        const double x[], double g[], size_t number,
                        int choice)
{
  const double ZERO = 0.0;
  const double ONE = 1.0;
  double mu, result, tneg, tpos, qneg, qpos, r, s, t;
  size_t i;

  result = ZERO;
  mu = hyper[0];
  s = mu + mu;
  switch (choice) {
  case 0:
    /* L2 norm for all residuals. */
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        r = x[i];
        g[i] = s*r;
        result += r*r;
      }
    } else {
      for (i = 0; i < number; ++i) {
        r = x[i];
        result += r*r;
      }
    }
    break;

  case 1:
    /* L2-L0 norm for negative residuals, L2 norm for positive residuals. */
    tneg = hyper[1];
    qneg = ONE/tneg;
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        if ((r = x[i]) < ZERO) {
          t = qneg*r;
          r = tneg*atan(t);
          g[i] = s*r/(ONE + t*t);
          result += r*r;
        } else {
          g[i] = s*r;
          result += r*r;
        }
      }
    } else {
      for (i = 0; i < number; ++i) {
        if ((r = x[i]) < ZERO) {
          r = tneg*atan(qneg*r);
          result += r*r;
        } else {
          result += r*r;
        }
      }
    }
    break;

  case 2:
    /* L2 norm for negative residuals, L2-L0 norm for positive residuals. */
    tpos = hyper[2];
    qpos = ONE/tpos;
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        if ((r = x[i]) > ZERO) {
          t = qpos*r;
          r = tpos*atan(t);
          g[i] = s*r/(ONE + t*t);
          result += r*r;
        } else {
          g[i] = s*r;
          result += r*r;
        }
      }
    } else {
    }
    break;

  case 3:
    /* L2-L0 norm for all residuals. */
    tneg = hyper[1];
    qneg = ONE/tneg;
    tpos = hyper[2];
    qpos = ONE/tpos;
    if (g != NULL) {
      for (i = 0; i < number; ++i) {
        if ((r = x[i]) < ZERO) {
          t = qneg*r;
          r = tneg*atan(t);
        } else {
          t = qpos*r;
          r = tpos*atan(t);
        }
        g[i] = s*r/(ONE + t*t);
        result += r*r;
      }
    } else {
      for (i = 0; i < number; ++i) {
        if ((r = x[i]) < ZERO) {
          r = tneg*atan(qneg*r);
        } else {
          r = tpos*atan(qpos*r);
        }
        result += r*r;
      }
    }
    break;
  }

  return mu*result;
}
