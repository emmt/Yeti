/*
 * yeti.i -
 *
 * Main startup file for Yeti (an extension of Yorick).
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

if (is_func(plug_in) && is_func(yeti_init) != 2) plug_in, "yeti";

local YETI_HOME, YETI_VERSION, YETI_VERSION_MAJOR, YETI_VERSION_MINOR;
local YETI_VERSION_MICRO, YETI_VERSION_SUFFIX;
extern yeti_init;
/* DOCUMENT YETI_HOME           the directory where Yeti is installed
         or YETI_VERSION        version of current Yeti interpreter (string)
         or YETI_VERSION_MAJOR  major Yeti version number (integer)
         or YETI_VERSION_MINOR  minor Yeti version number (integer)
         or YETI_VERSION_MICRO  micro Yeti version number (integer)
         or YETI_VERSION_SUFFIX suffix Yeti version number (string, e.g. "pre1")
         or yeti_init;
         or yeti_init();

     YETI_VERSION and YETI_HOME are global variables predefined by Yeti to
     store its version number (as "MAJOR.MINOR.MICROSUFFIX") and installation
     directory (e.g. "/usr/local/lib/yeti-VERSION").  In YETI_VERSION, a
     non-empty suffix like "x" or "pre1" indicates a development version.

     The function yeti_init can be used to restore the values of YETI_VERSION
     and YETI_HOME.  When called as a function, yeti_init() returns Yeti
     version as a string.

     If Yeti is loaded as a plugin, YETI_HOME is left undefined and no path
     initialization is performed.  Otherwise, the first time yeti_init is
     called (this is automatically done at Yeti startup), it set the default
     path list for Yeti applications.

     A convenient way to check if your script is parsed by Yeti is to do:

       if (is_func(yeti_init) == 2) {
         // we are in Yeti
         ...
       } else {
         // not in Yeti
         ...
       }

   SEE ALSO: Y_LAUNCH, Y_HOME, Y_SITE, Y_VERSION,
             get_path, set_path.
 */
if (batch()) {
  yeti_init;
} else {
  write, format=" Yeti %s ready.  Copyright (c) 1996-2018, Eric THIEBAUT.\n",
    yeti_init();
}

func setup_package(plugname)
/* DOCUMENT PACKAGE_HOME = setup_package();
         or PACKAGE_HOME = setup_package(plugname);

     The setup_package function must be directly called in a Yorick source
     file, the so-called Yorick package source file.  This function determines
     the package directory which is the absolute directory name of the package
     source file and setup Yorick search paths to include this directory.  The
     returned value is the package directory (guaranteed to be terminated by a
     slash "/").

     If PLUGNAME is specified, the corresponding plugin is loaded
     (preferentially from the package directory).


   SEE ALSO: plug_in, plug_dir, current_include, get_path, set_path.
 */
{
  /* Quick check. */
  path = current_include();
  if (is_void(path)) {
    error, "setup_package must be called from a Yorick source file";
  }

  /* Figure out the absolute directory from where we are called. */
  cwd = cd(".");
  j = strfind("/", path, back=1)(2);
  if (j > 0) {
    pkgdir = cd(strpart(path, 1:j));
    cd, cwd;
  } else {
    pkgdir = cwd;
  }
  if (is_void(pkgdir)) {
    error, "bad path for include file: \"" + path + "\"";
  }
  if (strpart(pkgdir, 0:0) != "/") {
    pkgdir += "/";
  }

  /* Setup Yorick search path. */
  list = get_path();
  if (! strlen(list)) {
    list = [];
    flag = 1n;
  } else {
    c = strchar(list);
    j = where(c == ':');
    if (is_array(j)) {
      c(j) = 0;
      list = strchar(c);
    }
    found = (list == pkgdir);
    if (noneof(found)) {
      flag = 1n;
    } else if (! found(1) || sum(found) > 1) {
      flag = 1n;
      list = list(where(! found));
    } else {
      flag = 0n; /* no need to add PKGDIR */
    }
  }
  if (flag) {
    set_path, (numberof(list) ? pkgdir + sum(":" + list) : pkgdir);
  }

  /* Setup list of directories for plugins so that the package directory is
     searched first and load package plugin. */
  if (! is_void(plugname) && is_func(plug_in)) {
    list = plug_dir();
    if (is_void(list)) {
      plug_dir, pkgdir;
    } else {
      /* move directory in first position */
      plug_dir, grow(pkgdir, list(where(list != pkgdir)));
    }
    plug_in, plugname;
  }

  return pkgdir;
}

func anonymous(args, code)
/* DOCUMENT f = anonymous(args, code);

     Make an anonymous function with ARGS its argument list and CODE the body
     of the function.  ARGS and CODE must be (array of) strings.

     For instance:

       f1 = anonymous("x", "c = x*x; return sqrt(c + abs(x));");
       f2 = anonymous("x,y", "return cos(x*y + abs(x));");

     define two functions f1 and f2 which take respectively one and two
     arguments.  When variables f1 and f2 get out of scope the function
     definition is automatically deleted.

     Other example:

       a = _lst(12,34,67);
       b = map(anonymous("x", "return sin(x);"), a);

     B is a list with its elements the sines of the elements of A.

   SEE ALSO: map, include, funcdef, h_functor, closure.
 */
{
  local __lambda__;
  include, grow("func __lambda__(", args, ") { ", code, " }"), 1;
  return __lambda__;
}

/*---------------------------------------------------------------------------*/
/* SORTING */

extern heapsort;
/* DOCUMENT heapsort(a)
         or heapsort, a;
     When called as a function, returns a vector of numberof(A) longs
     containing index values such that A(heapsort(A)) is a monotonically
     increasing vector.  When called as a subroutine, performs in-place
     sorting of elements of array A.  This function uses the heap-sort
     algorithm which may be superior to the quicksort algorithm (for
     instance for integer valued arrays).  Beware that headpsort(A) and
     sort(A) differ for multidimensional arrays.

   SEE ALSO: quick_select, sort. */

extern quick_select;
/* DOCUMENT quick_select(a, k [, first, last])
         or quick_select, a, k [, first, last];

     Find the K-th smallest element in array A.  When called as a function,
     the value of the K-th smallest element in array A is returned.  When
     called as a subroutine, the elements of A are re-ordered (in-place
     operation) so that A(K) is the K-th smallest element in array A and
     A(J) <= A(K) for J <= K and A(J) >= A(K) for J >= K.

     Optional arguments FIRST and LAST can be used to specify the indices of
     the first and/or last element of A to consider: elements before FIRST and
     after LAST are ignored and left unchanged when called as a subroutine;
     index K however always refers to the full range of A.  By default,
     FIRST=1 and LAST=numberof(A).

     Yorick indexing rules are supported for arguments K, FIRST and LAST
     (i.e. 0 means last element, etc).


   EXAMPLES

     The index K which splits a sample of N=numberof(A) elements into
     fractions ALPHA (strictly before K, that is K - 1 elements) and 1 - ALPHA
     (strictly after K, that is N - K elements) is such that:

         (1 - ALPHA)*(K - 1) = ALPHA*(N - K)

     hence:

         K = 1 + ALPHA*(N - 1)

     Accounting for rounding to nearest integer, this leads to compute the
     value at the boundary of the split as:

         q(ALPHA) = quick_select(A, long(1.5 + ALPHA*(numberof(A) - 1)))

     Therefore the first inter-quartile split is at (1-based and rounded
     to the nearest integer) index:

         K1 = (N + 5)/4     (with integer division)

     the second inter-quartile (median) is at:

         K2 = N/2 + 1       (with integer division)

     the third inter-quartile is at:

         K3 = (3*N + 3)/4   (with integer division)


   SEE ALSO: quick_median, quick_quartile, sort, heapsort.
 */

func quick_median(a)
/* DOCUMENT quick_median(a)
     Returns the median of values in array A.

   SEE ALSO
     median, quick_quartile, quick_select, insure_temporary.
 */
{
  n = numberof(a);
  k = (n + 1)/2;
  if (n % 2) {
    /* odd number of elements */
    return quick_select(a, k);
  } else {
    /* even number of elements */
    insure_temporary, a;
    quick_select, a, k;
    return (double(a(k)) + a(min:k+1:n))/2.0;
  }
}

local quick_interquartile_range;
func quick_quartile(a)
/* DOCUMENT q = quick_quartile(a);
         or iqr = quick_interquartile_range(a);

     The function quick_quartile() returns the 3 quartiles of the values in
     array A.

     The function quick_interquartile_range() returns IQR = Q(3) - Q(1), the
     interquartile range of values in array A.

     Linear interpolation is used to estimate the value of A at fractional
     orders.  Array A must have at least 3 elements.


   SEE ALSO
     quick_median, quick_select, insure_temporary.
 */
{
  /* Check argument and prepare for in-place operation. */
  if ((n = numberof(a)) <= 2) {
    error, "expecting an array with at least 3 elements";
  }
  insure_temporary, a;
  q = array(double, 3);

  /* The 1st interquartile is at fractional index (n + 2)/4. */
  k1 = (p = n + 2)/4;
  quick_select, a, k1;
  if ((r = (p & 3)) == 0) {
    q(1) = double(a(k1));
  } else {
    ++k1;
    quick_select, a, k1, k1;
    u = r/4.0;
    q(1) = (1.0 - u)*a(k1 - 1) + u*a(k1);
  }

  /* The median (2nd interquartile) is at fractional index (n + 1)/2. */
  k2 = (p = n + 1)/2;
  if (k2 > k1) {
    quick_select, a, k2, k1 + 1;
  }
  if ((p & 1) == 0) {
    q(2) = double(a(k2));
  } else {
    ++k2;
    quick_select, a, k2, k2;
    q(2) = (double(a(k2 - 1)) + a(k2))/2.0;
  }

  /* The 3rd interquartile is at fractional index (3*n + 2)/4. */
  k3 = (p = 3*n + 2)/4;
  if (k3 > k2) {
    quick_select, a, k3, k2 + 1;
  }
  if ((r = (p & 3)) == 0) {
    q(3) = double(a(k3));
  } else {
    ++k3;
    quick_select, a, k3, k3;
    u = r/4.0;
    q(3) = (1.0 - u)*a(k3 - 1) + u*a(k3);
  }

  return q;
}

func quick_interquartile_range(a)
{
  /* Check argument and prepare for in-place operation. */
  if ((n = numberof(a)) <= 2) {
    error, "expecting an array with at least 3 elements";
  }
  insure_temporary, a;

  /* The 1st interquartile is at fractional index (n + 2)/4. */
  k1 = (p = n + 2)/4;
  quick_select, a, k1;
  if ((r = (p & 3)) == 0) {
    q1 = double(a(k1));
  } else {
    ++k1;
    quick_select, a, k1, k1;
    u = r/4.0;
    q1 = (1.0 - u)*a(k1 - 1) + u*a(k1);
  }

  /* The 3rd interquartile is at fractional index (3*n + 2)/4. */
  k3 = (p = 3*n + 2)/4;
  if (k3 > k1) {
    quick_select, a, k3, k1 + 1;
  }
  if ((r = (p & 3)) == 0) {
    q3 = double(a(k3));
  } else {
    ++k3;
    quick_select, a, k3, k3;
    u = r/4.0;
    q3 = (1.0 - u)*a(k3 - 1) + u*a(k3);
  }

  return q3 - q1;
}

/*---------------------------------------------------------------------------*/
/* SYMBOLIC LINKS */

