if (! is_integer(0)) error;
if (! is_integer(char(0))) error;
if (! is_integer(short(0))) error;
if (! is_integer(int(0))) error;
if (! is_integer(long(0))) error;
if (  is_integer(0.0)) error;
if (  is_integer(float(0))) error;
if (  is_integer(double(0))) error;
if (  is_integer(complex(0))) error;
if (  is_integer(string(0))) error;
if (! is_integer((x=[0]))) error;
if (! is_integer((x=[char(0)]))) error;
if (! is_integer((x=[short(0)]))) error;
if (! is_integer((x=[int(0)]))) error;
if (! is_integer((x=[long(0)]))) error;
if (  is_integer((x=[0.0]))) error;
if (  is_integer((x=[float(0)]))) error;
if (  is_integer((x=[double(0)]))) error;
if (  is_integer((x=[complex(0)]))) error;
if (  is_integer((x=[string(0)]))) error;

if (  is_real(0)) error;
if (  is_real(char(0))) error;
if (  is_real(short(0))) error;
if (  is_real(int(0))) error;
if (  is_real(long(0))) error;
if (! is_real(0.0)) error;
if (! is_real(float(0))) error;
if (! is_real(double(0))) error;
if (  is_real(complex(0))) error;
if (  is_real(string(0))) error;
if (  is_real((x=[0]))) error;
if (  is_real((x=[char(0)]))) error;
if (  is_real((x=[short(0)]))) error;
if (  is_real((x=[int(0)]))) error;
if (  is_real((x=[long(0)]))) error;
if (! is_real((x=[0.0]))) error;
if (! is_real((x=[float(0)]))) error;
if (! is_real((x=[double(0)]))) error;
if (  is_real((x=[complex(0)]))) error;
if (  is_real((x=[string(0)]))) error;

if (  is_complex(0)) error;
if (  is_complex(char(0))) error;
if (  is_complex(short(0))) error;
if (  is_complex(int(0))) error;
if (  is_complex(long(0))) error;
if (  is_complex(0.0)) error;
if (  is_complex(float(0))) error;
if (  is_complex(double(0))) error;
if (! is_complex(complex(0))) error;
if (  is_complex(string(0))) error;
if (  is_complex((x=[0]))) error;
if (  is_complex((x=[char(0)]))) error;
if (  is_complex((x=[short(0)]))) error;
if (  is_complex((x=[int(0)]))) error;
if (  is_complex((x=[long(0)]))) error;
if (  is_complex((x=[0.0]))) error;
if (  is_complex((x=[float(0)]))) error;
if (  is_complex((x=[double(0)]))) error;
if (! is_complex((x=[complex(0)]))) error;
if (  is_complex((x=[string(0)]))) error;

if (! is_numerical(0)) error;
if (! is_numerical(char(0))) error;
if (! is_numerical(short(0))) error;
if (! is_numerical(int(0))) error;
if (! is_numerical(long(0))) error;
if (! is_numerical(0.0)) error;
if (! is_numerical(float(0))) error;
if (! is_numerical(double(0))) error;
if (! is_numerical(complex(0))) error;
if (  is_numerical(string(0))) error;
if (! is_numerical((x=[0]))) error;
if (! is_numerical((x=[char(0)]))) error;
if (! is_numerical((x=[short(0)]))) error;
if (! is_numerical((x=[int(0)]))) error;
if (! is_numerical((x=[long(0)]))) error;
if (! is_numerical((x=[0.0]))) error;
if (! is_numerical((x=[float(0)]))) error;
if (! is_numerical((x=[double(0)]))) error;
if (! is_numerical((x=[complex(0)]))) error;
if (  is_numerical((x=[string(0)]))) error;

/* check for L-value bug. */
t1 = 1;
t2 = 1.0;
t3 = "hello";
vp = [&t1, &t2, &t3];
if (! is_scalar(*vp(1))) error;
if (! is_scalar(*vp(2))) error;
if (! is_scalar(*vp(3))) error;
if (! is_integer(*vp(1))) error;
if (  is_integer(*vp(2))) error;
if (  is_integer(*vp(3))) error;
if (  is_real(*vp(1))) error;
if (! is_real(*vp(2))) error;
if (  is_real(*vp(3))) error;
if (! is_numerical(*vp(1))) error;
if (! is_numerical(*vp(2))) error;
if (  is_numerical(*vp(3))) error;
if (  is_complex(*vp(1))) error;
if (  is_complex(*vp(2))) error;
if (  is_complex(*vp(3))) error;
if (  is_string(*vp(1))) error;
if (  is_string(*vp(2))) error;
if (! is_string(*vp(3))) error;

func yeti_test_quick_quartile
{
  for (n = 300; n <= 400; ++n) {
    x = random_n(n);
    q = quick_quartile(x);
    n1 = numberof(where(x < q(1)));
    n2 = numberof(where(x < q(2)));
    n3 = numberof(where(x < q(3)));
    f = 1e2/n;
    write, format="%4d: %.2f%% %.2f%% %.2f%%\n",
      n, f*n1, f*n2, f*n3;
  }
}
