/*
 * regul.c -
 *
 * Regularization functions for Yorick.
 *
 *-----------------------------------------------------------------------------
 *
 * This file is part of Yeti (https://github.com/emmt/Yeti) released under the
 * MIT "Expat" license.
 *
 * Copyright (C) 1996-2020: Éric Thiébaut.
 *
 *-----------------------------------------------------------------------------
 *
 * CUSTOMIZATION AT COMPILE TIME
 *
 *	Yorick: This code can be compiled with preprocessor flag
 *		-DYORICK to enable Yorick support.
 *
 *	Number of dimensions: Compile this code with preprocessor flag
 *		-DRGL_MAX_NDIMS=n with n <= 8 to limit the maximum number
 *		of dimensions (and reduce the size of the code).  Since the
 *		code is smart enough to "compress" dimensions, it may
 *		however be applicable for more than RGL_MAX_NDIMS
 *		dimensions.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef _RGL_CODE
#define _RGL_CODE 1

#include <math.h>
#include <string.h>
#include <stdio.h>
#ifdef YORICK
# include <yapi.h> /* for Yorick interface */
#endif

#ifndef NULL
# define NULL 0
#endif

#ifndef RGL_MAX_NDIMS
# define RGL_MAX_NDIMS 8
#endif

/*---------------------------------------------------------------------------*/
/* DEFINITIONS */

typedef double rgl_roughness_penalty_t(const double hyper[], /* hyper-parameters */
                                       const long ndims,     /* number of dimensions */
                                       const long dim[],     /* dimensions */
                                       const long off[],     /* offsets */
                                       const double arr[],   /* model array */
                                       double grd[]);        /* gradient (can be NULL) */

extern rgl_roughness_penalty_t rgl_roughness_l2;
extern rgl_roughness_penalty_t rgl_roughness_l2_periodic;
extern rgl_roughness_penalty_t rgl_roughness_l1;
extern rgl_roughness_penalty_t rgl_roughness_l1_periodic;
extern rgl_roughness_penalty_t rgl_roughness_l2l1;
extern rgl_roughness_penalty_t rgl_roughness_l2l1_periodic;
extern rgl_roughness_penalty_t rgl_roughness_l2l0;
extern rgl_roughness_penalty_t rgl_roughness_l2l0_periodic;
extern rgl_roughness_penalty_t rgl_roughness_cauchy;
extern rgl_roughness_penalty_t rgl_roughness_cauchy_periodic;

#define integer_t long
#define real_t double

/* Error codes: */
#define RGL_ERROR_BAD_ADDRESS   -1
#define RGL_ERROR_BAD_DIMENSION -2
#define RGL_ERROR_BAD_HYPER     -3
#define RGL_ERROR_TOO_MANY_DIMS -4

/* Maximum number of dimensions: */
#define RGL_MAX_NDIMS 8

/* Codes for cost functions: */
#define RGL_COST_L1        1
#define RGL_COST_L2        2
#define RGL_COST_L2L1      3
#define RGL_COST_CAUCHY    4
#define RGL_COST_L2L0      5

#define RGL_MAX(a,b) ((a) >= (b) ? (a) : (b))
#define RGL_MIN(a,b) ((a) <= (b) ? (a) : (b))

#define RGL_PERIODIC  0
#define RGL_COST      RGL_COST_L2
#define RGL_ROUGHNESS rgl_roughness_l2
#include __FILE__

#define RGL_PERIODIC  1
#define RGL_COST      RGL_COST_L2
#define RGL_ROUGHNESS rgl_roughness_l2_periodic
#include __FILE__

#define RGL_PERIODIC  0
#define RGL_COST      RGL_COST_L1
#define RGL_ROUGHNESS rgl_roughness_l1
#include __FILE__

#define RGL_PERIODIC  1
#define RGL_COST      RGL_COST_L1
#define RGL_ROUGHNESS rgl_roughness_l1_periodic
#include __FILE__

#define RGL_PERIODIC  0
#define RGL_COST      RGL_COST_L2L1
#define RGL_ROUGHNESS rgl_roughness_l2l1
#include __FILE__

#define RGL_PERIODIC  1
#define RGL_COST      RGL_COST_L2L1
#define RGL_ROUGHNESS rgl_roughness_l2l1_periodic
#include __FILE__