extern symlink_to_variable;
extern symlink_to_name;
extern is_symlink;
extern name_of_symlink;
extern value_of_symlink;
/* DOCUMENT lnk = symlink_to_variable(var)
         or lnk = symlink_to_name(varname)
         or is_symlink(lnk)
         or name_of_symlink(lnk)
         or value_of_symlink(lnk)

     The call symlink_to_variable(var) creates a symbolic link to variable
     VAR.  The call symlink_to_name(varname) creates a symbolic link to
     variable whose name is VARNAME.  When the link object LNK is used in an
     'eval' context or a 'get member' context (see examples below), LNK gets
     replaced 'on the fly' by the symbol which is actually stored into the
     corresponding Yorick's variable.  Therefore LNK adds no additional
     reference to the variable which only has to exist when LNK is later used.
     This functionality can be used to implement 'virtual' methods for
     pseudo-object in Yorick (using hash tables).

     For instance:

        > lnk = symlink_to_variable(foo);  // variable foo does not yet exists
        > lnk = symlink_to_name("foo");    // same link, using a name
        > func foo(x) { return 2*x; }
        > lnk(9)
         18
        > func foo(x) { return 3*x; }
        > lnk(9)
         27

        > z = array(complex, 10, 4);
        > lnk = symlink_to_variable(z);
        > info, lnk.re;
         array(double,10,4)

     The function is_symlink(LNK) check whether LNK is a symbolic link.

     The function name_of_symlink(LNK) returns the name of the variable linked
     by LNK.

     The function value_of_symlink(LNK) returns the actual value of the
     variable corresponding to the symbolic link LNK.  This function can be
     used to force the substitution in a context where it is not automatically
     done.  For instance:

       > lnk = symlink_to_variable(a);
       > a = random(10);
       > avg(lnk)
       ERROR (*main*) avg requires numeric argument
       > avg(value_of_symlink(lnk))
       0.383679
       > avg(a)
       0.383679


   SEE ALSO: h_new.
 */

/*---------------------------------------------------------------------------*/
/* TUPLE-LIKE OBJECTS */

extern tuple;
/* DOCUMENT tup = tuple(arg1, arg2, ...);

     Yields a lightweight tuple-like object collecting references to items
     ARG1, ARG2, ...  A tuple instance can be indexed to retrieve the different
     items:

         tup(i) __________ yields i-th item using Yorick's conventions that
                           i <= 0 refers to the end of the range;
         tup() ___________ yields the number of items.

     Tuples may be used to get around the limitation that Yorick functions can
     only return a single value.

   SEE ALSO is_tuple, _lst.
*/

extern is_tuple;
/* DOCUMENT is_tuple(obj)
     Yield whethet object OBJ is a tuple.

   SEE ALSO tuple.
*/

/*---------------------------------------------------------------------------*/
/* HASH TABLE OBJECTS */

extern h_debug;
/* DOCUMENT h_debug, object, ...
     Print out some debug information on OBJECT.

     ****************************
     *** WILL BE REMOVED SOON ***
     ****************************/

extern h_new;
/* DOCUMENT h_new();
         or h_new(key=value, ...);
         or h_new("key", value, ...);

     Returns a new hash table with member(s) KEY set to VALUE.  There may be
     any number of KEY-VALUE pairs. A particular member of a hash table TAB
     can be specified as a scalar string, i.e.  "KEY", or using keyword
     syntax, i.e. KEY=.  The keyword syntax is however only possible if KEY is
     a valid Yorick's symbol name.  VALUE can be anything (even a non-array
     object).

     h_save and h_functor (which to see) provide alternative means to create
     hash table object.

     yhd_save and yhd_restore (which to see) let you save and restore hash
     tables to data files.

     A hash table can be used to implement some kind of object-oriented
     abstraction in Yorick.  However, in Yorick, a hash table must have a
     simple tree structure -- no loops or rings are allowed (loops break
     Yorick's memory manager -- beware).  You need to be careful not to do
     this as the error will not be detected.

     The difference between a hash table and a list object is that items are
     retrieved by key identifier rather than by order (by h_get, get_member or
     dot dereferenciation).  It is possible to dereference the contents of TAB
     using the dot operator (as for a structure) or the get_member function.
     For instance, it is legal to do:

        tab = h_new(x=span(-7,7,100), name="my name", op=sin, scale=33);
        plg, tab.op(tab.x), tab.x;

     but the member must already exists and there are restrictions to
     assignation, i.e. only contents of array members can be assigned:

        tab.name() = "some other string"; // ok
        tab.name   = "some other string"; // error
        tab.x(RANGE_OR_INDEX) = EXPR;     // ok if conformable AND member X
                                          // is not a 'fast' scalar (int,
                                          // long or double scalar)
        tab.x                 = EXPR;     // error

     and assignation cannot therefore change the dimension list or data type
     of a hash table member.  Redefinition/creation of a member can always be
     performed with the h_set function which is the recommended method to set
     the value of a hash table member.

     Hash tables behave differently depending how they are used:

        tab.key      - de-reference hash member
        tab("key")   - returns member named "key" in hash table TAB, this is
                       exactly the same as: h_get(tab, "key")
        tab()        - returns number of elements in hash table TAB

        tab(i)       - returns i-th member in hash table TAB; i is a scalar
                       integer and can be less or equal zero to start from the
                       last one; if the hash table is unmodified, tab(i) is
                       the same as tab(keys(i)) where keys=h_keys(tab) --
                       beware that this is very inefficient way to access the
                       contents of a hash table and will probably be removed
                       soon.

     However, beware that the behaviour of calls such that TAB(...) may be
     changed if the has table implements its own "evaluator" (see
     h_evaluator).

     For instance, to explore the whole hash table, there are different
     possibilities:

        keys = h_keys(tab);
        n = numberof(keys);   // alternatively: n = tab()
        for (i = 1; i <= n; ++i) {
          a = tab(keys(i));
          ...;
        }

     or:

        for (key = h_first(tab); key; key = h_next(tab, key)) {
          a = tab(key);
          ...;
        }

     or:

        n = tab();
        for (i=1 ; i<=n ; ++i) {
          a = tab(i);
          ...;
        }

     the third form is slower for large tables and will be made obsolete
     soon.

     An important point to remember when using hash table is that hash members
     are references to their contents, i.e.

        h_set, hash, member=x;

     makes an additional reference to array X and does not copy the array
     although you can force that, e.g.:

        tmp = x;                   // make a copy of array X
        h_set, hash, member=tmp;   // reference copy in hash table
        tmp = [];                  // delete one reference to the copy

     Because assignation result is its rhs (right-hand-side), you cannot do:

        h_set, hash, member=(tmp = x);   // assignation result is X

     Similarly, unlike Yorick array data types, a statement like x=hash does
     not make a copy of the hash table, it merely makes an additional
     reference to the list.


   CAVEATS:
     In Yorick (or Yeti), many objects can be used to reference other objects:
     pointers, lists and hash tables.  Since Yorick uses a simple reference
     counter to delete unused object, cyclic references (i.e.  an object
     referencing itself either directly or indirectly) result in objects that
     will not be properly deleted.  It is the user reponsibility to create no
     cyclic references in order to avoid memory leaks.  Checking a potential
     (or effective) cyclic reference would require recursive investigation of
     all members of the parent object and could be very time consuming.

   SEE ALSO: h_save, h_copy, h_get, h_has, h_keys, h_pop, h_set, h_stat,
             h_first, h_next, yhd_save, yhd_restore, _lst, h_functor,
             get_member. */

extern h_get;
/* DOCUMENT h_get(tab, key=);
         or h_get(tab, "key");
     Returns the value of member KEY of hash table TAB.  If no member KEY
     exists in TAB, nil is returned.  h_get(TAB, "KEY") is identical to
     get_member(TAB, "KEY") and also to TAB("KEY").

   SEE ALSO h_new, get_member. */

extern h_set;
/* DOCUMENT h_set, tab, key=value, ...;
         or h_set, tab, "key", value, ...;
     Stores VALUE in member KEY of hash table TAB.  There may be any number of
     KEY-VALUE pairs.  If called as a function, the returned value is TAB.

   SEE ALSO h_new, h_set_copy. */

func h_set_copy(tab, ..)
/* DOCUMENT h_set_copy, tab, key, value, ...;
     Set member KEY (a scalar string) of hash table TAB with VALUE.  Unlike
     h_set, VALUE is duplicated if it is an array.  There may be any number of
     KEY-VALUE pairs.

   SEE ALSO h_copy, h_new, h_set. */
{
  while (more_args()) {
    key = next_arg();
    value = next_arg();
    h_set, tab, key, value;
  }
  return tab;
}

func h_copy(tab, recursively)
/* DOCUMENT h_copy(tab);
         or h_copy(tab, recursively);
     Effectively copy contents of hash table TAB into a new hash table that is
     returned.  If argument RECURSIVELY is true, every hash table contained
     into TAB get also duplicated.  This routine is needed because doing
     CPY=TAB, where TAB is a hash table, would only make a new reference to
     TAB: CPY and TAB would be the same object.

   SEE ALSO h_new, h_set, h_clone. */
{
  key_list = h_keys(tab);
  n = h_number(tab); /* number of members */
  new = h_new();
  h_evaluator, new, h_evaluator(tab);
  if (recursively) {
    for (i=1 ; i<=n ; ++i) {
      key = key_list(i);
      member = h_get(tab, key);
      h_set, new, key, (is_hash(member) ? h_copy(member, 1) : member);
    }
  } else {
    for (i=1 ; i<=n ; ++i) {
      key = key_list(i);
      member = h_get(tab, key);
      h_set, new, key, member;
    }
  }
  return new;
}

/*
 * NOTE: h_clone(tab, copy=1)           is the same as h_copy(tab)
 *       h_clone(tab, copy=1, depth=-1) is the same as h_copy(tab, 1)
 */
func h_clone(tab, copy=, depth=)
/* DOCUMENT h_clone(tab, copy=, depth=);
     Make a new hash table with same contents as TAB.  If keyword COPY is
     true, a fresh copy is made for array members.  Otherwise, array members
     are just referenced one more time by the new hash table.  If keyword
     DEPTH is non-zero, every hash table referenced by TAB get also cloned
     (this is done recursively) until level DEPTH has been reached (infinite
     recursion if DEPTH is negative).  The value of keyword COPY is kept the
     same across the recursions.

   SEE ALSO h_new, h_set, h_copy. */
{
  local member;
  key_list = h_keys(tab); /* list of hash keys */
  n = h_number(tab); /* number of members */
  new = h_new();
  h_evaluator, new, h_evaluator(tab);
  if (depth) {
    --depth;
    for (i=1 ; i<=n ; ++i) {
      key = key_list(i);
      if (copy) member = h_get(tab, key);
      else eq_nocopy, member, h_get(tab, key);
      h_set, new, key,
        (is_hash(member) ? h_clone(member, copy=copy, depth=depth) : member);
    }
  } else if (copy) {
    for (i=1 ; i<=n ; ++i) {
      key = key_list(i);
      member = h_get(tab, key);
      h_set, new, key, member;
    }
  } else {
    for (i=1 ; i<=n ; ++i) {
      key = key_list(i);
      h_set, new, key, h_get(tab, key);
    }
  }
  return new;
}

extern h_number;
/* DOCUMENT h_number(tab);
     Returns number of entries in hash table TAB.

   SEE ALSO h_new, h_keys. */

extern h_keys;
/* DOCUMENT h_keys(tab);
     Returns list of members of hash table TAB as a string vector of key
     names.  The order in which keys are returned is arbitrary.

   SEE ALSO h_new, h_first, h_next, h_number. */

extern h_has;
/* DOCUMENT h_has(tab, "key");
         or h_has(tab, key=);
     Returns 1 if member KEY is defined in hash table TAB, else 0.

   SEE ALSO h_new. */

extern h_first;
extern h_next;
/* DOCUMENT h_first(tab);
         or h_next(tab, key);
     Get first or next key in hash table TAB.  A NULL string is returned if
     key is not found or if it is the last one (for h_next).  Thes routines
     are useful to run through all entries in a hash table (however beware
     that the hash table should be left unchanged during the scan).  For
     instance:

       for (key = h_first(tab); key; key = h_next(tab, key)) {
         value = h_get(tab, key);
         ...;
       }

   SEE ALSO h_new, h_keys. */

