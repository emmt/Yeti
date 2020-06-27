/* TESTING ROUTINES */
/* very inefficient DFT transforms... */
func fftw_compare(dims, only_ok=)
{
  x = (random(dims) - 0.5) + 1i*(random(dims) - 0.5);
  forward = fftw_plan(dims, +1);
  backward = fftw_plan(dims, -1);
  write, format="%s\n%s\n",
    "   operation A        operation B     OK  avg(abs(A-B))  max(abs(A-B))",
    "-----------------  -----------------  --  -------------  -------------";
  n = 6;
  p = array(pointer, n);
  s = array(string, n);
  p(1) = &fft(x, +1);
  s(1) = "fft(x, +1)";
  p(2) = &fft(x, -1);
  s(2) = "fft(x, -1)";
  p(3) = &dft(x, +1);
  s(3) = "dft(x, +1)";
  p(4) = &dft(x, -1);
  s(4) = "dft(x, -1)";
  p(5) = &fftw(x, forward);
  s(5) = "fftw(x, forward)";
  p(6) = &fftw(x, backward);
  s(6) = "fftw(x, backward)";

  if (0) {
    // tolerance for avg()
    epsilon = 2.2e-16; // machine precision
    tolerance = 10*epsilon*sqrt(numberof(x));
  } else {
    // tolerance for max()
    tolerance = 1e-14*numberof(x);
  }

  for (i=1 ; i<=n ; ++i) {
    for (j=1 ; j<i ; ++j) {
      if ((i % 2) != (j % 2)) continue; /* i.e. only compare _same_
                                           transforms */
      w = abs(*p(i) - *p(j));
      max_w = max(w);
      avg_w = avg(w);
      if (max_w <= tolerance) ok = "+";
      else if (only_ok)       continue;
      else                    ok = " ";
      write, format="%-17s  %-17s  %s   %13.1e  %13.1e\n",
        s(i), s(j), ok, avg_w, max_w;
    }
  }
}
func dft(x, dir)
{
  // PI = 3.14159265358979323846264338327950;
  TWO_PI = 6.28318530717958647692528676655901; // 2*PI
  i = -1i*sign(dir);
  if (is_array(x)) {
    rank = (dims = dimsof(x))(1);
    if (rank == 1) return __dft1(complex(x), i, dims(2));
    if (rank == 2) return __dft2(complex(x), i, dims(2), dims(3));
  }
  error, "bad input";
}
func __dft_op(i, n)
{
  u = indgen(0:n-1);
  w = (TWO_PI/n)*u*u(-,);
  return (cos(w) + i*sin(w));
}
func __dft1(x, i, n1)
{
  return __dft_op(i, n1)(+,)*x(+);
}
func __dft2(x, i, n1, n2)
{
  return (__dft_op(i, n1)(+,)*x(+,))(,+)*__dft_op(i, n2)(+,);
}


func __fftw_time(s,t,n)
{
  t *= 1e6/n; // convert to microseconds
  write, format="%13s  %9.0f  %9.0f  %9.0f\n",
    s, t(1), t(2), t(3);
}
//fftw_time,[2,512,512],repeat=10,measure=0
//dims=51; f = fftw_plan(dims,-1,real=1); b=fftw_plan(dims,+1,real=1);
//x=random(dims); z=fftw(x,f);
func fftw_time(dims, measure=, repeat=, real=)
{
  if (is_void(repeat)) repeat=1;
  x = (random(dims) - 0.5);
  if (! real) x += 1i*(random(dims) - 0.5);
  dims = dimsof(x);
  elapsed = t1 = t2 = t3 = t4 = t5 = t6 = t7 = array(double, 3);
  n = (measure ? 1 : repeat);

  write, format="%s\n%s\n%s\n",
    " (timings in microseconds per operation)",
    "  operation       cpu       system      wall",
    "-------------  ---------  ---------  ---------";


  timer, elapsed;
  for (i=1 ; i<=n; ++i)
    forward = fftw_plan(dims, +1, measure=measure, real=real);
  timer, elapsed, t1;
  __fftw_time, "fftw_plan(-1)", t1, n;

  timer, elapsed;
  for (i=1 ; i<=n; ++i)
    backward = fftw_plan(dims, -1, measure=measure, real=real);
  timer, elapsed, t2;
  __fftw_time, "fftw_plan(+1)", t2, n

  timer, elapsed;
  for (i=1 ; i<=repeat; ++i)
    ws = fft_setup(dims);
  timer, elapsed, t3;
  __fftw_time, "fft_setup", t3, repeat;

  timer, elapsed;
  for (i=1 ; i<=repeat; ++i)
    x1 = fftw(x, forward);
  timer, elapsed, t4;
  __fftw_time, "fftw(x, -1)", t4, repeat;

  timer, elapsed;
  for (i=1 ; i<=repeat; ++i)
    x2 = fftw(x1, backward);
  timer, elapsed, t5;
  __fftw_time, "fftw(x, +1)", t5, repeat;

  timer, elapsed;
  for (i=1 ; i<=repeat; ++i)
    x3 = fft(x, +1, setup=ws);
  timer, elapsed, t6;
  __fftw_time, "fft(x, +1)", t6, repeat;

  timer, elapsed;
  for (i=1 ; i<=repeat; ++i)
    x4 = fft(x3, -1, setup=ws);
  timer, elapsed, t7;
  __fftw_time, "fft(x, -1)", t7, repeat;

}
/*---------------------------------------------------------------------------*/