#define RGL_PERIODIC  0
#define RGL_COST      RGL_COST_CAUCHY
#define RGL_ROUGHNESS rgl_roughness_cauchy
#include __FILE__

#define RGL_PERIODIC  1
#define RGL_COST      RGL_COST_CAUCHY
#define RGL_ROUGHNESS rgl_roughness_cauchy_periodic
#include __FILE__

#define RGL_PERIODIC  0
#define RGL_COST      RGL_COST_L2L0
#define RGL_ROUGHNESS rgl_roughness_l2l0
#include __FILE__

#define RGL_PERIODIC  1
#define RGL_COST      RGL_COST_L2L0
#define RGL_ROUGHNESS rgl_roughness_l2l0_periodic
#include __FILE__


/*---------------------------------------------------------------------------*/
/* YORICK INTERFACE */

#ifdef YORICK

static long *get_vector_l(int iarg, long *ntot)
{
  if (yarg_number(iarg) != 1 || yarg_rank(iarg) > 1) {
    y_error("expecting a vector of integers");
  }
  return ygeta_l(iarg, ntot, NULL);
}

static double *get_vector_d(int iarg, long *ntot)
{
  int id = yarg_number(iarg);
  if (id < 1 || id > 2 || yarg_rank(iarg) > 1) {
    y_error("expecting a vector of reals");
  }
  return ygeta_d(iarg, ntot, NULL);
}

static double *get_array_d(int iarg, long *ntot, long dims[])
{
  int id = yarg_number(iarg);
  if (id < 1 || id > 2) {
    y_error("expecting an array of reals");
  }
  return ygeta_d(iarg, ntot, dims);
}

static void roughness(int argc, const char *name,
                      rgl_roughness_penalty_t *rgl,
                      int n)
{
  double penalty;
  char buf[100];
  double *arr, *grd, *hyp;
  long dims[Y_DIMSIZE];
  long off[Y_DIMSIZE - 1], dim[Y_DIMSIZE - 1];
  long *offset;
  long j, ndims, ref, noffs, nhyps, ntot;
  int iarg, type, flag;

  if (argc < 3 || argc > 4) {
    strcpy(buf, name);
    strcat(buf, " takes 3 or 4 arguments");
    y_error(buf);
  }

  /* Get HYPER argument. */
  hyp = get_vector_d(argc - 1, &nhyps);
  if (nhyps != n) {
    y_error("bad number of hyper-parameters");
  }
  for (j = 0; j < nhyps; ++j) {
    if (hyp[j] < 0.0) {
      y_error("invalid hyper-parameter value(s)");
    }
  }

  /* Get OFFSET and ARR arguments.  Check compatibility of OFFSET and
     dimension list of ARR. */
  offset = get_vector_l(argc - 2, &noffs);
  arr = get_array_d(argc - 3, &ntot, dims);
  ndims = dims[0];
  for (j = 0; j < ndims; ++j) {
    if (j < noffs) {
      off[j] = offset[j];
    } else {
      off[j] = 0;
    }
    dim[j] = dims[j + 1];
  }
  for (j = ndims; j < noffs; ++j) {
    if (offset[j]) {
      y_error("non-zero extra offset(s)");
    }
  }

  /* Get GRD argument.  Create output gradient if needed. */
  grd = NULL;
  if (argc >= 4) {
    iarg = argc - 4;
    ref = yget_ref(iarg);
    if (ref == -1L) {
      y_error("expecting a simple variable reference for argument GRD");
    }
    type = yarg_typeid(iarg);
    flag = 0;
    switch (type) {
    case Y_CHAR:
    case Y_SHORT:
    case Y_INT:
    case Y_LONG:
    case Y_FLOAT:
    case Y_DOUBLE:
      grd = ygeta_d(iarg, NULL, dims);
      if (dims[0] != ndims) {
        flag = 1;
      } else {
        for (j = 0; j < ndims; ++j) {
          if (dims[j + 1] != dim[j]) {
            flag = 1;
            break;
          }
        }
      }
      break;
    case Y_VOID:
      grd = ypush_d(dims);
      break;
    default:
      flag = 1;
    }
    if (flag) {
      y_error("argument GRD must be nil or an array of reals with same dimension list as ARR");
    }
    if (type != Y_DOUBLE) {
      yput_global(ref, iarg);
    }
  }

  /* Compute penalty and return result. */
  penalty = rgl(hyp, ndims, dim, off, arr, grd);
  if (penalty < 0.0) {
    if (penalty == -1.0) {
      strcpy(buf, "bad 1st hyper-parameter in ");
    } else if (penalty == -2.0) {
      strcpy(buf, "bad 2nd hyper-parameter in ");
    } else if (penalty == -11.0) {
      strcpy(buf, "too many dimensions in ");
    } else {
      strcpy(buf, "unknown error in ");
    }
    strcat(buf, name);
    y_error(buf);
  }
  ypush_double(penalty);
}