extern h_evaluator;
/* DOCUMENT h_evaluator(obj)
         or h_evaluator(obj, evl);
         or h_evaluator, obj, evl;

     Set/query evaluator function of hash table OBJ.  When called as a
     function, the evaluator of OBJ prior to any change is returned as a
     scalar string.  If EVL is specified, it becomes the new evaluator of OBJ.
     EVL must be a scalar string (the name of the evaluator function), or a
     function, or nil.  If EVL is explicitely nil (for instance []) or a
     NULL-string (for instance string(0)), the default behaviour is restored.

     When hash table OBJ is used as:

       OBJ(...)

     where "..." represents any list of arguments (including none) then its
     evaluator get called as:

       EVL(OBJ, ...)

     that is with OBJ prepended to the same argument list.


   EXAMPLES:
     // create a hash table:
     obj = h_new(data=random(200), count=0);

     // define a fucntion:
     func eval_me(self, incr)
     {
        if (incr) h_set, self, count = (self.count + incr);
        return self.data(1 + abs(self.count)%200);
     }

     // set evaluator (which must be already defined as a function):
     h_evaluator, obj, eval_me;

     obj(49);   // return 49-th value
     obj();     // return same value
     obj(3);    // return 51-th value
     h_evaluator, obj, []; // restore standard behaviour

     // set evaluator (not necessarily already defined as a function):
     h_evaluator, obj, "some_name";

     // then define the function code prior to use:
     func some_name(self, a, b) { return self.count; }


   SEE ALSO: h_new, h_get.
 */

func h_save(args)
/* DOCUMENT tbl = h_save(var1, var2, ...);
         or h_save, tbl, var1, var2, ...;

     Save  variables VAR1,  VAR2, ...  into an  hash table.   When called  as a
     function, the resulting  new hash table is returned by  the function; when
     called as  a subroutine,  the first  argument, TBL, must  be a  hash table
     whose contents gets updated.

     The VARi arguments may be:

      - a  simple variable  reference,  in  which case  the  name  of the  VARi
        specifies the key in the hash table;

      - a (KEY,VAL)  pair where  KEY is the  name of the  hash entry  (a scalar
        string) and VAL is  the value of the hash entry; as  a special case, if
        KEY is the  NULL string, string(0), the corresponding value  is used to
        set the evaluator of the hash table;

      - a keyword KEY=VAL;

     Note that positional  arguments are processed in order,  and the keywords,
     if any, are processed last; if multiple arguments are stored with the same
     key, the  final value will  be the last  one (according to  the processing
     order).


   PERFORMANCES:
     Currently,  this  function is  implemented  by  interpreted code.   It  is
     however quite  fast: about  1.5 microseconds  to create  a table  with one
     entry, plus about  0.6 microsecond per additional  entry.  For comparison,
     it takes 0.3  microsecond to create a single entry  table with h_new() and
     about  0.06  microsecond  per  additional entry.   All  these  times  were
     measured on an Intel Core i7-870 at 2.93GHz.


   SEE ALSO: h_new, h_evaluator, h_functor, save. */
{
  /* Get the keywords and the number of positional arguments. */
  keys = args(-);
  nargs = args(*);
  nkeys = numberof(keys);

  /* Create/fetch the hash table and process positional arguments. */
  local obj, key;
  if (am_subroutine()) {
    if (nargs >= 1) eq_nocopy, obj, args(1);
    if (! is_hash(obj)) {
      error, ("expecting a hash table object as first argument " +
              "when called as a subroutine");
    }
    i = 1;
  } else {
    obj = h_new();
    i = 0;
  }
  while (++i <= nargs) {
    eq_nocopy, key, args(-,i);
    if (! key) {
      /* Argument is not a simple variable reference; then, the key is the
         value of the current positional argument and the the value is that of
         the next positional argument. */
      eq_nocopy, key, args(i);
      if (! is_string(key) || ! is_scalar(key)) {
        error, "expecting key name or variable reference";
      }
      if (++i > nargs) {
        error, "missing value after last key name in argument list";
      }
      if (! key) {
        h_evaluator, obj, args(i);
        continue;
      }
    }
    h_set, obj, key, args(i);
  }

  /* Process keywords. */
  for (i = 1; i <= nkeys; ++i) {
    key = keys(i);
    h_set, obj, key, args(key);
  }
  return obj;
}
//errs2caller, h_save;
wrap_args, h_save;

func h_functor(args)
/* DOCUMENT obj = h_functor(fn, ..., var, ..., key=val, ..., "key", val, ...);

     This function creates  a functor object OBJ (actually a  hash table) which
     calls function FN with itself prepended to its argument list:

         obj(arg1, arg2, ...)

     is the same as:

         fn(obj, arg1, arg2, ...)

     First positional  argument FN specify  the function to  call, it can  be a
     name or any object callable as a function (including another functor or an
     anonymous function, see anonymous).

     Any  other arguments  (either positional  ones  or keywords)  are used  to
     populate the created  object with hash entries.  Any given  keyword -- say
     KEY=VAL -- is stored  into the returned object with "KEY"  as its name and
     VAL as its value.   Any simple variable reference -- say  VAR -- is stored
     with name "VAR" and the contents of VAR as value.  Finally, pairs of names
     and values -- say "KEY",VAL -- are stored with name "KEY" and value VAL.

     If you  want to  use the  contents of  a variable  (not the  variable name
     itself) as the  name of an entry,  just turn it into  an expression, e.g.,
     noop(VAR) or VAR+"" will do the trick.

     See h_save() for performance issues if that matters to you.


   SEE ALSO: h_new, h_evaluator, h_save, anonymous, closure, wrap_args, noop.
 */
{
  /* Get the keywords and the number of positional arguments. */
  keys = args(-);
  nargs = args(*);
  nkeys = numberof(keys);

  /* Get the functions. */
  local fn;
  if (nargs >= 1) eq_nocopy, fn, args(1);
  if (is_void(fn)) {
    error, "invalid or missing function";
  }
  obj = h_new();

  /* Add hash table entries from positional arguments. */
  local key;
  for (i = 2; i <= nargs; ++i) {
    eq_nocopy, key, args(-,i);
    if (! key) {
      /* Argument is not a simple variable reference; then, the key is the
         value of the current positional argument and the the value is that of
         the next positional argument. */
      eq_nocopy, key, args(i);
      if (! is_string(key) || ! is_scalar(key)) {
        error, "expecting attribute name or variable reference";
      }
      if (++i > nargs) {
        error, "missing value for last attribute";
      }
    }
    if (h_has(obj, key)) {
      error, ("attribute \""+key+"\" already exists");
    }
    h_set, obj, key, args(i);
  }

  /* Add hash table entries from keywords. */
  for (k = 1; k <= nkeys; ++k) {
    key = keys(k);
    if (h_has(obj, key)) {
      error, ("attribute \""+key+"\" already exists");
    }
    h_set, obj, key, args(key);
  }
  h_evaluator, obj, fn;
  return obj;
}
wrap_args, h_functor;

func h_info(tab, align)
/* DOCUMENT h_info, tab;
         or h_info, tab, align;
     List contents of hash table TAB in alphabetical order of keys.  If second
     argument is true, the key names are right aligned.

   SEE ALSO: h_new, h_keys, h_first, h_next, h_show, sort. */
{
  key_list = h_keys(tab);
  if (is_void(key_list)) return;
  key_list = key_list(sort(key_list));
  n = numberof(key_list);
  width = max(strlen(key_list));
  format = swrite(format=(align?"%%%ds":"%%-%ds"), width + 1);
  for (i=1 ; i<=n ; ++i) {
    key = key_list(i);
    write, format=format, key+":";
    info, h_get(tab, key);
  }
}

local __h_show_worker;
func h_show(tab, prefix=, maxcnt=, depth=)
/* DOCUMENT h_show, tab;
     Display contents of hash table TAB in a tree-like representation.
     Keyword PREFIX can be used to prepend a prefix to the printed lines.
     Keyword MAXCNT (default 5) can be used to specify the maximum number of
     elements for printing array values.

   SEE ALSO: h_show_style, h_info, h_keys. */
{
  __h_show_maxcnt = (is_void(maxcnt) ? 5 : maxcnt);
  __h_show_worker, tab, , (is_void(prefix) ? "" : prefix), 0;
}

func __h_show_worker(obj, name, prefix, stage)
{
  local prefix1, prefix2;
  if (! name) {
    name = "<top>";
  } else if (strlen(name) == 0) {
    name = "\"\"";
  } else if (strgrep("^[_A-Za-z][0-9_A-Za-z]*$", name)(2) < 0) {
    name = print(name)(sum);
  }
  if (stage == 1) {
    prefix1 = prefix + __h_show_pfx_a;
    prefix2 = prefix + __h_show_pfx_b;
  } else if (stage == 2) {
    prefix1 = prefix + __h_show_pfx_c;
    prefix2 = prefix + __h_show_pfx_d;
  } else {
    eq_nocopy, prefix1, prefix;
    eq_nocopy, prefix2, prefix;
  }
  if (is_hash(obj)) {
    key_list = h_keys(obj);
    if (is_array(key_list)) {
      key_list = key_list(sort(key_list));
      //width = max(strlen(key_list));
      //format = swrite(format=(align?"%%%ds":"%%-%ds"), width + 1);
    }
    n = numberof(key_list);
    e = h_evaluator(obj);
    write, format="%s %s (hash_table, %s%d %s)\n",
      prefix1, name, (e ? "evaluator=\""+e+"\", " : ""),
      n, (n <= 1 ? "entry" : "entries");
    for (k = 1; k <= n; ++k) {
      key = key_list(k);
      __h_show_worker, h_get(obj,key), key, prefix2, 1 + (k == n);
    }
  } else if (is_array(obj)) {
    descr = typeof(obj);
    dims = dimsof(obj);
    n = numberof(dims);
    k = 1;
    while (++k <= n) {
      descr += swrite(format=",%d", dims(k));
    }
    if (numberof(obj) <= __h_show_maxcnt) {
      write, format="%s %s (%s) %s\n", prefix1, name, descr, sum(print(obj));
    } else {
      write, format="%s %s (%s)\n", prefix1, name, descr;
    }
  } else if (is_void(obj)) {
    write, format="%s %s (void) []\n", prefix1, name;
  } else if (is_symlink(obj)) {
    write, format="%s %s (%s) \"%s\"\n", prefix1, name, typeof(obj),
      name_of_symlink(obj);
  } else {
    write, format="%s %s (%s)\n", prefix1, name, typeof(obj);
  }
}

local __h_show_pfx_a, __h_show_pfx_b, __h_show_pfx_c, __h_show_pfx_d;
func h_show_style(a, b, c, d)
/* DOCUMENT h_show_style;
         or h_show_style, "ascii"/"utf8";
         or h_show_style, a, b, c, d;

      Set the prefixes used by `h_show`.  Without any arguments, restore the
      default (new "utf8" style).  Called with a single argument, apply the
      corresponding style ("ascii" or "utf8"). Can be called with 4 arguments
      to set the 4 prefixes according to your taste.

   SEE ALSO: h_show.
 */
{
  extern __h_show_pfx_a, __h_show_pfx_b, __h_show_pfx_c, __h_show_pfx_d;
  if (is_void(b) && is_void(c) && is_void(d)) {
    if (a == "ascii") {
      a = " |-";
      b = " | ";
      c = " `-";
      d = "   ";
    } else if (is_void(a) || a == "utf8") {
      a = " ├─";
      b = " │ ";
      c = " └─";
      d = "   ";
    } else {
      error, "unknown style";
    }
  }
  if (! is_scalar(a) || ! is_string(a) ||
      ! is_scalar(b) || ! is_string(b) ||
      ! is_scalar(c) || ! is_string(c) ||
      ! is_scalar(d) || ! is_string(d)) {
    error, "all prefixes must be scalar strings";
  }
  __h_show_pfx_a = a;
  __h_show_pfx_b = b;
  __h_show_pfx_c = c;
  __h_show_pfx_d = d;
}
if (is_void(__h_show_pfx_a)) h_show_style;

