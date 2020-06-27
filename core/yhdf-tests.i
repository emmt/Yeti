#if 0
plug_dir,".";
include,"./yeti.i";
include,"./yhdf.i";
#endif

func yhd_test_random_0(type, ..)
{
  dimlist = [0];
  while (more_args()) {
    arg = next_arg();
    if (is_void(arg)) continue;
    if ((s=structof(arg))!=long) {
      if (s==char || s==short || s==int) arg=long(arg);
      else error, "bad data type in dimension list";
    }
    if (min(arg) <= 0) error, "invalid null or negative dimension";
    rank = dimsof(arg)(1);
    if (rank == 0) grow, dimlist, arg;
    else if (rank == 1 && numberof(arg) == arg(1)+1) grow, dimlist, arg(2:);
    else error, "invalid dimension list";
  }
  dimlist(1) = numberof(dimlist)-1;
  x = random(dimlist);
  if (type==double) return x;
  if (type==float) return float(x);
  if (type==complex) return x + 1i*random(dimlist);
  scale = 2.0^(8*sizeof(type));
  min_value = (type==char) ? 0.0 : -scale/2.0;
  max_value = scale + min_value - 1.0;
  x = floor(scale*x) + min_value;
  if (min(x) < min_value) error, "bad min value";
  if (max(x) > max_value) error, "bad max value";
  return type(x);
}

func yhd_test_random(type)
{
  MAX_LEN = 10; /* maximum string lenght */
  MAX_RANK = 4;  /* maximum number of dimensions */
  MAX_DIM =  7;  /* maximum dimension lenght */
  rank = long(MAX_RANK*random() + 0.5);
  dimlist = array(rank, rank+1);
  if (rank) dimlist(2:) = long((MAX_DIM-1)*random(rank) + 1.5);
  if (type==string) {
    s = array(string, dimlist);
    len = long(MAX_LEN*random(numberof(s)) + 0.5);
    k = where(len);
    n = numberof(k);
    for (i=1 ; i<=n ; ++i) {
      j = k(i);
      s(j) = string(&char(256*random(len(j))));
    }
    return s;
  }
  x = random(dimlist);
  if (type==double) return x;
  if (type==float) return float(x);
  if (type==complex) return x + 1i*random(dimlist);
  scale = 2.0^(8*sizeof(type));
  min_value = (type==char) ? 0.0 : -scale/2.0;
  max_value = scale + min_value - 1.0;
  x = floor(scale*x) + min_value;
  if (min(x) < min_value) error, "bad min value";
  if (max(x) > max_value) error, "bad max value";
  return type(x);
}

func yhd_test(tmpfilename)
{
  write, "Building some complex hash table...";
  nil = string(0);
  null = pointer(0);
  ptr = [[&yhd_test_random(float), &yhd_test_random(long),
          &yhd_test_random(short), null],
         [&yhd_test_random(char),  &yhd_test_random(long),
          &yhd_test_random(short), &yhd_test_random(string)]];
  a = h_new(f=sin, /* builtin function */
            g=symlink_to_name("foo"),
            x=random(12,7,14), /* array of doubles */
            y="hello",         /* a scalar string */
            /* a hash table */
            z=h_new(msg=[["this",  "is"  , "a"    , "string"  ],
                         ["array", "with", "some" , "elements"],
                         ["and",   "one" , "null" , nil       ],
                         ["and",   "some", "more" , "(3)"     ],
                         ["nulls", nil   , nil    , nil       ]],
                    int_array=yhd_test_random(int),
#if 1
                    complex_array=yhd_test_random(complex),
#endif
                    char_array=yhd_test_random(char),
                    short_array=yhd_test_random(short),
                    long_array=yhd_test_random(long),
                    string_array=yhd_test_random(string),
                    double_array=yhd_test_random(double),
                    float_array=yhd_test_random(float)),
            "this is a more complex member name",0xDEADC0DE,
#if 1
            p=[[&yhd_test_random(float), &yhd_test_random(long)],
               [pointer(0),              &ptr]],
#endif
            "break",933, /* member name is a reserved keyword */
            "",-711,     /* empty member name */
            "void",[]    /* empty member */
            );
  write, "Try with \"native\" encoding...";
  yhd_save, tmpfilename, a, overwrite=1;
  b = yhd_restore(tmpfilename);
  yhd_test_compare, a, b;

  names = ["alpha", "cray", "dec", "i86", "ibmpc", "mac", "macl",
          "sgi64", "sun", "sun3", "vax", "vaxg", "xdr"];
  for (i=1 ; i<=numberof(names) ; ++i) {
    write, "Try with \""+names(i)+"\" encoding...";
    yhd_save, tmpfilename, a, overwrite=1, encoding=names(i);
    b = yhd_restore(tmpfilename);
    yhd_test_compare,a, b;
  }

  remove, tmpfilename;
}

func yhd_test_compare(a, b, name)
{
  if (typeof(a) != typeof(b)) {
    write, format="  *** not same data type for member \"%s\" (%s vs. %s)\n",
      name, typeof(a), typeof(b);
    return;
  }
  if (is_array(a)) {
    if ((da = dimsof(a))(1) != (db = dimsof(b))(1) || anyof(da!=db)) {
      write, format="   *** not same dimension list for member \"%s\"\n", name;
      return;
    }
    s = structof(a);
    if (s==char || s==short || s==int || s==long || s==float ||
        s==double || s==complex || s==string) {
      if (anyof(a != b)) {
        write, format="   *** value(s) differ for member \"%s\"\n", name;
        return;
      }
      return;
    }
    if (s==pointer) {
      n = numberof(a);
      f = name+"(%d)";
      for (i=1 ; i<=n ; ++i) {
        yhd_test_compare, *a(i), *b(i), swrite(format=f, i);
      }
      return;
    }
  } else if (is_hash(a)) {

    prefix = (is_void(name) ? "" : name+"::");
    na = numberof((ka = h_keys(a)));
    nb = numberof((kb = h_keys(b)));
    if (nb) match = array(int, nb);
    for (i=1 ; i<=na ; ++i) {
      key = ka(i);
      name = prefix+key;
      if (h_has(b, key)) {
        yhd_test_compare, h_get(a, key), h_get(b, key), name;
        if (nb) match(where(key == kb)) = 1;
      } else {
        write, format="   *** missing member \"%s\" in B\n", name;
      }
    }
    if (nb) {
      missing = kb(where(!match));
      f = "   *** missing member \""+prefix+"%s\" in A\n"
      for (i=1 ; i<=numberof(missing) ; ++i) write, format=f, missing(i);
    }
  } else if (is_func(a)) {
    if (a != b) {
      write, format="   *** value(s) differ for member \"%s\"\n", name;
      return;
    }
  } else if (is_symlink(a)) {
    if (name_of_symlink(a) != name_of_symlink(b)) {
      write, format="   *** value(s) differ for member \"%s\"\n", name;
      return;
    }
  } else if (! is_void(a)) {
    write, format="   *** unexpected type for member \"%s\" (%s)\n",
      name, typeof(a);
  }
}
