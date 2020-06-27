/*
 * fftw.i --
 *
 * Implement support for FFTW (version 2), the "fastest Fourier transform in
 * the West", in Yorick.  This package is certainly outdated, consider using
 * XFFT instead (https://github.com/emmt/XFFT).
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 1996-2013 Éric Thiébaut <eric.thiebaut@obs.univ-lyon1.fr>
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can use, modify
 * and/or redistribute the software under the terms of the CeCILL-C license as
 * circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
 *
 *-----------------------------------------------------------------------------
 */

/* load dynamic code */
if (is_func(plug_in)) plug_in, "yor_fftw";

extern fftw_plan;
/* DOCUMENT fftw_plan(dimlist, dir)
     Creates a  plan for fast Fourier  transforming by fftw  (which see) of
     arrays of dimension  list DIMLIST.  DIR=+/-1 and has  the same meaning
     as in fft (which see):

        DIR  meaning                1-D discrete Fourier transform
        ---  ---------------------  ------------------------------
         +1  "forward" transform    sum_k x(k)*exp(-i*2*PI*k*l/N)
         -1  "backward" transform   sum_k x(k)*exp(+i*2*PI*k*l/N)

     where i=sqrt(-1).   Except when keyword  REAL is true (se  below), the
     name "forward"  or "backward" is  only a question of  convention, only
     the sign of the complex exponent really matters.

     If keyword  REAL is  true, then  the result is  a plan  for a  real to
     complex  transform if  DIR=+1 ("forward")  or  for a  complex to  real
     transform if  DIR=-1 ("backward").   The result of  a real  to complex
     transform contains only half of  the complex DFT amplitudes (since the
     negative-frequency amplitudes for real  data are the complex conjugate
     of  the   positive-frequency  amplitudes).   If  the   real  array  is
     N1xN2x...xNn then the result is  a complex (N1/2 + 1)xN2x...xNn array.
     Reciprocally,  the   complex  to  real  transform  takes   a  (N1/2  +
     1)xN2x...xNn complex input array to compute a N1xN2x...xNn real array.
     When  the plan is  created with  REAL set  to true,  DIMS must  be the
     dimension list of the real array  and DIR must be +1 ("forward") for a
     real to  complex transform and -1  ("backward") for a  complex to real
     transform.

     If keyword  MEASURE is  true, then FFTW  attempts to find  the optimal
     plan by actually computing several FFT's and measuring their execution
     time.  The  default is to not  run any FFT and  provide a "reasonable"
     plan  (for  a  RISC  processor  with many  registers).   Computing  an
     efficient plan for FFTW (with keyword MEASURE set to true) may be very
     expensive.  FFTW  is therefore mostly advantageous  when several FFT's
     of arrays with  same dimension lists are to be  computed; in this case
     the user should save the plan in a variable, e.g.:

         plan_for_x_and_dir = fftw_plan(dimsof(x), dir, measure=1);
         for (...) {
           ...;
           fft_of_x = fftw(x, plan_for_x_and_dir);
           ...;
         }

     instead of:

         for (...) {
           ...;
           fft_of_x = fftw(x, fftw_plan(dimsof(x), dir, measure=1));
           ...;
         }

     However note that it is relatively inexpensive to compute a plan for
     the default strategy; therefore:

         for (...) {
           ...;
           fft_of_x = fftw(x, fftw_plan(dimsof(x), dir));
           ...;
         }

     is not too inefficient (this is what does cfftw).


  SEE ALSO fftw, fft, fft_setup. */

extern fftw;
func cfftw(x, dir) { return fftw(x, fftw_plan(dimsof(x), dir)); }
/* DOCUMENT  fftw(x, plan)
       -or- cfftw(x, dir)
     Computes the  fast Fourier  transform of X  with the  "fastest Fourier
     transform in the West".

     The fftw function makes use of PLAN that has been created by fftw_plan
     (which  see)  and  computes a  real  to  complex  or complex  to  real
     transform,  if  PLAN  was  created  with keyword  REAL  set  to  true;
     otherwise, fftw computes a complex transform.

     The cfftw function  always computes a complex transform  and creates a
     temporary plan for  the dimensions of X and  FFT direction DIR (+/-1).
     If  you  want to  compute  seral  FFT's  of identical  dimensions  and
     directions, or if  you want to compute real to  complex (or complex to
     real)  transforms, or if  you want  to use  the "measure"  strategy in
     defining FFTW plan, you should rather use fftw_plan and fftw.

   SEE ALSO fftw_plan. */

func fftw_indgen(dim) { return (u= indgen(0:dim-1)) - dim*(u > dim/2); }
/* DOCUMENT fftw_indgen(len)
     Return FFT frequencies along a dimension of length LEN.
   SEE ALSO: indgen, span, fftw_dist. */