extern h_pop;
/* DOCUMENT h_pop(tab, "key");
         or h_pop(tab, key=);
     Pop member KEY out of hash table TAB and return it.  When called as a
     subroutine, the net result is therefore to delete the member from the
     hash table.

   SEE ALSO h_new, h_delete. */

func h_delete(h, ..)
/* DOCUMENT h_delete(tab, "key", ...);
     Delete members KEY, ... from hash table TAB and return it.  Any KEY
     arguments may be present and must be array of strings or nil.

   SEE ALSO h_new, h_pop. */
{
  local key;
  while (more_args()) {
    eq_nocopy, key, next_arg();
    n = numberof(key);
    for (i=1 ; i<=n ; ++i) h_pop, h, key(i);
  }
  return h;
}

extern h_stat;
/* DOCUMENT h_stat(tab);
     Returns an histogram of the slot occupation in hash table TAB.  The
     result is a long integer vector with i-th value equal to the number of
     slots with (i-1) items.  Note: efficient hash table should keep the
     number of items per slot as low as possible.

   SEE ALSO h_new. */

func h_list(tab, sorted)
/* DOCUMENT h_list(tab);
         or h_list(tab, sorted);
     Convert hash table TAB into a list: _lst("KEY1", VALUE1, ...).  The order
     of key-value pairs is arbitrary unless argument SORTED is true in which
     case keys get sorted in alphabetical order.

   SEE ALSO h_new, _lst, sort. */
{
  keylist = h_keys(tab);
  n = numberof(keylist);
  if (sorted && n>1) keylist = keylist(sort(keylist)(::-1));
  list = _lst();
  for (i=1 ; i<=n ; ++i) {
    /* grow the list the fast way, adding new values to its head (adding to
       the tail would make growth an N^2 proposition, as would using the grow
       function) */
    key = keylist(i);
    list = _cat(key, h_get(tab, key), list);
  }
  return list;
}

func h_cleanup(tab, recursively)
/* DOCUMENT h_cleanup, tab, 0/1;
         or h_cleanup(tab, 0/1);
     Delete all void members of hash table TAB and return TAB.  If the second
     argument is a true (non nil and non-zero) empty members get deleted
     recursively.

   SEE ALSO h_new. */
{
  local member;
  keylist = h_keys(tab);
  n = numberof(keylist);
  for (i=1 ; i<=n ; ++i) {
    key = keylist(i);
    eq_nocopy, member, h_get(tab, key);
    if (is_void(member)) h_pop, tab, key;
    else if (recursively && is_hash(member)) h_cleanup, member, recursively;
  }
  return tab;
}

func h_grow(tab, .., flatten=)
/* DOCUMENT h_grow, tab, key, value, ...;
     Grow member named KEY of hash table TAB by VALUE.  There may be any
     number of key-value pairs.  If keyword FLATTEN is true, then VALUE(*)
     instead of VALUE is appended to the former contents of TAB.KEY.  If
     member KEY does not already exists in TAB, then a new member is created
     with VALUE, or VALUE(*), as contents.

   SEE ALSO h_new. */
{
  local key, value;
  if (flatten) {
    while (more_args()) {
      eq_nocopy, key, next_arg();
      h_set, tab, key, grow(h_get(tab, key), next_arg()(*));
    }
  } else {
    while (more_args()) {
      eq_nocopy, key, next_arg();
      h_set, tab, key, grow(h_get(tab, key), next_arg());
    }
  }
}

func h_save_symbols(____l____, ..)
/* DOCUMENT h_save_symbols(namelist, ...);
         or h_save_symbols(flag);
     Return hash table which references symbols given in NAMELIST or selected
     by FLAG (see symbol_names).  Of course, the symbol names will be used as
     member names in the result.

   SEE ALSO h_new, h_restore_builtin, symbol_names. */
{
  /* Attempt to use dummy symbol names in this routine to avoid clash with
     the symbols ddefined in caller's context. */
  while (more_args()) grow, ____l____, next_arg();
  if ((____s____ = structof(____l____)) != string) {
    if ((____s____!=long && ____s____!=int && ____s____!=short &&
         ____s____!=char) || dimsof(____l____)(1))
      error, "expected a list of names, or nil, or a scalar integer";
    ____l____ = symbol_names(____l____);
  }
  ____s____ = h_new();
  ____n____ = numberof(____l____);
  for (____i____=1 ; ____i____<=____n____ ; ++____i____) {
    ____k____ = ____l____(____i____);
    h_set, ____s____, ____k____, symbol_def(____k____);
  }
  return ____s____;
}

local SAVE_BUILTINS;
local __h_saved_builtins;
func h_restore_builtin(name) { return h_get(__h_saved_builtins, name); }
/* DOCUMENT h_restore_builtin(name);
     Get the original definition of builtin function NAME.  This is useful if
     you deleted by accident a builtin function and want to recover it; for
     instance:
        sin = 1;
        ...
        sin = h_restore_builtin("sin");
     would restore the definition of the sine function that was redefined by
     the assignation.

     To enable this feature, you must define the global variable SAVE_BUILTINS
     to be true before loading the Yeti package.  For instance:
        SAVE_BUILTINS = 1;
        include, "yeti.i";
     then all all current definitions of builtin functions will be referenced
     in global hash table __h_saved_builtins and could be retrieved by calling
     h_restore_builtin.

     Note that this feature is disabled in batch mode.

   SEE ALSO h_new, h_save_symbols, batch. */
if (! batch() && SAVE_BUILTINS && ! is_hash(__h_saved_builtins)) {
  __h_saved_builtins = h_save_symbols(32);
}

/*---------------------------------------------------------------------------*/
/* MORPHO-MATH OPERATORS */

extern morph_dilation;
extern morph_erosion;
/* DOCUMENT morph_dilation(a, r);
         or morph_erosion(a, r);

     These functions perform a dilation/erosion morpho-math operation onto
     input array A which must have at most 3 dimensions.  A dilation (erosion)
     operation replaces every voxel of A by the maximum (minimum) value found
     in the voxel neighborhood as defined by the structuring element. Argument
     R defines the structuring element as follows:

      - If R is a scalar integer, then it is taken as the radius (in voxels)
        of the structuring element.

      - Otherwise, R gives the offsets of the structuring element relative to
        the coordinates of the voxel of interest.  In that case, R must an
        array of integers with last dimension equals to the number of
        dimensions of A.  In other words, if A is a 3-D array, then the
        offsets are:

           DX = R(1,..)
           DY = R(2,..)
           DZ = R(3,..)

        and the neighborhood of a voxel at (X,Y,Z) is defined as: (X + DX(I),
        Y + DY(I), Z + DZ(i)) for i=1,...,numberof(DX).  Conversely, R = [DX,
        DY, DZ].  Thanks to that definition, structuring element with
        arbitrary shape and relative position can be used in morpho-math
        operations.

        For instance, the dilation of an image (a 2-D array) IMG by a 3-by-5
        rectangular structuring element centered at the pixel of interest is
        obtained by:

           dx = indgen(-1:1);
           dy = indgen(-2:2);
           result =  morph_dilation(img, [dx, dy(-,)])


   SEE ALSO: morph_closing, morph_opening, morph_white_top_hat,
             morph_black_top_hat, morph_enhance.
 */

func morph_closing(a, r)
{ return morph_erosion(morph_dilation(a, r), r); }
func morph_opening(a, r)
{ return morph_dilation(morph_erosion(a, r), r); }
/* DOCUMENT morph_closing(a, r);
         or morph_opening(a, r);
     Perform an image closing/opening of A by a structuring element R.  A
     closing is a dilation followed by an erosion, whereas an opening is an
     erosion followed by a dilation.  See morph_dilation for the meaning of
     the arguments.

   SEE ALSO: morph_dilation, morph_white_top_hat,
             morph_black_top_hat. */

func morph_white_top_hat(a, r, s) {
  if (! is_void(s)) a = morph_closing(a, s);
  return a - morph_opening(a, r); }
func morph_black_top_hat(a, r, s) {
  if (! is_void(s)) a = morph_opening(a, s);
  return morph_closing(a, r) - a; }
/* DOCUMENT morph_white_top_hat(a, r);
         or morph_white_top_hat(a, r, s);
         or morph_black_top_hat(a, r);
         or morph_black_top_hat(a, r, s);
     Perform a summit/valley detection by applying a top-hat filter to
     array A.  Argument R defines the structuring element for the feature
     detection.  Optional argument gives the structuring element used to
     apply a smoothing to A prior to the top-hat filter.  If R and S are
     specified as the radii of the structuring elements, then S should be
     smaller than R.  For instance:

       morph_white_top_hat(bitmap, 3, 1)

     may be used to detect text or lines in a bimap image.


   SEE ALSO: morph_dilation, morph_closing, morph_enhance. */

func morph_enhance(a, r, s)
/* DOCUMENT morph_enhance(a, r);
         or morph_enhance(a, r, s);

     Perform noise reduction with edge preserving on array A.  The result is
     obtained by rescaling the values in A in a non-linear way between the
     local minimum and the local maximum.  Argument R defines the structuring
     element for the local neighborhood.  Argument S is a shape factor for the
     rescaling function which is a sigmoid function.  If S is given, it must
     be a non-negative value, the larger is S, the steeper is the rescaling
     function.  The shape factor should be larger than 3 or 5 to have a
     noticeable effect.

     If S is omitted, a step-like rescaling function is chosen: the output
     elements are set to either the local minimum or the local maximum which
     one is the closest.  This corresponds to the limit of very large shape
     factors and implements the "toggle filter" proposed by Kramer & Bruckner
     [1].

     The morph_enhance() may be iterated to achieve deblurring of the input
     array A (hundreds of iterations may be required).


  REFERENCES
     [1] H.P. Kramer & J.B. Bruckner, "iterations of a nonlinear
         transformation for enhancement of digital images", Pattern
         Recognition, vol. 7, pp. 53-58, 1975.


  SEE ALSO: morph_erosion, morph_dilation.
 */
{
  if (is_void(s)) {
    s = -1.0; /* special value */
  } else if (s < 0.0) {
    error, "S must be non-negative";
  } else {
    /* Pre-compute the range of the sigmoid function to detect early return
       with no change and skip the time consuming morpho-math operations. */
    s = double(s);
    hi = 1.0/(1.0 + exp(-s));
    lo = ((hi == 1.0) ? 0.0 : 1.0/(1.0 + exp(s)));
    if (hi == lo) {
      return a;
    }
  }

  /* Compute the local minima and maxima. */
  amin = morph_erosion(a, r);
  amax = morph_dilation(a, r);

  /* Staircase remapping of values. */
  if (s < 0.0) {
    test = ((a - amin) >= (amax - a));
    return merge(amax(where(test)), amin(where(! test)), test);
  }

  /* Remapping of values with a sigmoid. */
  test = ((amin < a)&(a < amax)); // values that need to change
  w = where(test);
  if (! is_array(w)) {
    return a;
  }
  type = structof(a);
  integer = is_integer(a);
  if (numberof(w) != numberof(a)) {
    /* Not all values change, select only those for which there is a
       difference between the local minimum and the local maximum. */
    unchanged = a(where(! test));
    a = a(w);
    amin = amin(w);
    amax = amax(w);
  }

  /* We use the sigmoid function f(t) = 1/(1 + exp(-t)) for the rescaling
     function g(t) = alpha*f(s*t) + beta with S the shape parameter, and
     (ALPHA,BETA) chosen to map the range [-1,1] into [0,1]. */
  alpha = 1.0/(hi - lo);
  beta = alpha*lo;

  /* Linearly map the values in the range [-1,1] -- we already know that the
     local minimum and maximum are different. */
  a = (double(a - amin) - double(amax - a))/double(amax - amin);
  a = alpha/(1.0 + exp(-s*a)) - beta;
  a = a*amax + (1.0 - a)*amin;
  if (! is_void(unchanged)) {
    a = merge(a, unchanged, test);
  }
  if (type != structof(a)) {
    if (integer) return type(round(a));
    return type(a);
  }
  return a;
}


