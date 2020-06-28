/*
 * convolve.c -
 *
 * Convolution and wavelet smoothing along one dimension.
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

#ifndef _YETI_CONVOLVE_C
#define _YETI_CONVOLVE_C 1

extern void yeti_convolve_f(float dst[], const float src[], int stride,
                            int n, int nafter, const float ker[], int w,
                            int scale, int border, float ws[]);
extern void yeti_convolve_c(float dst[], const float src[], int stride,
                            int n, int nafter, const float ker[], int w,
                            int scale,  int border, float ws[]);
extern void yeti_convolve_d(double dst[], const double src[], int stride,
                            int n, int nafter, const double ker[], int w,
                            int scale, int border, double ws[]);
extern void yeti_convolve_z(double dst[], const double src[], int stride,
                            int n, int nafter, const double ker[], int w,
                            int scale, int border, double ws[]);

#define HAVE_MEMCPY 1 /* use memcpy instead of loops? */

#if HAVE_MEMCPY
# include <string.h>
#endif
/*---------------------------------------------------------------------------*/
/* OPERATIONS FOR COMPLEX DATA TYPE */

#define ENCODE(name, op, real_t)                                           \
void name(real_t dst[], const real_t src[], int stride, int n, int nafter, \
          const real_t ker[], int w, int scale, int border, real_t ws[])   \
{								           \
  op(dst,   src,   2*stride, n, nafter, ker, w, scale, border, ws);	   \
  op(dst+1, src+1, 2*stride, n, nafter, ker, w, scale, border, ws);        \
}
ENCODE(yeti_convolve_c, yeti_convolve_f, float)
ENCODE(yeti_convolve_z, yeti_convolve_d, double)
#undef ENCODE

/*---------------------------------------------------------------------------*/
/* OPERATIONS FOR FLOATING POINT DATA TYPE */
#define real_t           float
#define ZERO             0.0f
#define CONVOLVE         yeti_convolve_f
#define CONVOLVE_1       convolve_f
#include __FILE__

#define real_t           double
#define ZERO             0.0
#define CONVOLVE         yeti_convolve_d
#define CONVOLVE_1       convolve_d
#include __FILE__

#else /* _YETI_CONVOLVE_C defined. ------------------------------------------*/

/* Private routines, data and definitions used in this file. */

#ifdef CONVOLVE_1
static void CONVOLVE_1(real_t dst[], const real_t src[], int n,
                       const real_t ker[], int w, int scale, int border);
#endif

#ifdef CONVOLVE
void CONVOLVE(real_t* dst, const real_t* src, int stride, int n,
              int nafter, const real_t* ker, int w, int scale,
              int border, real_t* ws)
{
  int i, j, k, l;

  ker += w;
  if (stride == 1) {
    if (dst == src) {
      for (k=l=0; l<nafter; ++l, k+=n) {
#if HAVE_MEMCPY
        memcpy(ws, src+k, n*sizeof(real_t));
#else
        for (j=0, i=k; j<n; ++j, ++i) ws[j] = src[i];
#endif
        CONVOLVE_1(dst+k, ws, n, ker, w, scale, border);
      }
    } else {
      for (k=l=0; l<nafter; ++l, k+=n) {
        CONVOLVE_1(dst+k, src+k, n, ker, w, scale, border);
      }
    }
  } else {
    real_t* wp = ws+n;
    for (l=0; l<nafter ; ++l) {
      for (k=0; k<stride; ++k) {
        for (j=0, i=k+stride*n*l; j<n; ++j, i+=stride) ws[j] = src[i];
        CONVOLVE_1(wp, ws, n, ker, w, scale, border);
        for (j=0, i=k+stride*n*l; j<n; ++j, i+=stride) dst[i] = wp[j];
      }
    }
  }
}
#endif /* CONVOLVE */