#define MAKE_BUILTIN(cost, n)						\
void Y_rgl_roughness_##cost(int argc)					\
{									\
   roughness(argc, "rgl_roughness_"#cost, rgl_roughness_##cost, n);	\
}

MAKE_BUILTIN(l2, 1)
MAKE_BUILTIN(l2_periodic, 1)

MAKE_BUILTIN(l1, 1)
MAKE_BUILTIN(l1_periodic, 1)

MAKE_BUILTIN(l2l1, 2)
MAKE_BUILTIN(l2l1_periodic, 2)

MAKE_BUILTIN(l2l0, 2)
MAKE_BUILTIN(l2l0_periodic, 2)

MAKE_BUILTIN(cauchy, 2)
MAKE_BUILTIN(cauchy_periodic, 2)

#endif /* YORICK */

#else /* _RGL_CODE is defined */

/*---------------------------------------------------------------------------*/
/* ROUGHNESS PENALTY */

/*
 * Nomenclature:
 *
 *   PREFIX_NAME_COST[_PERIODIC]
 *
 * where:
 *
 *   PREFIX = rgl
 *
 *   NAME = roughness
 *
 *   COST = l1 / l2 / l2l1 / cauchy / l2l0
 *
 *   DIMENSIONS = # of dimensions of interest, prefixed with a 'p' for
 *                periodic bounds
 *
 * Prototype of penalty functions:
 *
 *    double rgl(const double hyper[], const integer_t ndims,
 *               const long dim[], const long off[],
 *               const double arr[], double grd[]);
 *
 * the returned value is the penalty and the arguments are:
 *
 *   HYPER is array of hyper-parameters. HYPER[0] is the weight of the
 *       regularization; other elements of HYPER depend on the
 *       regularization and/or on the cost function, for instance, HYPER[1]
 *       is the threshold in L2-L1, L2-L0 and Cauchy cost functions;
 *
 *   NDIMS is the number of dimensions (number of elements in DIM and OFF
 *       arrays).
 *
 *   DIM is the list of dimensions.  DIM[0] is the
 *       lenght of the faster varying index.  Hence ARR has
 *       DIM[0]*DIM[1]*...*DIM[NDIMS - 1] elements.
 *
 *   OFF is the list of offsets.  OFF[j] is the offset along j+1-th
 *       dimension.
 *
 *   ARR is the model array.
 *
 *   GRD is an optional (can be NULL) array to store the gradient.  If
 *       non-NULL, GRD must have as many elements has ARR.  GRD is assumed
 *       to have been correctly initialized.  This can be used to sum
 *       the gradient of the cost for differetnt offsets.
 */

#ifdef RGL_ROUGHNESS
double RGL_ROUGHNESS(const double hyper[],  /* hyper-parameters */
                     const integer_t ndims, /* number of dimensions */
                     const integer_t dim[], /* dimensions */
                     const integer_t off[], /* offsets */
                     const real_t arr[],    /* model array */
                     real_t grd[])          /* gradient (can be NULL) */
{
  const double ZERO = 0.0;
#if (RGL_COST == RGL_COST_L2)
  double w ,r;
#endif
#if (RGL_COST == RGL_COST_L1)
  double w;
#endif
#if (RGL_COST == RGL_COST_L2L1)
  const double ONE = 1.0;
  double q, r, s, w;
#endif
#if (RGL_COST == RGL_COST_L2L0)
  const double ONE = 1.0;
  double q, r, s, w;
#endif
#if (RGL_COST == RGL_COST_CAUCHY)
  const double ONE = 1.0;
  double q, r, s, w;
#endif
  double penalty;
#undef s1
#define s1 1 /* fisrt stride is always equal to 1 */
  integer_t j1, e1, lo1, hi1;
  integer_t j2, e2, lo2, hi2, s2;
  integer_t j3, e3, lo3, hi3, s3;
  integer_t j4, e4, lo4, hi4, s4;
  integer_t j5, e5, lo5, hi5, s5;
  integer_t j6, e6, lo6, hi6, s6;
  integer_t j7, e7, lo7, hi7, s7;
  integer_t j8, e8, lo8, hi8, s8;
  integer_t j9,               s9;
  integer_t n, j, jc;
  integer_t dim_c[RGL_MAX_NDIMS]; /* compact offsets */
  integer_t off_c[RGL_MAX_NDIMS]; /* compact dimensions */

  /* Check arguments. */
  if (hyper[0] < ZERO) {
    return -1.0;
  }
#if (RGL_COST == RGL_COST_L2L1) || \
    (RGL_COST == RGL_COST_L2L0) || \
    (RGL_COST == RGL_COST_CAUCHY)
  if (hyper[1] <= ZERO) {
    /*  By continuity, the cost is ZERO when HYPER[1] = 0. */
    return (hyper[1] ? -2.0 : 0.0);
  }
#endif
  if (ndims <= 0 || dim == NULL || off == NULL || hyper == NULL ||
      arr == NULL || hyper[0] <= 0.0) {
    return 0.0;
  }

  /* Compact dimensions. */
  jc = 0; /* index over "compact" dimensions list */
  dim_c[0] = dim[0];
  off_c[0] = off[0];
  for (j = 1; j < ndims; ++j) {
    if (off[j] == 0 && off_c[jc] == 0) {
      /* Collapse with previous dimension. */
      dim_c[jc] *= dim[j];
    } else {
      /* Add new dimension. */
      if (++jc >= RGL_MAX_NDIMS) {
        return -1.0;
      }
      dim_c[jc] = dim[j];
      off_c[jc] = off[j];
    }
  }
  n = jc + 1; /* number of "compact" dimensions */
#if RGL_PERIODIC
  for (jc = 0; jc < n; ++jc) {
    if (off_c[jc] >= 0) {
      off_c[jc] %= dim_c[jc];
    } else {
      off_c[jc] = ((-off_c[jc])%dim_c[jc]);
      if (off_c[jc]) {
        off_c[jc] = dim_c[jc] - off_c[jc];
      }
    }
  }
#endif /* RGL_PERIODIC */


  /* Macros for spk = (k+1)-th stride, jpk = (k+1)-th index. */
#undef jp1
#define jp1 j2
#undef jp2
#define jp2 j3
#undef jp3
#define jp3 j4
#undef jp4
#define jp4 j5
#undef jp5
#define jp5 j6
#undef jp6
#define jp6 j7
#undef jp7
#define jp7 j8
#undef jp8
#define jp8 j9

#undef sp1
#define sp1 s2
#undef sp2
#define sp2 s3
#undef sp3
#define sp3 s4
#undef sp4
#define sp4 s5
#undef sp5
#define sp5 s6
#undef sp6
#define sp6 s7
#undef sp7
#define sp7 s8
#undef sp8
#define sp8 s9

#undef LOOP
#undef CODE

#undef BODY_1
#undef FINAL_1

#undef BODY_2
#undef FINAL_2


  /* Macro definitions and constants for
     L1 (absolute value) cost function. */
#if (RGL_COST == RGL_COST_L1)
  w = (grd ? hyper[0] : ZERO);

# define BODY_1(a1, a2)  penalty += fabs(arr[a2] - arr[a1]);
# define FINAL_1         penalty *= hyper[0];

# define BODY_2(a1, a2)  if (arr[a2] > arr[a1]) { \
                           penalty += w*(arr[a2] - arr[a1]); \
                           grd[a2] += w; \
                           grd[a1] -= w; \
                         } \
                         if (arr[a2] < arr[a1]) { \
                           penalty -= w*(arr[a2] - arr[a1]); \
                           grd[a2] -= w; \
                           grd[a1] += w; \
                         }
#define FINAL_2            /* nothing to do */

#endif /* RGL_COST_L1 */


  /* Macro definitions and constants for
     L2 (quadratic) cost function. */
#if (RGL_COST == RGL_COST_L2)
  w = (grd ? 2.0*hyper[0] : ZERO);

# define BODY_1(a1, a2)  r = arr[a2] - arr[a1]; \
                         penalty += r*r;
# define FINAL_1         penalty *= hyper[0];

# define BODY_2(a1, a2)  r = arr[a2] - arr[a1]; \
                         penalty += r*r; \
                         grd[a2] += w*r; \
                         grd[a1] -= w*r;
# define FINAL_2         penalty *= hyper[0];

#endif /* RGL_COST_L2 */


  /* Macro definitions and constants for
     L2-L1 cost function. */
#if (RGL_COST == RGL_COST_L2L1)
  w = 2.0*hyper[0];
  q = ONE/hyper[1];

# define BODY_1(a1, a2)  r = arr[a2] - arr[a1]; \
                         s = q*fabs(r); \
                         penalty += (s - log(ONE + s));
# define FINAL_1         penalty *= (w*hyper[1]*hyper[1]);

# define BODY_2(a1, a2)  r = arr[a2] - arr[a1]; \
                         s = q*fabs(r); \
                         penalty += (s - log(ONE + s)); \
                         r *= w/(ONE + s); \
                         grd[a2] += r; \
                         grd[a1] -= r;
# define FINAL_2         penalty *= (w*hyper[1]*hyper[1]);

#endif /* RGL_COST_L2L1 */


  /* Macro definitions and constants for
     L2-L0 cost function. */
#if (RGL_COST == RGL_COST_L2L0)
  w = 2.0*hyper[0]*hyper[1];
  q = ONE/hyper[1];

# define BODY_1(a1, a2)  s = atan(q*(arr[a2] - arr[a1])); \
                         penalty += s*s;
# define FINAL_1         penalty *= (hyper[0]*hyper[1]*hyper[1]);

# define BODY_2(a1, a2)  r = q*(arr[a2] - arr[a1]); \
                         s = atan(r); \
                         penalty += s*s; \
                         r = w*s/(ONE + r*r); \
                         grd[a2] += r; \
                         grd[a1] -= r;
# define FINAL_2         penalty *= (hyper[0]*hyper[1]*hyper[1]);

#endif /* RGL_COST_L2L0 */


  /* Macro definitions and constants for
     CAUCHY cost function. */
#if (RGL_COST == RGL_COST_CAUCHY)
  w = 2.0*hyper[0]*hyper[1];
  q = ONE/hyper[1];

# define BODY_1(a1, a2)  r = q*(arr[a2] - arr[a1]); \
                         s = ONE + r*r; \
                         penalty += log(s);
# define FINAL_1         penalty *= (hyper[0]*hyper[1]*hyper[1]);

# define BODY_2(a1, a2)  r = q*(arr[a2] - arr[a1]); \
                         s = ONE + r*r; \
                         penalty += log(s); \
                         r *= w/s; \
                         grd[a2] += r; \
                         grd[a1] -= r;
# define FINAL_2         penalty *= (hyper[0]*hyper[1]*hyper[1]);

#endif /* RGL_COST_CAUCHY */


#if RGL_PERIODIC

  /*
   *  Periodic case
   *  ~~~~~~~~~~~~~
   *
   *    At k-th dimension, j_k is the offset of the 'other' element
   *    not accounting for indices lower than k.  Therefore a loop
   *    writes:
   *
   *      for (i_k = 0; i_k < dim_k; ++i_k) {
   *        j_k = j_{k+1} + ((i_k + off_k)%dim_k)*s_k;
   *        ...;
   *
   *    where i_k and s_k are the index and the stride along k-th
   *    dimension.
   *
   *    This can be rewritten as:
   *
   *      j_k = j_{k+1} + ((i_k*s_k + off_k*s_k) % (dim_k*s_k)
   *          = j_{k+1} + (e_k % s_{k+1})
   *
   *    with:
   *
   *      e_k = i_k*s_k + off_k*s_k
   *          = lo_k, lo_k + s_k, ..., hi_k - s_k
   *      lo_k = off_k*s_k          (i_k = 0)
   *      hi_k = lo_k + dim_k*s_k   (i_k = dim_k)
   *
   *    finally the loop reads:
   *
   *      for (e_k = lo_k; e_k < hi_k; e_k += s_k) {
   *        j_k = j_{k+1} + (e_k % s_{k+1});
   *        ...;
   *
   *    Note that j1 is the position of the 'other' element inside the
   *    final loop.
   */

# define LOOP(k)					\
  for (e##k = lo##k, j##k = jp##k + e##k;		\
       e##k < hi##k;					\
       e##k += s##k, j##k = jp##k + (e##k % sp##k))

# define CODE(k)					\
  sp##k =  dim_c[k-1]*s##k; /* next stride */		\
  lo##k =  off_c[k-1]*s##k;				\
  hi##k = (off_c[k-1] + dim_c[k-1])*s##k;		\
  if (n == k) {						\
    jp##k = 0; /* let the optimizer do the job */	\
    if (grd) {						\
      LOOPS {						\
        BODY_2(j, j1)					\
        ++j;						\
      }							\
      FINAL_2						\
    } else {						\
      LOOPS {						\
        BODY_1(j, j1)					\
        ++j;						\
      }							\
      FINAL_1						\
    }							\
    return penalty;					\
  }

#else /* not RGL_PERIODIC */

# define LOOP(k) \
  for (j##k = lo##k + jp##k, e##k = hi##k + jp##k; j##k < e##k; j##k += s##k)

# define CODE(k)							\
  j += off_c[k-1]*s##k;	/* increment total offset */			\
  sp##k =  dim_c[k-1]*s##k; /* next stride */				\
  lo##k = (off_c[k-1] >= 0 ? 0 : -off_c[k-1]*s##k);			\
  hi##k = (off_c[k-1] >= 0 ? dim_c[k-1] - off_c[k-1] : dim_c[k-1])*s##k;\
  if (lo##k >= hi##k) {							\
    return 0.0;								\
  }									\
  if (n == k) {								\
    jp##k = 0; /* let the optimizer do the job */			\
    if (grd) {								\
      LOOPS {								\
        BODY_2(j1, j1 + j)						\
      }									\
      FINAL_2								\
    } else {								\
      LOOPS {								\
        BODY_1(j1, j1 + j)						\
      }									\
      FINAL_1								\
    }									\
    return penalty;							\
  }

#endif /* not RGL_PERIODIC */

  /* Loop over dimensions. */
  penalty = 0.0;
  j = 0;

#if (RGL_MAX_NDIMS >= 1)
# undef LOOPS
# define LOOPS LOOP(1)
  CODE(1);
#endif

#if (RGL_MAX_NDIMS >= 2)
# undef LOOPS
# define LOOPS LOOP(2) LOOP(1)
  CODE(2);
#endif

#if (RGL_MAX_NDIMS >= 3)
# undef LOOPS
# define LOOPS LOOP(3) LOOP(2) LOOP(1)
  CODE(3);
#endif

#if (RGL_MAX_NDIMS >= 4)
# undef LOOPS
# define LOOPS LOOP(4) LOOP(3) LOOP(2) LOOP(1)
  CODE(4);
#endif

#if (RGL_MAX_NDIMS >= 5)
# undef LOOPS
# define LOOPS LOOP(5) LOOP(4) LOOP(3) LOOP(2) LOOP(1)
  CODE(5);
#endif

#if (RGL_MAX_NDIMS >= 6)
# undef LOOPS
# define LOOPS LOOP(6) LOOP(5) LOOP(4) LOOP(3) LOOP(2) LOOP(1)
  CODE(6);
#endif

#if (RGL_MAX_NDIMS >= 7)
# undef LOOPS
# define LOOPS LOOP(7) LOOP(6) LOOP(5) LOOP(4) LOOP(3) LOOP(2) LOOP(1)
  CODE(7);
#endif

#if (RGL_MAX_NDIMS >= 8)
# undef LOOPS
# define LOOPS LOOP(8) LOOP(7) LOOP(6) LOOP(5) LOOP(4) LOOP(3) LOOP(2) LOOP(1)
  CODE(8);
#endif

  return -11.0;
}

#endif /* RGL_ROUGHNESS */

/*---------------------------------------------------------------------------*/
/* CLEANUP */

/* Undefine all macros. */
#undef RGL_COST
#undef RGL_PERIODIC
#undef RGL_ROUGHNESS

#endif /* _RGL_CODE */