/*---------------------------------------------------------------------------*/
/* COST FUNCTIONS AND REGULARIZATION */

extern cost_l2;
extern cost_l2l1;
extern cost_l2l0;
/* DOCUMENT cost_l2(hyper, res [, grd])
         or cost_l2l1(hyper, res [, grd])
         or cost_l2l0(hyper, res [, grd])

     These functions compute the cost for an array of residuals RES and
     hyper-parameters HYPER (which can have 1, 2 or 3 elements).  If optional
     third argument GRD is provided, it must be a simple variable reference
     used to store the gradient of the cost function with respect to the
     residuals.

     The cost_l2() function returns the sum of squared residuals times
     HYPER(1):

        COST_L2 = MU*sum(RES^2)

     where MU = HYPER(1).

     The cost_l2l1() and cost_l2l0() functions are quadratic (L2) for small
     residuals and non-quadratic (L1 and L0 respectively) for larger
     residuals.  The thresholds for L2 / non-L2 transition are given by the
     second and third value of HYPER.

     If HYPER = [MU, TINF, TSUP] with TINF < 0 and TSUP > 0, an asymmetric
     cost function is computed as:

        COST_L2L0 = MU*(TINF^2*sum(atan(RES(INEG)/TINF)^2) +
                        TSUP^2*sum(atan(RES(IPOS)/TPOS)^2))

        COST_L2L1 = 2*MU*(TINF^2*sum(RES(INEG)/TINF -
                                     log(1 + RES(INEG)/TINF)) +
                          TSUP^2*sum(RES(IPOS)/TSUP -
                                     log(1 + RES(IPOS)/TSUP)))

     with INEG = where(RES < 0) and IPOS = where(RES >= 0).  If any or the
     thresholds is negative or zero, the L2 norm is used for residuals with
     the corresponding sign (same as having an infinite threshold level).  The
     different cases are:

        TINF < 0    ==> L2-L1/L0 norm for negative residuals
        TINF = 0    ==> L2 norm for negative residuals
        TSUP = 0    ==> L2 norm for positive residuals
        TSUP > 0    ==> L2-L1/L0 norm for positive residuals

     For residuals much smaller (in magnitude) than the thresholds, the non-L2
     cost function behave as the L2 one.  For residuals much larger (in
     magnitude), than the thresholds, the L2-L1 cost function is L1
     (i.e. scales as abs(RES)) and the L2-L0 cost function is L0 (tends to
     saturate).

     If HYPER = [MU, T], with T>0, a symmetric non-L2 cost function is
     computed with TINF = -T and TSUP = +T; in other words:

        COST_L2L0 = MU*T^2*sum(atan(RES/T)^2)

        COST_L2L1 = 2*MU*T^2*sum(abs(RES/T) - log(1 + abs(RES/T)))

     If HYPER has only one element (MU) the L2 cost function is used.  Note
     that HYPER = [MU, 0] or HYPER = [MU, 0, 0] is the same as HYPER = MU
     (i.e. L2 cost function).  This is an implementation issue; by continuity,
     the cost should be zero for a threshold equals to zero.


   SEE ALSO:
     rgl_roughness_l2;
 */

extern rgl_roughness_l2;
extern rgl_roughness_l2_periodic;
extern rgl_roughness_l1;
extern rgl_roughness_l1_periodic;
extern rgl_roughness_l2l1;
extern rgl_roughness_l2l1_periodic;
extern rgl_roughness_l2l0;
extern rgl_roughness_l2l0_periodic;
extern rgl_roughness_cauchy;
extern rgl_roughness_cauchy_periodic;
/* DOCUMENT err = rgl_roughness_SUFFIX(hyper, offset, arr);
         or err = rgl_roughness_SUFFIX(hyper, offset, arr, grd);

     Compute regularization penalty based on the roughness of array ARR.
     SUFFIX indicates the type of cost function and the boundary condition
     (see below).  HYPER is the array of hyper-parameters; depending on the
     particular cost function, HYPER may have 1 or 2 elements (see below).
     OFFSET is an array of offsets for each dimensions of ARR (missing offsets
     are treated as being equal to zero): OFFSET(j) is the offset along j-th
     dimension between elements to compare. The penalty is equal to the sum of
     the costs of the differences between values of ARR separated by OFFSET;
     schematically:

        ERR = sum_k  COST(ARR(k + OFFSET) - ARR(k))

     The following penalties are implemented:

        rgl_roughness_l1		L1 norm
        rgl_roughness_l1_periodic	L1 norm, periodic
        rgl_roughness_l2		L2 norm
        rgl_roughness_l2_periodic	L2 norm, periodic
        rgl_roughness_l2l1		L2-L1 norm
        rgl_roughness_l2l1_periodic	L2-L1 norm, periodic
        rgl_roughness_l2l0		L2-L0 norm
        rgl_roughness_l2l0_periodic	L2-L0 norm, periodic
        rgl_roughness_cauchy		Cauchy norm
        rgl_roughness_cauchy_periodic	Cauchy norm, periodic

     The suffix "periodic" indicates periodic boundary condition.  The
     different cost functions are):

        L1(x)	  = mu * abs(x)
        L2(x)	  = mu * x^2
        L2L0(x)	  = mu * eps^2 * atan(x/eps))^2
        L2L1(x)	  = 2 * mu * eps^2 * (abs(x/eps) - log(1 + abs(x/eps)))
        Cauchy(x) = mu * eps^2 * log(1 + (x/eps)^2)

     where X = ARR(k + OFFSET) - ARR(k), MU = HYPER(1) is the weight of the
     regularization and EPS = HYPER(2) is a threshold level.  Restrictions:
     MU >= 0 and EPS >= 0 and the result is ERR = 0 when MU = 0 or EPS = 0
     -- the case EPS = 0, is implemented by continuity.

     The L2-L0, L2-L1 and Cauchy cost functions behave as L2(X) = MU*X^2 for
     abs(X) much smaller than EPS.  They differ in their tail for large
     values of abs(X): L2-L0 tends to be flat; L2-L1 behave as abs(X) and
     CAUCHY is intermediate.

     From a Baysian viewpoint, L2 correspond to the neg-log likelihood of a
     Gaussian distribution, CAUCHY correspond to the neg-log likelihood of a
     Cauchy (or Lorentzian) distribution.

     Optional argument GRD must be an unadorned variable where to store the
     gradient. If the argument GRD is omitted, no gradient is computed. On
     entry, the value of GRD may be empty to automatically or an array
     (convertible to real type) with same dimension list as ARR.  In the first
     case, a new array is created to store the gradient; in the second case,
     the contents of GRD is augmented by the gradient (and GRD is converted to
     "double" if it is not yet the case).


   EXAMPLES

     To compute isotropic quadratic roughness along 2 first dimensions of A:

         g = array(double, dimsof(a)); // to store the gradient
         mu = 1e3; // regularization weight
         rgl = rgl_roughness_l2; // shortcut
         f = (rgl(    mu,   1,	   a, g) +
              rgl(    mu, [ 0, 1], a, g) +
              rgl(0.5*mu, [-1, 1], a, g) +
              rgl(0.5*mu, [ 1, 1], a, g));

     To compute anisotropic roughness along first and third dimensions of A:

         g = array(double, dimsof(a)); // to store the gradient
         mu1 = 1e3; // regularization weight along first dimension
         mu2 = 3e4; // regularization weight along second dimension
         rgl = rgl_roughness_l2; // shortcut
         f = (rgl(mu1,	       1,	 a, g) +  // 1st dim
              rgl(mu3,	     [ 0, 0, 1], a, g) +  // 3rd dim
              rgl(mu1 + mu3, [-1, 0, 1], a, g) +  // 1st & 3rd dim
              rgl(mu1 + mu3, [ 1, 0, 1], a, g));  // 1st & 3rd dim


   SEE ALSO
     cost_l2.
 */

/*---------------------------------------------------------------------------*/
/* 1D CONVOLUTION AND "A TROUS" WAVELET TRANSFORM */

extern __yeti_convolve_f;
/* PROTOTYPE
     void yeti_convolve_f(float array dst, float array src, int stride,
                          int n, int nafter, float array ker, int w,
                          int scale, int border, float array ws); */

extern __yeti_convolve_d;
/* PROTOTYPE
     void yeti_convolve_d(double array dst, double array src, int stride,
                          int n, int nafter, double array ker, int w,
                          int scale, int border, double array ws); */

func yeti_convolve(a, which=, kernel=, scale=, border=, count=)
/* DOCUMENT ap = yeti_convolve(a)
     Convolve  array A along  its dimensions  (all by  default) by  a given
     kernel.  By default, the convolution kernel is [1,4,6,4,1]/16.0.  This
     can be  changed by using keyword  KERNEL (but the kernel  must have an
     odd number  of elements).  The following operation  is performed (with
     special  handling for the  boundaries, see  keyword BORDER)  along the
     direction(s) of interest:
     |         ____
     |         \
     |   AP(i)= \ KERNEL(j+W) * A(i + j*SCALE)
     |          /
     |         /___
     |     -W <= j <= +W
     |
     where  numberof(KERNEL)=2*W+1.  Except  for  the SCALE  factor, AP  is
     mostly  a convolution  of A  by array  KERNEL along  the  direction of
     interest.

     Keyword WHICH can be used  to specify the dimension(s) of interest; by
     default, all  dimensions get convolved.   As for indices,  elements in
     WHICH less than  1 is taken as relative to the  final dimension of the
     array.  You may specify  repeated convolution along some dimensions by
     using them several times in array WHICH (see keyword COUNT).

     Keyword BORDER can be used to set the handling of boundary conditions:
       BORDER=0  Extrapolate  missing  values  by  the left/rightmost  ones
                 (this is the default behaviour).
       BORDER=1  Extrapolate missing left  values by zero and missing right
                 values by the rightmost one.
       BORDER=2  Extrapolate  missing left  values by the  leftmost one and
                 missing right values by zero.
       BORDER=3  Extrapolate missing left/right values by zero.
       BORDER=4  Use periodic conditions.
       BORDER>4 or BORDER<0
                 Do   not   extrapolate   missing  values   but   normalize
                 convolution product  by sum  of kernel weights  taken into
                 account (assuming they are all positive).

     By  default, SCALE=1 which  corresponds to  a simple  convolution.  An
     other value can be used thanks  to keyword SCALE (e.g. for the wavelet
     "a trou" method).  The value of SCALE must be a positive integer.

     Keyword COUNT  can be used to  augment the amount  of smoothing: COUNT
     (default COUNT=1) is  the number of convolution passes.   It is better
     (i.e. faster) to use only one pass with appropriate convolution kernel
     (see keyword KERNEL).

  SEE ALSO yeti_wavelet.

  RESTRICTIONS
    1. Should use the in-place ability of the operation to limit the number
       of array copies.
    2. Complex convolution not yet  implemented (although it exists in the
       C-code). */
{
  /* Check data type of A. */
  type = structof(a);
  if (type == complex) {
    return (yeti_convolve(double(a), which=which, kernel=kernel, scale=scale,
                          border=border, count=count)
            + 1i*yeti_convolve(a.im, which=which, kernel=kernel, scale=scale,
                               border=border, count=count));
  } else if (type == double) {
    op = __yeti_convolve_d;
  } else if (type == float || type == long || type == int || type == short ||
             type == char) {
    op = __yeti_convolve_f;
    type = float;
  } else {
    error, "bad data type";
  }
  a = type(a);  /* force a private copy of A */

  /* Check dimensions of A and keyword WHICH. */
  dims = dimsof(a);
  rank = dims(1);
  if (is_void(which)) {
    which = indgen(rank);
  } else {
    which += (which <= 0)*rank;
    if (min(which) < 1 || max(which) > rank)
      error, "dimension index out of range in WHICH";
  }

  /* Check KERNEL and other keywords. */
  if (is_void(kernel)) {
    k0= type(0.375);  /* 6.0/16.0 */
    k1= type(0.25);   /* 4.0/16.0 */
    k2= type(0.0625); /* 1.0/16.0 */
    kernel= [k2, k1, k0, k1, k2];
  }
  if ((w = numberof(kernel))%2 != 1)
    error, "KERNEL must have an odd number of elements";
  if (is_void(scale)) scale = 1;
  else if (structof(scale+0)!=long || scale<=0)
    error, "bad value for keyword SCALE";
  if (is_void(border)) border = 0;
  if (is_void(count)) count = 1;

  /* Compute strides. */
  stride = array(1, rank);
  for (s=1,i=2 ; i<=rank ; ++i) stride(i) = stride(i-1)*dims(i);
  stride = stride(which);
  dims = dims(which + 1);
  nafter = numberof(a)/(dims*stride);

  /* Apply the operator along every dimensions of interest. */
  for (i=1 ; i<=numberof(which) ; ++i) {
    len = dims(i);
    for (j=1 ; j<=count ; ++j) {
      op, a, a, stride(i), len, nafter(i), kernel, (w-1)/2, scale, border,
        array(type, 2*len);
    }
  }
  return a;
}

