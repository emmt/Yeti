* New `tuple` function to create lightweight tuple-like objects.
* Yeti can be compiled from anywhere (just run the `configure` script in the
  directory where you want to build Yeti and its components).
* The `yeti_` prefix has been removed from all `yeti_*.i` files.  If you rely
  on auto-load and do not explicitly `include` such files, this should be
  automatically handled for you.
* Yeti components are now independent from Yeti itself (they all have their own
  auto-load file, the name of associated dynamic library is `yor_$pkg.i` with
  `$pkg` the name of the component).
* Type `make install-doc` to build and install the documentation of the chosen
  components.

## 2018-03-09: Yeti version 6.4.1 released.
* Add autostart file and logo.
* Improve `anonymous` to have no side effects.
* `fullsizeof` can now cope with structures.
* Cleanup and convert NEWS to [Markdown](https://en.wikipedia.org/wiki/Markdown).

## 2015-06-02: Yeti version 6.4.0 released.
* Yeti is managed as a Git repository on [GitHub](https://github.com/emmt/Yeti).
* GSL support abandoned (use [YGSL](https://github.com/emmt/gsl) plugin instead).

## 2013-09-13: Yeti version 6.3.3 released.
* New function: anonymous to create anonymous functions.
  - New function: `h_save` to save variables to a hash-table.
  - New function: `h_functor` to create functor objects.
* Manage to autoload `yeti_yhdf.i` whenever `yhdf_*` functions are called.
* Add support for saving/restoring the evaluator of the hash table in a Yeti
  Hierarchical Data File.

## 2010-01-31: Yeti version 6.3.2 released.
* New `configure` script to mimics GNU-configure.
  - Heavy cleanup of the README file ;-)
* Function `set_alarm` removed, you should use Yorick's own `after` function
  which works better and has more features.
* Removed some old (unused) stuff to allow for compilation of a stand-alone
  Yorick+Yeti interpreter (thanks to Dave Munro for the fix).

## 2010-04-16: Yeti version 6.3.1 released.
* Memory leak in hash table code fixed.
  - Integer indexing of hash table has been removed.
  - Improved robustness of hash tables with respect to interrupts.

## 2010-04-13: Yeti version 6.3.0 released.
* Yeti is now under the CeCILL-C license (http://www.cecill.info).
  - Lots of functions have been moved from Yeti to Yorick.
  - Some code cleanup.

## 2009-12-16: Yeti version 6.2.5 released.
* Range objects can be saved into Yeti Hierarchical Data Files.
* New built-in functions: `parse_range` and `make_range`.

## 2009-12-09: Yeti version 6.2.4 released.
* New function `morph_enhance` to perform non-linear noise reduction on a 2D/3D
  array.
* Changes names of private functions in `yeti_sort.c` to prevent conflict with
  stdlib.h in Mac-OS-X.
* Some hash table functions have been fixed.
* Many small corrections in the documentation in `yeti.i`.
* New functions to save/restore sparse matrices in/from files: `sparse_save`,
  `sparse_restore`.

## 2008-10-29: Yeti version 6.2.3 released.
* Fixed bug in `h_next` which trigger on 64-bit machines.
* Some update in hash table documentation.
* New functions:
  - `h_grow`: grow a member of a hash table;
  - `mem_clear`: clear some global symbols;
  - `fullsizeof`: compute size of arrays, lists, hash-tables, ...
  - `make_hermitian`: make an array Hermitian.
* Some documentation fixed.
* Function `mem_info` correctly accounts for hash-tables, pointers and lists.
* Fix `symbol_names` not reporting scalar symbols.
* Add an error handler in GSL interface to avoid GSL aborting on error.

## 2008-02-14: Yeti version 6.2.2 released.
* Fix functions: identof, `is_scalar`, `is_vector`, `is_matrix`, `is_integer`,
  `is_real`, `is_complex`, `is_string`, and `is_numerical` when argument is an
  L-value. Thanks to "sguieu" for reporting this bug on Yorick forum at
  SourceForge.
* Functions `mem_copy` and `mem_copy` also fixed to prevent this problem.
* Cleanup of code to manage dimension lists (restricted functions to:
  `yeti_reset_dimlist`, `yeti_grow_dimlist` and `yeti_start_dimlist`).  Get rid
  of `ynew_dim` dependency which will be soon removed from Yorick's core.
* Minor fix in `window_geometry.`
* Configuration script fixed to allow for spaces in path to Yorick executable
  and modified to have defaults for FFTW/GSL/TIFF linker flags (i.e. so that
  only `--with-fftw` is needed if FFTW is installed in standard place).
* Function `h_show` now displays name of symbolic links.
* New function `h_grow` to grow contents of hash table members.

## 2007-07-27: Yeti version 6.2.2pre1 released.
* Script config.i, files config.h.in and Makefile.in updated to account for
  version numbers in the form: MAJOR.MINOR.MICROSUFFIX.
* New `global` variable `YETI_VERSION_SUFFIX`.
* New `global` variables: `YETI_VERSION_MAJOR`, `YETI_VERSION_MINOR` and
  `YETI_VERSION_MICRO`.
* New functions (`rgl_roughness_*`) for regularization based on roughness with
  various norms and boundary conditions.

## 2007-05-14: Yeti version 6.2.1 released.
* Configuration script fixed to work with Cygwin/MS-Windows and to allow for
  compilation with Yorick CVS version.
* Yeti Hierarchical Data File (YHDF) can now store functions and symbolic links
  (by their names).
* *** POSSIBLE INCOMPATIBILITY *** Due to inconsistencies, the API for symbolic
  links has been reworked.  To create a symbolic link to a variable, the
  ambiguous `link` function has been deleted in favor of `symlink_to_variable`
  and `symlink_to_name`.  The functions `link_name`, `solve_link` and `is_link`
  have been renamed as `name_of_symlink`, `value_of_symlink` and `is_symlink`
  respectively.

## 2007-04-24: Yeti version 6.2.0 released.
* Add rules to build and install documentation of Yeti plugins in Yorick
  installation directory.
* The `symbol_names` function can now specifically select lists, hash tables
  and/or auto-loaded functions.
* The `about` routine now account for auto-loaded functions.
* Bug in function `make_dimlist` fixed.
* Hash table objects can now have their own evaluator, which can be queried/set
  by the `h_evaluator` function.
* New function `h_number` to query number of entries in a hash table.
* Function `is_hash` returns 2 for a hash table object implementing
  its own evaluator.
* `h_clone`, `h_copy`, `h_info` and `h_show` fixed to account for the evaluator
  of an hash table object.
* Restricted possible values for JOB/FLAGS in evaluation of matrix products:
  must be 0 (default) for direct product and 1 for transpose product.
* New interpreted function: `setup_package`.
* New object type: symbolic link.  New related functions: `link`, `is_link`,
  `link_name` and `solve_link`.

## 2006-12-17: Yeti version 6.1.7 released.
* Renamed built-in `typeIDof` as `identof` and provide constants for `T_CHAR`,
  `T_SHORT`, ...

## 2006-07-19: Yeti version 6.1.6 released.
* New builtin function `insure_temporary`.
* New builtin function `quick_select`.  New functions `quick_median` and
  `quick_interquartile_range` for fast estimation of the median and the
  inter-quartile range of an array of values.

## 2006-06-10: Yeti version 6.1.5 released.
This is the first public release of Yeti which makes use of the new Yorick API
"YAPI" and which provides support for GSL (the GNU Scientific Library).
* Documentation for Yeti-GSL completed.
* Fixed inconsistency in error computation for Yeti-GSL.
* New function `h_show` to display a hash table as an expanded tree.
* Built-in function `is_list` removed from Yeti (it is now part of Yorick).
* New built-in functions: `make_dimlist`.
* New built-in functions: `window_select`, `window_exists and window_list`.
* Changed `yeti_tiff` plugin so that it uses new Yorick API "YAPI".
* In `tiff_plugin`, TIFF objects can now be indexed by their tag names.
* Changed `yeti_gsl` plugin so that it uses new Yorick API "YAPI".
* New plugin `yeti_gsl` which implements support for GNU Scientific
* Library.  At this time, the plugin provides 118 special functions from the
  GSL.
* Fix bug in `sparse_expand()`.
* In `about()` function, explicitely filter range operators which have a built-in
  function counterpart to avoid a deadly bug in Yorick.

## 2005-09-16: Yeti version 6.0.2 released.
* Use Yorick (instead of Bourne shell) for configuration of Yeti -- much more
  easy and portable ;-)
* New built-in function `machine_constant` to query machine dependent constant
  (such as `DBL_EPSILON`).
* Fixed signedness of strings in `yeti_hash.c` to avoid compiler warnings.
* Fixed bad variable name in `morph_white_top_hat`, and `morph_black_top_hat`.

## 2005-07-30: Yeti version 6.0.0 released.
* New functions `cost_l2`, `cost_l2l1` and `cost_l2l0` to compute, L2, L2-L1
  and L2-L0 cost functions and optionally their gradient.
* *** POSSIBLE INCOMPATIBILITY *** The old sparse matrix API has been removed
  in favor of a new sparse_matrix object class.  The new class is different
  from the old implementation: [1] it has two index lists (one for the rows,
  the other for the columns of the non-zero coefficients) to break the former
  limit on the dimension lists of input/output spaces; [2] the sparse matrix
  object can be used directly as a function to apply (possibly transpose)
  matrix multiplication; [3] the sparse matrix object can be used as a Yorick
  structure to query its contents.
* *** POSSIBLE INCOMPATIBILITY *** The built-in function `_yeti_init` was
  renamed as `yeti_init`.
* New morpho-math functions: `morph_dilation`, `morph_erosion`,
  `morph_opening`, `morph_closing`, `morph_white_top_hat`,
  `morph_black_top_hat`.
* *** POSSIBLE INCOMPATIBILITY *** Built-in `get_path` removed (conflict with
  same function in Yorick 1.6.02).  The new `get_path` function return a list
  of path separated by `:` into a single string whereas the former version
  built-in Yeti used to return an array of strings.
* Lots of changes to make Yeti a pure plugin for Yorick 1.6 and
  trying to maintain compatibility with previous version of Yeti:
  - Temporarily removed all the `chn_*` function (channel).
  - Builtin `expand_path` removed in favor of `filepath`.
  - Built-in `strlower` and `strlower` replaced by interpreted version which
    use the new `strcase` built-in function in Yorick-1.6. Similarly, `strtrim`
    removed because it is now provided by Yorick-1.6 with same behaviour.
  - `yeti_fftw` and `yeti_tiff` packages no longer require Yeti to be loaded
    (they are pure standalone Yorick packages).
  - Regular expression functions `regmatch`, `regsub`, `regcomp`, etc are now
    provided by a separate package `yeti_regex`.  Two reasons for that: (1) new
    Yorick-1.6 now provides some regular expression engine (see strgrep), and
    (2) my regular expression package required the GNU REGEX library (built-in
    GNU C library).  The GNU REGEX library can be optionally built into the new
    `yeti_regex` package (i.e. you no longer need an external REGEX library).
    Also note that the `yeti_regex` does not require Yeti to be loaded (it is a
    pure standalone Yorick package).
* New Built-in functions: `is_complex`, `is_hash`, `is_integer`, `is_list`,
  `is_matrix`, `is_numerical`, `is_real`, `is_scalar`, `is_string`,
  `is_vector`.

## 2005-05-24: yeti-5.3.12 released.
* Changes in `yeti_plugin.c`, `yeti_misc.c`, `yeti_utils.c` and `yeti.i` to
  avoid problems on 64-bit machines.  A (minor) side effect is that addresses
  in `mem_*` hack functions must now be long integer (*i.e.*, int's are no
  longer allowed).
* Fix bugs caused by using a nil string (*e.g.*, `string(0)`) as hash key and which
  trigger segmentation violation interrupt (`SIGSEGV`).
* New functions `h_first` and `h_next` to scan a hash table.
* Support for reloadable plugins.
* Complete rewrite of `configure` script.
* CP40/GPL support is now provided as a separate plugin.
* Startup routine `_yeti_init` to provide a more useful list of paths for
  script files (`*.i`) than the one setup by Yorick.
* Removed `yeti_version` function in favor of `_yeti_init` function and
  pre-defined variables `YETI_VERSION` and `YETI_HOME`.
* New function `get_includes` to get a list of included files.

## 2004-09-27: yeti-5.3.11 released.
* Fix a bug in `arc` and `round` built-in functions which triggers when the
  argument was a non-double scalar (thanks to Clémentine Béchet for finding
  this bug).
* `about` interpreted function can optionally ignore case.
* New interpreted function `h_clone` to clone/copy a hash object.
* Work around Yorick bug with palette in `gg_fit_2d_spike`.

## 2004-09-09: yeti-5.3.10 released.
* Yeti now displays its version number which can be retrieved by function
  `yeti_version`.
* New built-in function `current_include` to get the path of the currently
  parsed file if any.
* New built-in function `arc` to compute lengh of arc in radians.
* Fix built-in `heapsort` to always return a vector (as claimed in the doc).
* Avoid Gist markers when drawing widgets in `yeti_gist_gui.i`.
* Hash table objects can be invoked as a function with a member name (syntaxic
  shortcut for `h_get`) or with a nil argument to get the number of elements.

## 2004-02-23: yeti-5.3.8 released.
* Added built-in function `eigen` to compute the spectral decomposition of real
  symetric matrices.
