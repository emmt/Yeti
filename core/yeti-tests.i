
local _test_result;
local test_npassed;
local test_nfailed;
func test_summary(nil)
{
    if (is_void(test_npassed)) test_npassed = 0;
    if (is_void(test_nfailed)) test_nfailed = 0;
    write, format="\nNumber of successful test(s) ---> %5d\n", test_npassed;
    write, format="Number of failures(s) ----------> %5d\n", test_nfailed;
}

func test_eval(expr)
{
    extern _test_result;
    if (is_void(test_npassed)) test_npassed = 0;
    if (is_void(test_nfailed)) test_nfailed = 0;
    include, ["_test_result = ("+expr+");"], 1;
    if (_test_result) {
        ++test_npassed;
    } else {
        ++test_nfailed;
        write, format="TEST FAILED: %s\n", expr;
    }
}
errs2caller, test_eval;

func test_assert(expr, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9)
{
    extern _test_result;
    if (is_void(test_npassed)) test_npassed = 0;
    if (is_void(test_nfailed)) test_nfailed = 0;
    if (expr) {
        ++test_npassed;
    } else {
        ++test_nfailed;
        if (is_void(a1)) {
            write, format="ASSERTION FAILED: %s\n", a0;
        } else {
            f =  "ASSERTION FAILED: " + a0 + "\n";
            if (is_void(a2)) {
                write, format=f, a1;
            } else if (is_void(a3)) {
                write, format=f, a1, a2;
            } else if (is_void(a4)) {
                write, format=f, a1, a2, a3;
            } else if (is_void(a5)) {
                write, format=f, a1, a2, a3, a4;
            } else if (is_void(a6)) {
                write, format=f, a1, a2, a3, a4, a5;
            } else if (is_void(a7)) {
                write, format=f, a1, a2, a3, a4, a5, a6;
            } else if (is_void(a8)) {
                write, format=f, a1, a2, a3, a4, a5, a6, a7;
            } else if (is_void(a9)) {
                write, format=f, a1, a2, a3, a4, a5, a6, a7, a8;
            } else {
                write, format=f, a1, a2, a3, a4, a5, a6, a7, a8, a9;
            }
        }
    }
}
errs2caller, test_assert;

func test_quick_quartile(arg, verb=)
{
    if (is_void(arg)) arg = 300:320;
    if (is_range(arg)) arg = indgen(arg);
    for (i = 1; i <= numberof(arg); ++i) {
        n = arg(i);
        x = random_n(n);
        q = quick_quartile(x);
        n1 = numberof(where(x < q(1)));
        n2 = numberof(where(x < q(2)));
        n3 = numberof(where(x < q(3)));
        if (verb == 1) {
            f = 1e2/n;
            write, format="%4d: %.2f%% %.2f%% %.2f%%\n",
                n, f*n1, f*n2, f*n3;
            f = 1e2/n;
        } else if (verb) {
            e1 = n1 - 0.25*n;
            e2 = n2 - 0.50*n;
            e3 = n3 - 0.75*n;
            write, format="%4d: %4.1f %4.1f %4.1f\n",
                n, e1, e2, e3;
        } else {
            e1 = n1 - 0.25*n;
            e2 = n2 - 0.50*n;
            e3 = n3 - 0.75*n;
            test_assert, (abs(e1) < 1), "1st quartile wrong by %f.1", e1;
            test_assert, (abs(e2) < 1), "2nd quartile wrong by %f.1", e3;
            test_assert, (abs(e3) < 1), "3rd quartile wrong by %f.1", e4;
        }
    }
}