#ifdef CONVOLVE_1
static void CONVOLVE_1(real_t dst[], const real_t src[], int n,
                       const real_t ker[], int w, int scale, int border)
{
  int i, j, k;
  real_t sum, xl, xr;

  if (scale>1) {
    /*************************
     *                       *
     *  WAVELET CONVOLUTION  *
     *                       *
     *************************/
    int ws = w*scale;
    switch (border) {
    case 0:
      /* Extrapolate missing left values by the leftmost one
         and missing right values by the rightmost one. */
      xl = src[0];
      xr = src[n-1];
      for (i=0 ; i<n ; ++i) {
        for (sum=ZERO, j=-w, k=i-ws ; j<=w ; ++j, k+=scale) {
          sum += ker[j] * (k>=0 ? (k<n ? src[k] : xl): xr);
        }
        dst[i] = sum;
      }
      break;
    case 1:
      /* Extrapolate missing left values by zero and
         missing right values by the rightmost one. */
      xr = src[n-1];
      for (i=0 ; i<n ; ++i) {
        for (sum=ZERO, j=-w, k=i-ws ; j<=w ; ++j, k+=scale) {
          if      (k>=n) sum += ker[j] * xr;
          else if (k>=0) sum += ker[j] * src[k];
        }
        dst[i] = sum;
      }
      break;
    case 2:
      /* Extrapolate missing left values by the leftmost one
         and missing right values by zero. */
      xl = src[0];
      for (i=0 ; i<n ; ++i) {
        for (sum=ZERO, j=-w, k=i-ws ; j<=w ; ++j, k+=scale) {
          if      (k<0) sum += ker[j] * xl;
          else if (k<n) sum += ker[j] * src[k];
          else break;
        }
        dst[i] = sum;
      }
      break;
    case 3:
      /* Extrapolate missing values by zero. */
      for (i=0 ; i<n ; ++i) {
        for (sum=ZERO, j=-w, k=i-ws ; j<=w && k<n ; ++j, k+=scale) {
          if (k>=0) sum += ker[j] * src[k];
        }
        dst[i] = sum;
      }
      break;
    case 4:
      /* Periodic conditions. */
      for (i=0 ; i<n ; ++i) {
        if ((k= i-(ws%n)) < 0) k += n;
        for (sum=ZERO, j=-w ; j<=w ; ++j, k+=scale) {
          sum += ker[j] * src[k%n];
        }
        dst[i] = sum;
      }
      break;
    default:
      /* Do not extrapolate missing values but normalize convolution
         product by sum of kernel weights taken into account (assuming
         they are all positive). */
      for (i=0 ; i<n ; ++i) {
        for (xl=sum=ZERO, j=-w, k=i-ws ; j<=w && k<n ; ++j, k+=scale) {
          if (k>=0) {
            sum += (xr= ker[j]) * src[k];
            xl += xr;
          }
        }
        dst[i] = xl ? sum/xl : ZERO;
      }
    }

  } else {
    /************************
     *                      *
     *  NORMAL CONVOLUTION  *
     *                      *
     ************************/
    int jl, jr, wp1=w+1;

    switch (border) {
    case 0:
      /* Extrapolate missing left values by the leftmost one
         and missing right values by the rightmost one. */
      xl = src[0];
      xr = src[n-1];
      for (i=0 ; i<n ; ++i) {
        sum = ZERO;
        jl = i<=w ? -i : -w;            /* limit of left border */
        if ((jr = n-i) > wp1) jr = wp1; /* limit of right border */
        for (j=-w        ; j<jl ; ++j)      sum += ker[j] * xl;
        for (j=jl, k=i+j ; j<jr ; ++j, ++k) sum += ker[j] * src[k];
        for (j=jr        ; j<=w ; ++j)      sum += ker[j] * xr;
        dst[i] = sum;
      }
      break;
    case 1:
      /* Extrapolate missing left values by zero and
         missing right values by the rightmost one. */
      xr = src[n-1];
      for (i=0 ; i<n ; ++i) {
        sum = ZERO;
        jl = i<=w ? -i : -w;            /* limit of left border */
        if ((jr = n-i) > wp1) jr = wp1; /* limit of right border */
        for (j=jl, k=i+j ; j<jr ; ++j, ++k) sum += ker[j] * src[k];
        for (j=jr        ; j<=w ; ++j)      sum += ker[j] * xr;
        dst[i] = sum;
      }
      break;
    case 2:
      /* Extrapolate missing left values by the leftmost one
         and missing right values by zero. */
      xl = src[0];
      for (i=0 ; i<n ; ++i) {
        sum = ZERO;
        jl = i<=w ? -i : -w;            /* limit of left border */
        if ((jr = n-i) > wp1) jr = wp1; /* limit of right border */
        for (j=-w        ; j<jl ; ++j)      sum += ker[j] * xl;
        for (j=jl, k=i+j ; j<jr ; ++j, ++k) sum += ker[j] * src[k];
        dst[i] = sum;
      }
      break;
    case 3:
      /* Extrapolate missing values by zero. */
      for (i=0 ; i<n ; ++i) {
        sum = ZERO;
        jl = i<=w ? -i : -w;            /* limit of left border */
        if ((jr = n-i) > wp1) jr = wp1; /* limit of right border */
        for (j=jl, k=i+j ; j<jr ; ++j, ++k) sum += ker[j] * src[k];
        dst[i] = sum;
      }
      break;
    case 4:
      /* Periodic conditions. */
      for (i=0 ; i<n ; ++i) {
        sum = ZERO;
        if ((k= i-(w%n)) < 0) k += n;
        for (j=-w ; j<=w ; ++j, ++k) {
          if (k>=n) k = 0;
          sum += ker[j] * src[k];
        }
        dst[i] = sum;
      }
      break;
    default:
      /* Do not extrapolate missing values but normalize convolution
         product by sum of kernel weights taken into account (assuming
         they are all positive). */
      for (i=0 ; i<n ; ++i) {
        for (xl=sum=ZERO, j=-w, k=i-w ; j<=w && k<n ; ++j, ++k) {
          if (k >= 0) {
            xr = ker[j];
            sum += xr* src[k];
            xl  += xr;
          }
        }
        dst[i] = xl ? sum/xl : ZERO;
      }
    }
  }
}
#endif /* CONVOLVE_1 */
/*---------------------------------------------------------------------------*/
#undef real_t
#undef ZERO
#undef CONVOLVE
#undef CONVOLVE_1
#endif /* _YETI_CONVOLVE_C */
