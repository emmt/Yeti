require, "yeti-testing.i";

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

func test_tuples(nil)
{
    test_eval, "tuple()() == 1";
    test_eval, "empty_tuple()() == 0";
    test_eval, "is_void(tuple()(1))";
    test_eval, "is_tuple(empty_tuple())";
    test_eval, "is_tuple(tuple())";
    test_eval, "tuple(\"a\",1,[2.0,3.0])() == 3";
    test_eval, "tuple(\"a\",1,[2.0,3.0])(1) == \"a\"";
    test_eval, "tuple(\"a\",1,[2.0,3.0])(2) == 1";
    test_eval, "allof(tuple(\"a\",1,[2.0,3.0])(3) == [2.0,3.0])";
    test_eval, "allof(tuple(\"a\",1,[2.0,3.0])(0) == [2.0,3.0])";
    test_eval, "tuple(\"a\",1,[2.0,3.0])(-1) == 1";

    dbg = debug_refs();
    tup = tuple(dbg, dbg);
    test_eval, "dbg.nrefs == 3";
    dbg = [];
    test_eval, "tup(1).nrefs == 2";
    dbg = tup(2);
    test_eval, "dbg.nrefs == 3";
    tup = [];
    test_eval, "dbg.nrefs == 1";
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

    extern _test_eps, _test_min, _test_max;
    for (i = 1; i <= 2; ++i) {
        T = (i == 1 ? float : double);
        p = (i == 1 ? "FLT_" : "DBL_");
        n = nameof(T);
        test_eval, "structof(machine_constant(\""+p+"EPSILON\")) == "+n;
        test_eval, "structof(machine_constant(\""+p+"MIN\")) == "+n;
        test_eval, "structof(machine_constant(\""+p+"MAX\")) == "+n;
        test_eval, "structof(machine_constant(\""+p+"MIN_10_EXP\")) == long";
        test_eval, "structof(machine_constant(\""+p+"MAX_10_EXP\")) == long";
        test_eval, "structof(machine_constant(\""+p+"MANT_DIG\")) == long";
        test_eval, "structof(machine_constant(\""+p+"DIG\")) == long";
        _test_eps = machine_constant(p+"EPSILON");
        _test_min = machine_constant(p+"MIN");
        _test_max = machine_constant(p+"MIN");
        test_eval, "1 + _test_eps > 1";
        test_eval, "1 - _test_eps < 1";
        test_eval, "1 + _test_eps/2 == 1";
        test_eval, "1 - _test_eps/4 == 1";
        test_eval, "_test_min*(1 - _test_eps) == 0";
        //test_eval, "_eps_max*(1 + _test_eps) == +Inf";
    }

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

func test_mixed_vectors(nil)
{
    v = mvect_create(0);
    test_eval, "v.len == 0";
    test_eval, "v() == 0";
    test_eval, "is_mvect(v)";
    for (i=1;i<=5;++i) {
        mvect_push, v, i;
    }
    test_eval, "v.len == 5";
    for (i=v.len+1; i <= 64; ++i) {
        mvect_push, v, i;
    }
    test_eval, "v.len == 64";
    a = array(long, v.len);
    for (i=1; i<=v.len; ++i) {
        a(i) = v(i);
    }
    test_eval, "allof(a == indgen(v.len))";
    test_eval, "mvect_resize(v,37) == v";
    test_eval, "v.len == 37";
    a = array(long, v.len);
    for (i=1; i<=v.len; ++i) {
        a(i) = v(i);
    }
    test_eval, "allof(a == indgen(v.len))";
    test_eval, "v(0) == a(0)";
    test_eval, "v(-1) == a(-1)";
    oldlen = v.len;
    newlen = 71;
    test_eval, "mvect_resize(v,newlen).len == newlen";
    flag = 1n;
    for (i=oldlen+1; i<=newlen; ++i) {
        flag &= is_void(v(i));
    }
    test_eval, "flag == 1n";
    dbg = debug_refs();
    v = mvect_collect(dbg, 1, dbg, 2);
    test_eval, "dbg.nrefs == 3";
    dbg = [];
    test_eval, "v(1).nrefs == 2";
    test_eval, "v(3).nrefs == 2";
    dbg = v(1);
    test_eval, "dbg.nrefs == 3";
    v, 3, pi;
    test_eval, "v(3) == pi";
    test_eval, "dbg.nrefs == 2";
    test_eval, "mvect_store(v, 4, dbg) == 2";
    test_eval, "dbg.nrefs == 3";
    test_eval, "mvect_push(v, dbg, dbg, dbg, dbg).len == 8";
    test_eval, "dbg.nrefs == 7";
    test_eval, "mvect_store(v, 1, pi) == dbg";
    test_eval, "dbg.nrefs == 6";
    test_eval, "mvect_resize(v, v.len - 2).len == 6";
    test_eval, "dbg.nrefs == 4";
    v = [];
    test_eval, "dbg.nrefs == 1";
}

if (batch()) {
    test_tuples;
    test_types;
    test_mixed_vectors;
    test_quick_quartile;
    test_summary;
}