func yeti_wavelet(a, order, which=, kernel=, border=)
/* DOCUMENT cube = yeti_wavelet(a, order)
     Compute the "a trou" wavelet transform of A.  The result is such
     that:
       CUBE(.., i) = S_i - S_(i+1)
     where:
       S_1 = A
       S_(i+1) = yeti_convolve(S_i, SCALE=2^(i-1))
     As a consequence:
       CUBE(..,sum) = A;

  SEE ALSO yeti_convolve. */
{
  if (((s=structof(order)) != long && s!=int && s!=short && s!=char) ||
      dimsof(order)(1) || order<0) {
    error, "ORDER must be a non-negative integer";
  }
  dims = dimsof(a);
  grow, dims, order+1;
  ++dims(1);
  cube = array(structof(a(1)+0.0f), dims);
  for (scale=1, i=1 ; i<=order ; ++i, scale*=2) {
    ap = a;
    a = yeti_convolve(a, which=which, kernel=kernel, scale=scale,
                      border=border);
    cube(..,i) = ap-a;
  }
  cube(..,0) = a;
  return cube;
}

extern smooth3;
/* DOCUMENT smooth3(a)
     Returns array A smoothed by a simple 3-element convolution (but for
     the edges).  In one dimension, the smoothing operation reads:
        smooth3(A)(i) = C*A(i) + D*(A(i-1) + A(i+1))
     but for the first and last element for which:
        smooth3(A)(1) = E*A(1) + D*A(2)
        smooth3(A)(n) = E*A(n) + D*A(n-1)
     where N is the length of the dimension and the coefficients are:
        C = 0.5
        D = 0.25
        E = 0.75
     With the default value of C (see keyword C below), the smoothing
     operation is identical to:

        smooth3(A) = A(pcen)(zcen)             for a 1D array
        smooth3(A) = A(pcen,pcen)(zcen,zcen)   for a 2D array
        ...                                    and so on

     Keyword C can be used to specify another value for the coefficient
     C (default: C=0.5); coefficients D and E are computed as follows:
        D = 0.5*(1 - C)
        E = 0.5*(1 + C)

     The default is to smooth A along all its dimensions, but keyword WHICH
     can be used to specify the only dimension to smooth.  If WHICH is less
     or equal zero, then the smoothed dimension is the last one + WHICH.

     The smoothing operator implemented by smooth3 has the following
     properties:

     1. The smoothing operator is linear and symmetric (for any number of
        dimensions in A).  The symmetry of the smoothing operator is
        important for the computation of gradients in regularization.  For
        instance, let Y = smooth3(X) and Q be a scalar function of Y, then
        then the gradient of Q with respect to X is simply:
           DQ_DX = smooth3(DQ_DY)
        where DQ_DY is the gradient of Q with respect to Y.

     2. For a vector, A, smooth3(A)=S(,+)*A(+) where the matrix S is
        tridiagonal:

           [E D         ]
           [D C D       ]
           [  D C D     ]
           [   \ \ \    ]    where, to improve readability,
           [    \ \ \   ]    missing values are all zero.
           [     D C D  ]
           [       D C D]
           [         D E]

        You can, in principle, reverse the smoothing operation with TDsolve
        along each dimensions of smooth3(A).  Note: for a vector A, the
        operator S-I applied to A (where I is the identity matrix) is the
        finite difference 2nd derivatives of A (but for the edges).

     3. The definition of coefficients C, D and E insure that the smoothing
        operator does not change the sum of the element values of its
        argument, i.e.: sum(smooth3(A)) = sum(A).

     4. Only an array with all elements having the same value is invariant
        by the smoothing operator.  In fact "slopes" along dimensions of A
        are almost invariant, only the values along the edges are changed.


   KEYWORDS: c, which.

   SEE ALSO: TDsolve. */

/*---------------------------------------------------------------------------*/
/* MATH ROUTINES */

extern sinc;
/* DOCUMENT sinc(x);
     Returns the "sampling function" of X as defined by Woodward (1953) and
     Bracewell (1999):

       sinc(x) = 1                 for x=0
                 sin(PI*x)/(PI*x)  otherwise

     Note: This definition correspond to the "normalized sinc function";
     some other authors may define the sampling function without the PI
     factors in the above expression.


   REFERENCES
     Bracewell, R. "The Filtering  or Interpolating Function, sinc(x)."  In
     "The  Fourier Transform  and  Its Applications",  3rd  ed.  New  York:
     McGraw-Hill, pp. 62-64, 1999.

     Woodward, P.  M. "Probability and Information Theory with Applications
     to Radar". New York: McGraw-Hill, 1953.

  SEE ALSO: sin. */

extern arc;
/* DOCUMENT arc(x);
     Returns angle X wrapped in range (-PI, +PI]. */

/*---------------------------------------------------------------------------*/
/* SPARSE MATRICES AND MATRIX-VECTOR MULTIPLICATION */

extern sparse_matrix;
/* DOCUMENT s = sparse_matrix(coefs, row_dimlist, row_indices,
                                     col_dimlist, col_indices);

     Returns a sparse matrix object.  COEFS is an array with the non-zero
     coefficients of the full matrix.  ROW_DIMLIST and COL_DIMLIST are the
     dimension lists of the matrix 'rows' and 'columns'.  ROW_INDICES and
     COL_INDICES are the 'row' and 'column' indices of the non-zero
     coefficients of the full matrix.

     The sparse matrix object S can be used to perform sparse matrix
     multiplication as follows:

       S(x) or S(x, 0) yields the result of matrix multiplication of 'vector'
               X by S; X must be an array with dimension list COL_DIMLIST (or
               a vector with as many elements as an array with such a
               dimension list); the result is an array with dimension list
               ROW_DIMLIST.

       S(y, 1) yields the result of matrix multiplication of 'vector' Y by the
               transpose of S; Y must be an array with dimension list
               ROW_DIMLIST (or a vector with as many elements as an array with
               such a dimension list); the result is an array with dimension
               list COL_DIMLIST.

      The contents of the sparse matrix object S can be queried as with a
      regular Yorick structure: S.coefs, S.row_dimlist, S.row_indices,
      S.col_dimlist or S.col_indices are valid expressions if S is a sparse
      matrix.


    SEE ALSO: is_sparse_matrix, mvmult,
              sparse_expand, sparse_squeeze, sparse_grow.
 */

extern is_sparse_matrix;
/* DOCUMENT is_sparse_matrix(obj)
 *   Returns true if OBJ is a sparse matrix object; false otherwise.
 *
 *  SEE ALSO: sparse_matrix.
 */


func sparse_grow(s, coefs, row_indices, col_indices)
/* DOCUMENT sparse_grow(s, coefs, row_indices, col_indices);
     Returns a sparse matrix object obtained by growing the non-zero
     coefficients of S by COEFS with the corresponding row/column indices
     given by ROW_INDICES and COL_INDICES which must have the same number of
     elements as COEFS.

    SEE ALSO: sparse_matrix.
 */
{
  return sparse_matrix(grow(s.coefs, coefs),
                       s.row_dimlist, grow(s.row_indices, row_indices),
                       s.col_dimlist, grow(s.col_indices, col_indices));
}

func sparse_squeeze(a, n)
/* DOCUMENT s = sparse_squeeze(a);
         or s = sparse_squeeze(a, n);
     Convert array A into its sparse matrix representation.  Optional argument
     N (default, N=1) is the number of dimensions of the input space.  The
     dimension list of the input space are the N trailing dimensions of A and,
     assuming that A has NDIMS dimensions, the dimension list of the output
     space are the NDIMS - N leading dimensions of A.

   SEE ALSO: sparse_matrix, sparse_expand.
 */
{
  if (! is_array(a)) error, "unexpected non-array";
  dimlist = dimsof(a);
  ndims = dimlist(1);
  if (is_void(n)) n = 1; /* one trailing dimension for the input space */
  if ((m = ndims - n) < 0) error, "input space has too many dimensions";
  if (! is_array((i = where(a)))) error, "input array is zero everywhere!";
  (row_dimlist = array(long, m + 1))(1) = m;
  stride = 1;
  if (m >= 1) {
    row_dimlist(2:) = dimlist(2:m+1);
    for (j=m+1;j>=2;--j) stride *= dimlist(j);
  }
  (col_dimlist = array(long, n + 1))(1) = n;
  if (n >= 1) col_dimlist(2:) = dimlist(m+2:0);
  j = i - 1;
  return sparse_matrix(a(i),
                       row_dimlist, 1 + j%stride,
                       col_dimlist, 1 + j/stride);
}

func sparse_expand(s)
/* DOCUMENT a = sparse_expand(s);
     Convert sparse matrix S into standard Yorick's array A.

   SEE ALSO: sparse_squeeze, histogram.
 */
{
  row_dimlist = s.row_dimlist;
  stride = 1;
  j = row_dimlist(1) + 2;
  while (--j >= 2) stride *= row_dimlist(j);
  a = array(structof(s.coefs), row_dimlist, s.col_dimlist);
#if 0
  /* We cannot do that because, coefficients may not be unique. */
  a(s.row_indices + (s.col_indices - 1)*stride) = s.coefs;
#endif
  a(*) = histogram(s.row_indices + (s.col_indices - 1)*stride,
                   s.coefs, top=numberof(a));
  return a;
}

local sparse_restore, sparse_save;
/* DOCUMENT sparse_save, pdb, obj;
         or sparse_restore(pdb);
     The subroutine sparse_save saves the sparse matrix OBJ into file PDB.
     The function sparse_restore restores the sparse matrix saved into file
     PDB.  PDB is either a file name or a PDB file handle.

   SEE ALSO: createb, openb, restore, save, sparse_matrix.
 */

func sparse_save(pdb, obj)
{
  if (! is_sparse_matrix(obj)) error, "expecting a sparse matrix";
  if (structof(pdb) == string) {
    logfile = pdb + "L";
    if (open(logfile, "r", 1)) logfile = 0;
    pdb = createb(pdb);
    if (logfile) remove, logfile;
  }
  local coefs, row_dimlist, row_indices, col_dimlist, col_indices;
  eq_nocopy, coefs, obj.coefs;
  eq_nocopy, row_dimlist, obj.row_dimlist;
  eq_nocopy, row_indices, obj.row_indices;
  eq_nocopy, col_dimlist, obj.col_dimlist;
  eq_nocopy, col_indices, obj.col_indices;
  save, pdb, coefs, row_dimlist, row_indices, col_dimlist, col_indices;
}

func sparse_restore(pdb)
{
  local coefs, row_dimlist, row_indices, col_dimlist, col_indices;
  if (structof(pdb) == string) pdb = openb(pdb);
  restore, pdb, coefs, row_dimlist, row_indices, col_dimlist, col_indices;
  return sparse_matrix(coefs, row_dimlist, row_indices,
                              col_dimlist, col_indices);
}