func fftw_dist(.., nyquist=, square=, half=)
/* DOCUMENT fftw_dist(dimlist);
     Returns Euclidian lenght of spatial frequencies in frequel units for a
     FFT of dimensions DIMLIST.

     If keyword  NYQUIST is true,  the frequel coordinates get  rescaled so
     that the Nyquist frequency is  equal to NYQUIST along every dimension.
     This is obtained by using coordinates:
        (2.0*NYQUIST/DIM(i))*fft_indgen(DIM(i))
     along i-th dimension of lenght DIM(i).

     If  keyword  SQUARE is  true,  the square  of  the  Euclidian norm  is
     returned instead.

     If keyword HALF  is true, the length is only computed  for half of the
     spatial frequencies so that it can be used with a real to complex FFTW
     forward transform (the first dimension becomes DIM(1)/2 + 1).

   SEE ALSO: fftw, fftw_indgen. */
{
  /* Build dimension list. */
  local arg, dims;
  while (more_args()) {
    eq_nocopy, arg, next_arg();
    if ((s = structof(arg)) == long || s == int || s == short || s == char) {
      /* got an integer array */
      if (! (n = dimsof(arg)(1))) {
        /* got a scalar */
        grow, dims, arg;
      } else if (n == 1 && (n = numberof(arg) - 1) == arg(1)) {
        /* got a vector which is a valid dimension list */
        if (n) grow, dims, arg(2:);
      } else {
        error, "bad dimension list";
      }
    } else if (! is_void(arg)) {
      error, "unexpected data type in dimension list";
    }
  }
  if (! (n = numberof(dims))) return 0.0; /* scalar array */
  if (min(dims) <= 0) error, "negative value in dimension list";

  /* Build square radius array one dimension at a time, starting with the
     last dimension. */
  local r2;
  if (is_void(nyquist)) {
    for (k=n ; k>=1 ; --k) {
      u = double((k==1 && half ? indgen(0:dims(k)/2) : fftw_indgen(dims(k))));
      r2 = (k<n ? r2(-,..) + u*u : u*u);
    }
  } else {
    s = 2.0*nyquist;
    for (k=n ; k>=1 ; --k) {
      dim = dims(k);
      u = (s/dim)*(k==1 && half ? indgen(0:dim/2) : fftw_indgen(dim));
      r2 = (k<n ? r2(-,..) + u*u : u*u);
    }
  }
  return (square ? r2 : sqrt(r2));
}

func fftw_smooth(a, fwhm, fp=, bp=)
/* DOCUMENT fftw_smooth(a, fwhm)
     Returns  array A  smoothed  along  all its  dimensions  by a  discrete
     convolution by  a gaussian with full  width at half  maximum equals to
     FWHM (in "pixel" units).

     Keywords FP and  BP may be used to specify FFTW  plans for the forward
     and/or backward transforms respectively  (which must have been created
     with REAL set to true if and only if A is a real array).

   SEE ALSO: fftw, fftw_plan. */
{
  /*
   * The FFT of a Gaussian of given FWHM is:
   *    exp(-pi^2*fwhm^2*(k/dim)^2/log(16))
   *  where K is the FFT index; hence:
   *    Nyquist = sqrt(pi^2*fwhm^2*(dim/2/dim)^2/4/log(2))
   *            = pi*fwhm/4/sqrt(log(2))
   *            ~ 0.9433593338763967992*fwhm
   */
  dims = dimsof(a);
  real = (structof(a) != complex);
  u = fftw_dist(dims, square=1, nyquist=0.9433593338763967992*fwhm, half=real);
  return fftw((1.0/numberof(a))*exp(-u)
              *fftw(a, (is_void(fp) ? fftw_plan(dims, 1, real=real) : fp)),
              (is_void(bp) ? fftw_plan(dims, -1, real=real) : bp));
}

func fftw_convolve(orig, psf, do_not_roll, fp=, bp=)
/* DOCUMENT fftw_convolve(orig, psf);
       -or- fftw_convolve(orig, psf, do_not_roll);
     Return discrete convolution (computed by  FFTW) of array ORIG by point
     spread  function PSF  (ORIG and  PSF must  have same  dimension list).
     Unless argument  DO_NOT_ROLL is true, PSF is  rolled before.

     Keywords FP and  BP may be used to specify FFTW  plans for the forward
     and/or backward transforms respectively  (which must have been created
     with REAL  set to true  if and  only if _both_  ORIG and PSF  are real
     arrays).

   SEE ALSO: fftw, fftw_plan, roll. */
{
  dims = dimsof(orig);
  real = (structof(orig) != complex && structof(psf) != complex);
  if (is_void(fp)) fp = fftw_plan(dims, +1, real=real);
  p = fftw(fftw(orig, fp)*fftw((do_not_roll?psf:roll(psf)), fp),
           (is_void(bp) ? fftw_plan(dims, -1, real=real) : bp));
  return (1.0/numberof(p))*p;
}