func test_types(nil)
{
    test_eval, "is_integer(0)";
    test_eval, "is_integer(char(0))";
    test_eval, "is_integer(short(0))";
    test_eval, "is_integer(int(0))";
    test_eval, "is_integer(long(0))";
    test_eval, "!is_integer(0.0)";
    test_eval, "!is_integer(float(0))";
    test_eval, "!is_integer(double(0))";
    test_eval, "!is_integer(complex(0))";
    test_eval, "!is_integer(string(0))";
    test_eval, "is_integer(([0]))";
    test_eval, "is_integer(([char(0)]))";
    test_eval, "is_integer(([short(0)]))";
    test_eval, "is_integer(([int(0)]))";
    test_eval, "is_integer(([long(0)]))";
    test_eval, "!is_integer(([0.0]))";
    test_eval, "!is_integer(([float(0)]))";
    test_eval, "!is_integer(([double(0)]))";
    test_eval, "!is_integer(([complex(0)]))";
    test_eval, "!is_integer(([string(0)]))";

    test_eval, "!is_real(0)";
    test_eval, "!is_real(char(0))";
    test_eval, "!is_real(short(0))";
    test_eval, "!is_real(int(0))";
    test_eval, "!is_real(long(0))";
    test_eval, "is_real(0.0)";
    test_eval, "is_real(float(0))";
    test_eval, "is_real(double(0))";
    test_eval, "!is_real(complex(0))";
    test_eval, "!is_real(string(0))";
    test_eval, "!is_real(([0]))";
    test_eval, "!is_real(([char(0)]))";
    test_eval, "!is_real(([short(0)]))";
    test_eval, "!is_real(([int(0)]))";
    test_eval, "!is_real(([long(0)]))";
    test_eval, "is_real(([0.0]))";
    test_eval, "is_real(([float(0)]))";
    test_eval, "is_real(([double(0)]))";
    test_eval, "!is_real(([complex(0)]))";
    test_eval, "!is_real(([string(0)]))";

    test_eval, "!is_complex(0)";
    test_eval, "!is_complex(char(0))";
    test_eval, "!is_complex(short(0))";
    test_eval, "!is_complex(int(0))";
    test_eval, "!is_complex(long(0))";
    test_eval, "!is_complex(0.0)";
    test_eval, "!is_complex(float(0))";
    test_eval, "!is_complex(double(0))";
    test_eval, "is_complex(complex(0))";
    test_eval, "!is_complex(string(0))";
    test_eval, "!is_complex(([0]))";
    test_eval, "!is_complex(([char(0)]))";
    test_eval, "!is_complex(([short(0)]))";
    test_eval, "!is_complex(([int(0)]))";
    test_eval, "!is_complex(([long(0)]))";
    test_eval, "!is_complex(([0.0]))";
    test_eval, "!is_complex(([float(0)]))";
    test_eval, "!is_complex(([double(0)]))";
    test_eval, "is_complex(([complex(0)]))";
    test_eval, "!is_complex(([string(0)]))";

    test_eval, "is_numerical(0)";
    test_eval, "is_numerical(char(0))";
    test_eval, "is_numerical(short(0))";
    test_eval, "is_numerical(int(0))";
    test_eval, "is_numerical(long(0))";
    test_eval, "is_numerical(0.0)";
    test_eval, "is_numerical(float(0))";
    test_eval, "is_numerical(double(0))";
    test_eval, "is_numerical(complex(0))";
    test_eval, "!is_numerical(string(0))";
    test_eval, "is_numerical(([0]))";
    test_eval, "is_numerical(([char(0)]))";
    test_eval, "is_numerical(([short(0)]))";
    test_eval, "is_numerical(([int(0)]))";
    test_eval, "is_numerical(([long(0)]))";
    test_eval, "is_numerical(([0.0]))";
    test_eval, "is_numerical(([float(0)]))";
    test_eval, "is_numerical(([double(0)]))";
    test_eval, "is_numerical(([complex(0)]))";
    test_eval, "!is_numerical(([string(0)]))";

    /* check for L-value bug. */
    extern _test_vp;
    t1 = 1;
    t2 = 1.0;
    t3 = "hello";
    _test_vp = [&t1, &t2, &t3];
    test_eval, "is_scalar(*_test_vp(1))";
    test_eval, "is_scalar(*_test_vp(2))";
    test_eval, "is_scalar(*_test_vp(3))";
    test_eval, "is_integer(*_test_vp(1))";
    test_eval, "!is_integer(*_test_vp(2))";
    test_eval, "!is_integer(*_test_vp(3))";
    test_eval, "!is_real(*_test_vp(1))";
    test_eval, "is_real(*_test_vp(2))";
    test_eval, "!is_real(*_test_vp(3))";
    test_eval, "is_numerical(*_test_vp(1))";
    test_eval, "is_numerical(*_test_vp(2))";
    test_eval, "!is_numerical(*_test_vp(3))";
    test_eval, "!is_complex(*_test_vp(1))";
    test_eval, "!is_complex(*_test_vp(2))";
    test_eval, "!is_complex(*_test_vp(3))";
    test_eval, "!is_string(*_test_vp(1))";
    test_eval, "!is_string(*_test_vp(2))";
    test_eval, "is_string(*_test_vp(3))";

    TYPE_MIN = [(char(-1) > 0 ?   0 : -128), -32768, -2147483648, -9223372036854775808];
    TYPE_MAX = [(char(-1) > 0 ? 255 :  127),  32767,  2147483647,  9223372036854775807];
    TYPES = tuple(char, short, int, long, float, double, complex);
    for (i = 1; i <= numberof(TYPES); ++i) {
        T = TYPES(i);
        if (T == float || T == double) {
            pfx = (T == float ? "FLT" : "DBL");
            tmin = -(tmax = machine_constant(pfx + "_MAX"));
            test_assert, typemin(T) == tmin, "typemin(%s) == %g", name, tmin;
            test_assert, typemax(T) == tmax, "typemax(%s) == %g", name, tmax;
        } else {
            k = 1 + lround(log(sizeof(T))/log(2));
            name = nameof(T);
            test_assert, typemin(T) == TYPE_MIN(k), "typemin(%s) == %d", name, TYPE_MIN(k);
            test_assert, typemax(T) == TYPE_MAX(k), "typemax(%s) == %d", name, TYPE_MAX(k);
        }
    }
}

if (batch()) {
    test_types;
    test_quick_quartile;
    test_summary;
}