extern mvmult;
/* DOCUMENT y = mvmult(a, x);
         or y = mvmult(a, x, 0/1);

     Returns the result of (generalized) matrix-vector multiplication of
     vector X (a regular Yorick array) by matrix A (a regular Yorick array or
     a sparse matrix).  The matrix-vector multiplication is performed as if
     there is only one index running over the elements of X and the
     trailing/leading dimensions of A.

     If optional last argument is omitted or false, the summation index runs
     across the trailing dimensions of A which must be the same as those of
     X and the dimensions of the result are the remaining leading dimensions
     of A.

     If optional last argument is 1, the matrix operator is transposed: the
     summation index runs across the leading dimensions of A which must be the
     same as those of X and the dimensions of the result are the remaining
     trailing dimensions of A.

   SEE ALSO: sparse_matrix, sparse_squeeze.
 */


/*---------------------------------------------------------------------------*/
/* ACCESSING YORICK'S INTERNALS */

extern is_hash;
/* DOCUMENT is_hash(object)
     Returns 1, if OBJECT is a regular hash table; returns 2, if OBJECT is a
     hash table with a specialized evaluator; returns 0, if OBJECT is not a
     hash table.

   SEE ALSO: h_new, h_evaluator,
             is_array, is_func, is_integer, is_list, is_range, is_scalar,
             is_stream, is_struct, is_void.
 */

extern nrefsof;
/* DOCUMENT nrefsof(object)
     Returns number of references on OBJECT.

   SEE ALSO: unref. */

extern get_encoding;
/* DOCUMENT get_encoding(name);
     Return the data layout for machine NAME, one of:
       "native"   the current machine
          (little-endians)
       "i86"      Intel x86 Linux
       "ibmpc"    IBM PC (2 byte int)
       "alpha"    Compaq alpha
       "dec"      DEC workstation (MIPS), Intel x86 Windows
       "vax"      DEC VAX (H-double)
       "vaxg"     DEC VAX (G-double)
          (big-endians)
       "xdr"      External Data Representation
       "sun"      Sun, HP, SGI, IBM-RS6000, MIPS 32 bit
       "sun3"     Sun-2 or Sun-3 (old)
       "sgi64"    SGI, Sun, HP, IBM-RS6000 64 bit
       "mac"      MacIntosh 68000 (power Mac, Gx are __sun)
       "macl"     MacIntosh 68000 (12 byte double)
       "cray"     Cray XMP, YMP

     The result is a vector of 32 long's as follow:
       [size, align, order] repeated 6  times for  char,  short, int, long,
                            float,  and double, except  that char  align is
                            always  1,   so  result(2)  is   the  structure
                            alignment (see struct_align).
       [sign_address,  exponent_address, exponent_bits,
        mantissa_address, mantissa_bits,
        mantissa_normalization, exponent_bias]  repeated  twice  for  float
                            and double.  See the comment at the top of file
                            prmtyp.i for an explanation of these fields.

     The total number of items is therefore 3*6 + 7*2 = 32.

   SEE ALSO get_primitives, set_primitives, install_encoding, machine_constant. */

func install_encoding(file, encoding)
/* DOCUMENT install_encoding, file, encoding;
     Set layout of  primitive data types for binary  stream FILE.  ENCODING
     may be  one of the  names accepted by  get_encoding or an array  of 32
     integers as explained in get_encoding documentation.

    SEE ALSO: get_encoding, install_struct. */
{
  /* Get encoding parameters with minimal check. */
  if (structof(encoding) == string) {
    p = get_encoding(encoding);
  } else {
    if ((s = structof(encoding)) == long) p = encoding;
    else if (/*s==char || s==short || */s==int) p = long(encoding);
    else error, "bad data type for ENCODING";
    if (numberof(p) != 32) error, "bad number of elements for encoding";
  }

  /* Install primitive definitions. */
  install_struct, file, "char",    1,     1,     p( 3);
  install_struct, file, "short",   p( 4), p( 5), p( 6);
  install_struct, file, "int",     p( 7), p( 8), p( 9);
  install_struct, file, "long",    p(10), p(11), p(12);
  install_struct, file, "float",   p(13), p(14), p(15), p(19:25);
  install_struct, file, "double",  p(16), p(17), p(18), p(26:32);
  struct_align, file, p(2);
}

func same_encoding(a, b)
/* DOCUMENT same_encoding(a, b)
     Compare primitives  A and B  which must be conformable  integer arrays
     with first dimension equals to 32 (see set_primitives).  The result is
     an array  of int's with one  less dimension than A-B  (the first one).
     Some checking is  performed for the operands.  The  byte order for the
     char data type is ignored in the comparison.

   SEE ALSO install_encoding, get_encoding.*/
{
  if (! is_array((d = dimsof(a, b))) || d(1) < 1 || d(2) != 32)
    error, "bad dimensions";
  diff = abs(a - b);
  if ((s = structof(diff)) != long && s != int) error, "bad data type";
  if (anyof(a(1,..) != 1) || anyof(b(1,..) != 1))
    error, "unexpected sizeof(char) != 1";
  diff(3, ..) = 0; /* ignore byte order for type char */
  return ! diff(max,);
}

local DBL_EPSILON, DBL_MIN, DBL_MAX;
local FLT_EPSILON, FLT_MIN, FLT_MAX;
extern machine_constant;
/* DOCUMENT machine_constant(str)
     Returns the value of the machine dependent constant given its name
     STR.  STR is a scalar string which can be one of (prefixes "FLT_" and
     "DBL_" are for single/double precision respectively):

     "FLT_MIN",
     "DBL_MIN" - minimum normalized positive floating-point number;

     "FLT_MAX",
     "DBL_MAX" - maximum representable finite floating-point number;

     "FLT_EPSILON",
     "DBL_EPSILON" - the difference between 1 and the least value greater
             than 1 that is representable in the given floating point
             type: B^(1 - P);

     "FLT_MIN_EXP",
     "DBL_MIN_EXP" - minimum integer EMIN such that FLT_RADIX^(EMIN - 1)
             is a normalized floating-point value;

     "FLT_MIN_10_EXP"
     "DBL_MIN_10_EXP" - minimum negative integer such that 10 raised to
             that power is in the range of normalized floating-point
             numbers: ceil(log10(B)*(EMIN - 1));

     "FLT_MAX_EXP",
     "DBL_MAX_EXP" - maximum integer EMAX such that FLT_RADIX^(EMAX - 1)
             is a normalized floating-point value;

     "FLT_MAX_10_EXP"
     "DBL_MAX_10_EXP" - maximum integer such that 10 raised to that power
             is in the range of normalized floating-point numbers:
             floor(log10((1 - B^(-P))*(B^EMAX)))

     "FLT_RADIX" - radix of exponent representation, B;

     "FLT_MANT_DIG",
     "DBL_MANT_DIG" - number of base-FLT_RADIX significant digits P in the
             mantissa;

     "FLT_DIG",
     "DBL_DIG" - number of decimal digits, Q, such that any floating-point
             number with Q decimal digits can be rounded into a
             floating-point number with P (FLT/DBL_MANT_DIG) radix B
             (FLT_RADIX) digits and back again without change to the Q
             decimal digits:
                 Q = P*log10(B)                if B is a power of 10
                 Q = floor((P - 1)*log10(B))   otherwise


    SEE ALSO: get_encoding.
 */
DBL_EPSILON = machine_constant("DBL_EPSILON");
DBL_MIN = machine_constant("DBL_MIN");
DBL_MAX = machine_constant("DBL_MAX");
FLT_EPSILON = machine_constant("FLT_EPSILON");
FLT_MIN = machine_constant("FLT_MIN");
FLT_MAX = machine_constant("FLT_MAX");

local typemin, typemax;
/* DOCUMENT typemin(T);
         or typemax(T);

     These functions yield the maximum/minimum value of integer or
     floating-point type T.

   SEE ALSO: sizeof.
 */
func typemin(T)
{
    minus_one = T(-1);
    if (is_integer(minus_one)) {
        return (minus_one < 0 ? (-(T(1) << (sizeof(T)*8 - 2)))*T(2) : T(0));
    } else if (T == double) {
        return -machine_constant("DBL_MAX");
    } else if (T == float) {
        return -machine_constant("FLT_MAX");
    }
    error, "invalid type";
}

func typemax(T)
{
    minus_one = T(-1);
    if (is_integer(minus_one)) {
        if (minus_one > 0) return minus_one;
        one = T(1);
        return ((one << (sizeof(T)*8 - 2)) - one)*T(2) + one;
    } else if (T == double) {
        return machine_constant("DBL_MAX");
    } else if (T == float) {
        return machine_constant("FLT_MAX");
    }
    error, "invalid type";
}

func symbol_info(____n____)
/* DOCUMENT symbol_info, flags;
         or symbol_info, names;
         or symbol_info;
     Print out some information about  Yorick's symbols.  FLAGS is a scalar
     integer used to select symbol types (as in symbol_names).  NAMES is an
     array  of symbol  names.  If  argument  is omitted  or undefined,  all
     defined array symbols get selected (as with FLAGS=3).

   SEE ALSO: info, mem_info, symbol_def, symbol_names.*/
{
  /* attempt to use _very_ odd names to avoid clash with caller */
  if (is_void(____n____)) ____n____ = symbol_names(3);
  else if (! is_string(____n____)) ____n____ = symbol_names(____n____);
  for (____i____ = 1; ____i____ <= numberof(____n____); ++____i____) {
    write, format="%s:", ____n____(____i____);
    info, symbol_def(____n____(____i____));
  }
}

func mem_info(____a____)
/* DOCUMENT mem_info;
         or mem_info, count;
     Print out some information about memory occupation. If COUNT is
     specified, the COUNT biggest (in bytes) symbols are listed (use
     COUNT<0 to list all symbols sorted by size).  Only the memory used by
     Yorick's array symbols (including array of pointers), lists and hash
     tables is considered.

   BUGS:
     Symbols
     which are aliases (e.g. by using eq_nocopy) may be considered several
     times.

   SEE ALSO:
     symbol_def, symbol_info, symbol_names, mem_clear, fullsizeof.
 */
{
  ____n____ = symbol_names(3);
  ____i____ = numberof(____n____);
  ____s____ = array(long, ____i____);
  while (____i____ > 0) {
    ____s____(____i____) = fullsizeof(symbol_def(____n____(____i____)));
    --____i____;
  }
  ____i____ = sum(____s____);
  write, format="Total memory used by array symbols: %d bytes (%.3f Mb)\n",
    ____i____, ____i____/1024.0^2;
  if (____a____) {
    ____i____ = sort(____s____);
    if (____a____ > 0 && ____a____ < numberof(____i____))
      ____i____ = ____i____(1-____a____:0);
    ____n____ = ____n____(____i____);
    ____s____ = ____s____(____i____);
    ____i____ = numberof(____i____);
    if (____i____ > 1) {
      write, format="The %d biggest symbols are:\n", ____i____;
    } else {
      write, format="%s", "The biggest symbol is:\n";
    }
    ____a____ = swrite(format="  %%%ds: %%%.0fd bytes,",
                       max(strlen(____n____)), ceil(log10(max(____s____))));
    while (____i____ > 0) {
      write, format=____a____, ____n____(____i____), ____s____(____i____);
      info, symbol_def(____n____(____i____));
      --____i____;
    }
  }
}

func mem_clear(_____s_____, _____f_____)
/* DOCUMENT mem_clear;
         or mem_clear, minsize;
         or mem_clear, minsize, flags;

                  *** USE THIS FUNCTION WITH CARE ***

     Clear (that is destroy) global symbols larger than MINSIZE bytes (default
     1024 bytes).  Symbol names starting with an underscore are not destroyed.
     Optional argument FLAGS (default 0) is a bitwise combination of the
     following bits:

        0x01 - quiet mode, do not even print the summary.
        0x02 - verbose mode, print out the names of matched symbols.
        0x04 - dry-run mode, the symbols are not really destroyed.

     When called as a function, returns the number of bytes released.


   SEE ALSO: symbol_def, symbol_info, symbol_names, fullsizeof.
 */
{
  _____t_____ = 0;
  if (is_void(_____s_____)) {
    _____s_____ = 1024;
  }
  _____n_____ = symbol_names(1 | 2 | 1024);
  _____i_____ = 1 + numberof(_____n_____);
  while (--_____i_____ > 0) {
    if (strpart(_____n_____(_____i_____), 1:1) != "_") {
      _____u_____ = fullsizeof(symbol_def(_____n_____(_____i_____)));
      if (_____u_____ > _____s_____) {
        if ((_____f_____ & 0x02) != 0) {
          write, _____n_____(_____i_____);
        }
        if ((_____f_____ & 0x04) == 0) {
          symbol_set, _____n_____(_____i_____), [];
        }
        _____t_____ += _____u_____;
      }
    }
  }
  if (am_subroutine()) {
    if ((_____f_____ & 0x01) == 0) {
      write, format="%d bytes of memory cleared\n", _____t_____;
    }
  } else {
    return t;
  }
}

func fullsizeof(obj)
/* DOCUMENT fullsizeof(obj)
     Returns size in bytes of object OBJ.  Similar to sizeof (which see)
     function but also works for lists, arrays of pointers, structures
     or hash tables.

   SEE ALSO: sizeof, is_list, is_array, is_hash.
 */
{
  size = sizeof(obj);
  id = identof(obj);
  if (id > Y_COMPLEX) {
    if (id <= Y_STRUCT) {
      if (id == Y_STRING) {
        size += numberof(obj) + sum(strlen(obj));
      } else if (id == Y_POINTER) {
        n = numberof(obj);
        for (i = 1; i <= n; ++i) {
          size += fullsizeof(*obj(i));
        }
      } else if (id == Y_STRUCT) {
        /* For each member name, get the size of the member.  TMP is a
           single element structure of the same type for fast checking the
           type of each member. */
        tmp = array(structof(obj));
        str = sum(print(structof(obj)));
        sub = [1,2];
        reg = "^ *[A-Z_a-z][0-9A-Z_a-z]* +([A-Z_a-z][0-9A-Z_a-z]*)[^;]*;(.*)";
        str = strpart(str, strgrep("^struct +([^ ]+) +{(.*)", str, sub=sub));
        for (;;) {
          str = str(2);
          str = strpart(str, strgrep(reg, str, sub=sub));
          name = str(1);
          if (! name) {
            break;
          }
          if (identof(get_member(tmp, name)) > Y_COMPLEX) {
            size += fullsizeof(get_member(obj, name));
          }
        }
      }
    } else if (is_hash(obj)) {
      for (key = h_first(obj); key; key = h_next(obj, key)) {
        size += fullsizeof(h_get(obj, key));
      }
    } else if (is_list(obj)) {
      while (obj) {
        size += fullsizeof(_car(obj));
        obj = _cdr(obj);
      }
    }
  }
  return size;
}

extern insure_temporary;
/* DOCUMENT insure_temporary, var1 [, var2, ...];
     Insure that symbols VAR1 (VAR2 ...) are temporary variables referring to
     arrays.  Useful prior to in-place operations to avoid side-effects for
     caller.

     The call:
         insure_temporary, var;
     has the same effect as:
         var = unref(var);

   SEE ALSO: eq_nocopy, nrefsof, swap, unref. */

extern mem_base;
extern mem_copy;
extern mem_peek;
/* DOCUMENT mem_base(array);
         or mem_copy, address, expression;
         or mem_peek(address, type, dimlist);
     Hacker routines to read/write data at a given memory location.  These
     routines allow the user to do _very_ nasty but sometimes needed things.
     They do not provide the safety level of ususal Yorick routines, and must
     therefore be used with extreme care (you've been warned).  In all these
     routines, ADDRESS is either a long integer scalar or a scalar pointer
     (e.g. &OBJECT).

     mem_base returns the address (as a long scalar) of the first element of
              array object ARRAY.  You can use this function if you need to
              add some offset to the address of an object, e.g. to reach some
              particular element of an array or a structure.

     mem_copy copy the contents of EXPRESSION at memory location ADDRESS.

     mem_peek returns a new array of data type TYPE and dimension list DIMLIST
              filled with memory contents starting at address ADDRESS.

   EXAMPLE
     The following statement converts the contents of complex array Z as an
     array of doubles:
       X = mem_peek(mem_base(Z), double, 2, dimsof(Z));
     then:
       X(1,..) is Z.re
       X(2,..) is Z.im

   SEE ALSO reshape, native_byte_order. */

func native_byte_order(type)
/* DOCUMENT native_byte_order()
         or native_byte_order(type)
     Returns the native byte  order, one of: "LITTLE_ENDIAN", "BIG_ENDIAN",
     or  "PDP_ENDIAN".  Optional  argument  TYPE is  an  integer data  type
     (default is long).

   SEE ALSO mem_peek. */
{
  if (is_void(type)) type = long;
  size = sizeof(type);
  (carr = array(char, size))(*) = indgen(size:1:-1);
  value = mem_peek(mem_base(carr), type);
  if (size == 4) {
    if (value == 0x01020304) {
      return "LITTLE_ENDIAN";
    } else if (value == 0x04030201) {
      return "BIG_ENDIAN";
    } else if (value == 0x03040102) {
      return "PDP_ENDIAN";
    }
  } else if (size == 2) {
    if (value == 0x0102) {
      return "LITTLE_ENDIAN";
    } else if (value == 0x0201) {
      return "BIG_ENDIAN";
    }
  }
  error, "unknown byte order";
}

local Y_MMMARK, Y_PSEUDO, Y_RUBBER, Y_RUBBER1, Y_NULLER;
local Y_MIN_DFLT, Y_MAX_DFLT;
extern parse_range;
extern make_range;
/* DOCUMENT arr = parse_range(rng);
         or rng = make_range(arr);

     The parse_range() function converts the index range RNG into an array
     of 4 long integers: [FLAGS,MIN,MAX,STEP].  The make_range() function
     does the opposite.

     For a completely specified range, FLAGS is 1.  Otherwise, FLAGS can be
     Y_MMMARK for a matrix multiply marker (+) -- which is almost certainly a
     syntax error in any other context -- Y_PSEUDO for the pseudo-range index
     (-), Y_RUBBER for the .. index, Y_RUBBER1 for the * index, and Y_NULLER
     for the result of where(0).  Bits Y_MIN_DFLT and Y_MAX_DFLT can be set in
     FLAGS to indicate whether the minimum or maximum is defaulted; there is
     no flag for a default step, so there is no way to tell the difference
     between x(:) and x(::1).

   SEE ALSO: is_range.
*/
Y_MMMARK   =  2;
Y_PSEUDO   =  3;
Y_RUBBER   =  4;
Y_RUBBER1  =  5;
Y_NULLER   =  6;
Y_MIN_DFLT = 16;
Y_MAX_DFLT = 32;

extern make_dimlist;
/* DOCUMENT make_dimlist(arg1, arg2, ...)
         or make_dimlist, arg1, arg2, ...;

     Concatenate all arguments as a single dimension list.  The function
     form returns the resulting dimension list whereas the subroutine form
     redefines the contents of its first argument which must be a simple
     variable reference.  The resulting dimension list is always of the
     form [NDIMS, DIM1, DIM2, ...].


   EXAMPLES

     In the following example, a first call to make_dimlist is needed to
     make sure that input argument DIMS is a valid dimension list if there
     are no other input arguments:

         func foo(a, b, dims, ..)
         {
           // build up dimension list:
           make_dimlist, dims;
           while (more_args()) make_dimlist, dims, next_arg();
           ...;
         }

     Here is an other example:

         func foo(a, b, ..)
         {
           // build up dimension list:
           dims = [0];
           while (more_args()) make_dimlist, dims, next_arg();
           ...;
         }


   SEE ALSO: array, build_dimlist.
 */

extern product;
/* DOCUMENT product(x);

     Yield the product of the elements of X.  Result is a scalar of type
     `long`, `double` or `complex` depending on the type of the elements of X
     (integer, floating-point or complex).

   SEE ALSO: sum.
*/

/*---------------------------------------------------------------------------*/
/* COMPLEX NUMBERS */

func make_hermitian(z, half=, method=, debug=)
/* DOCUMENT zp = make_hermitian(z)
         or make_hermitian, z;

     Insure that complex array Z is hermitian (in the FFT sense).  The
     resulting hermitian array ZP is such that:

         ZP(kneg(k)) = conj(ZP(k))

     where k is the index of a given FFT frequency U and kneg(k) is the
     index of -U.  When called as a subroutine, the operation is made
     in-place.  Input array Z must be complex.

     The particular method used to apply the hermitian constraint can be
     set by keyword METHOD.  Use METHOD = 0 or undefined to "copy" the
     values:

         ZP(k) = Z(k)
         ZP(kneg(k)) = conj(Z(k))

     for all indices k such that k < kneg(k) -- in words, the only relevant
     values in input array Z are those which appear before their negative
     frequency counterpart.  Use METHOD = 1 to average the values:

         ZP(k) = (Z(k) + conj(Z(kneg(k))))/2
         ZP(kneg(k)) = (conj(Z(k)) + Z(kneg(k)))/2

     Finally, use METHOD = 2 to "sum" the values (useful to make a gradient
     hermitian):

         ZP(k) = Z(k) + conj(Z(kneg(k)))
         ZP(kneg(k)) = conj(Z(k)) + Z(kneg(k))

     Set keyword HALF true to indicate that only half the Fourier
     frequencies are stored into Z.  This is for instance the case when
     real FFTW is used.


   SEE ALSO: fft, fftw.
 */
{
  if (! am_subroutine()) {
    /* force copy */
    insure_temporary, z;
  }

  /* Compute indices of negative frequencies starting by the last
     dimension. */
  local u, v;
  dimlist = dimsof(z);
  if ((n = numberof(dimlist)) <= 1) {
    /* Same as if: u = v = 1 below */
    z.im = 0.0;
    return z;
  }
  flag = 0n;
  for (k = n; k >= 2; --k) {
    dim = dimlist(k);
    /* Compute index J of negative frequency along that dimension. */
    if (k == 2) {
      /* Cope with first dimension.  Index J is 1-based. */
      if (half) {
        /* Only zero-th frequency along the first dimension has possibly a
           negative counterpart in the same array. */
        j = [1];
      } else {
        (j = indgen(dim+1:2:-1))(1) = 1;
      }
    } else {
      /* Other dimension than the first one.  Index J is zero-based. */
      (j = indgen(dim:1:-1))(1) = 0;
    }
    if (flag) {
      v = j + dim*v(-,..);
    } else {
      v = j;
      flag = 1n;
    }
  }
  u = array(long, dimsof(v));
  if (half) {
    stride = dimlist(2);
    u(*) = indgen(1 : 1 + stride*(numberof(u) - 1) : stride);
  } else {
    u(*) = indgen(numberof(u));
  }
  if (debug) {
    return [&u, &v];
  }

  /* Fix frequencies which must be real (at least zero-th frequency). */
  z(u(where(u == v))).im = 0.0;

  /* Fix other frequencies. */
  j = where(u < v);
  if (is_array(j)) {
    u = u(j);
    v = v(j);
    if (! method) {
      /* apply "copy" method */
      z(v) = conj(z(u));
    } else if (method == 1) {
      /* apply "average" method */
      tmp = 0.5*(z(u) + conj(z(v)));
      z(u) = tmp;
      z(v) = conj(tmp);
    } else if (method == 2) {
      /* apply "sum" method */
      tmp = z(u) + conj(z(v));
      z(u) = tmp;
      z(v) = conj(tmp);
    } else {
      error, "bad METHOD";
    }
  }
  return z;
}
