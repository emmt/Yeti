/* config.i -
 *
 * Configuration script for setting up building of Yeti.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 1996-2010 Eric Thiébaut <thiebaut@obs.univ-lyon1.fr>
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can use, modify
 * and/or redistribute the software under the terms of the CeCILL-C license as
 * circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty and the software's author, the holder of the
 * economic rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more
 * generally, to use and operate it in the same conditions as regards
 * security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/*---------------------------------------------------------------------------*/
/* GLOBAL CONFIGURATION PARAMETERS */

/* path to Yorick executable: */
local CFG_YORICK;

/* Yorick version: */
local CFG_YORICK_VERSION;
local CFG_YORICK_VERSION_MAJOR, CFG_YORICK_VERSION_MINOR, CFG_YORICK_VERSION_MICRO;

/* Yeti version: */
local CFG_YETI_VERSION;
local CFG_YETI_VERSION_MAJOR, CFG_YETI_VERSION_MINOR, CFG_YETI_VERSION_MICRO;

/* Settings for FFTW plugin: */
local CFG_WITH_FFTW, CFG_WITH_FFTW_DEFS, CFG_WITH_FFTW_LIBS;
CFG_WITH_FFTW = "no";
CFG_WITH_FFTW_DEFS = "";
CFG_WITH_FFTW_LIBS = "-lrfftw -lfftw";

/* Settings for REGEX plugin: */
local CFG_WITH_REGEX, CFG_WITH_REGEX_DEFS, CFG_WITH_REGEX_LIBS;
CFG_WITH_REGEX = "yes";
CFG_WITH_REGEX_DEFS = "";
CFG_WITH_REGEX_LIBS = "";

/* Settings for TIFF plugin: */
local CFG_WITH_TIFF, CFG_WITH_TIFF_DEFS, CFG_WITH_TIFF_LIBS;
CFG_WITH_TIFF = "no";
CFG_WITH_TIFF_DEFS = "";
CFG_WITH_TIFF_LIBS = "-ltiff";

/*---------------------------------------------------------------------------*/
/* HELP AND MAIN CONFIGURATION FUNCTIONS */

func cfg_help {
  w = cfg_prt; /* shortcut */
  w, "Usage: yorick -batch ./config.i [OPTIONS]";
  w, "  Configure Yeti software for compilation and installation.  The default";
  w, "  settings shown in brackets below are based on values set by environment";
  w, "  variables or guessed by this script.";
  w, "";
  w, "Options: (default values shown in brackets)";
  w, "  -h, --help              print this help summary";
  w, "  --debug                 turn on debug mode for this script";
  w, "  --yorick=PATH           path to Yorick executable [%s]", CFG_YORICK;
  w, "";
  w, "  --with-fftw=yes/no      build FFTW plugin? [%s]", CFG_WITH_FFTW;
  w, "  --with-fftw-defs=DEFS   preprocessor options for FFTW [%s]", CFG_WITH_FFTW_DEFS;
  w, "  --with-fftw-libs=LIBS   library specification for FFTW [%s]", CFG_WITH_FFTW_LIBS;
  w, "";
  w, "  --with-regex=yes/no     build REGEX plugin? [%s]", CFG_WITH_REGEX;
  w, "  --with-regex-defs=DEFS  preprocessor options for REGEX [%s]", CFG_WITH_REGEX_DEFS;
  w, "  --with-regex-libs=LIBS  library specification for REGEX [%s]", CFG_WITH_REGEX_LIBS;
  w, "";
  w, "  --with-tiff=yes/no      build TIFF plugin? [%s]", CFG_WITH_TIFF;
  w, "  --with-tiff-defs=DEFS   preprocessor options for TIFF [%s]", CFG_WITH_TIFF_DEFS;
  w, "  --with-tiff-libs=LIBS   library specification for TIFF [%s]", CFG_WITH_TIFF_LIBS;
  w, "";
  w, "Alternative syntax:";
  w, "  --with-PACKAGE          same as --with-PACKAGE=yes";
  w, "  --without-PACKAGE       same as --with-PACKAGE=no";
}

func cfg_configure(argv)
{
  extern CFG_DIR;
  extern CFG_YORICK;
  extern CFG_YORICK_VERSION;
  extern CFG_YORICK_VERSION_MAJOR, CFG_YORICK_VERSION_MINOR, CFG_YORICK_VERSION_MICRO;
  extern CFG_YETI_VERSION;
  extern CFG_YETI_VERSION_MAJOR, CFG_YETI_VERSION_MINOR, CFG_YETI_VERSION_MICRO;
  extern CFG_WITH_FFTW, CFG_WITH_FFTW_DEFS, CFG_WITH_FFTW_LIBS;
  extern CFG_WITH_REGEX, CFG_WITH_REGEX_DEFS, CFG_WITH_REGEX_LIBS;
  extern CFG_WITH_TIFF, CFG_WITH_TIFF_DEFS, CFG_WITH_TIFF_LIBS;

  CFG_YORICK = argv(1);
  CFG_DIR = get_cwd();
  CFG_TMP = "config.tmp";
  CFG_DEBUG = 0n;

  pkg_list = ["fftw", "regex", "tiff"];

  nil = string();
  s = string();
  argc = numberof(argv);
  for (i=2 ; i<=argc ; ++i) {
    arg = argv(i);
    if (arg == "-h" || arg == "--help") {
      cfg_help;
      if (batch()) quit;
    } else if (arg == "--debug") {
      CFG_DEBUG = 1n;
    } else if ((s = cfg_split(arg, "--yorick="))) {
      CFG_YORICK = s;
    } else if ((s = cfg_split(arg, "--with-"))) {
      sel = strfind("=", s);
      if (sel(2) < 0) {
        cfg_define, "CFG_WITH_" + s, "yes";
      } else if (sel(1) > 0) {
        cfg_define, "CFG_WITH_" + strpart(s, 1:sel(1)), strpart(s, sel(2)+1:0);
      } else {
        cfg_die, "bad option \"", arg, "\"";
      }
    } else if ((s = cfg_split(arg, "--without-"))) {
      cfg_define, "CFG_WITH_" + s, "no";
    } else {
      cfg_die, "unknown option: \"", arg, "\"; try \"--help\"";
    }
  }

  /* Get version of Yeti. */
  CFG_YETI_VERSION = rdline(open("VERSION"));
  cfg_parse_version, "CFG_YETI", CFG_YETI_VERSION;

  /* Get version of Yorick executable. */
  if (structof(CFG_YORICK) != string) {
    CFG_YORICK_VERSION = nil;
  } else {
    write, format="%s\n", open(CFG_TMP, "w"), "write, format=\"%s\", Y_VERSION;"
    CFG_YORICK_VERSION = rdline(popen("\"" + CFG_YORICK + "\" -batch " + CFG_TMP, 0));
    if (! CFG_DEBUG) remove, CFG_TMP;
  }
  if (! CFG_YORICK_VERSION) cfg_die, "bad path to Yorick executable";
  cfg_parse_version, "CFG_YORICK", CFG_YORICK_VERSION;
  if (CFG_YORICK_VERSION_MAJOR < 1
      || (CFG_YORICK_VERSION_MAJOR == 1 && CFG_YORICK_VERSION_MINOR < 6)) {
    cfg_die, "too old Yorick version (upgrade to at least Yorick 1.6-02)";
  }

  /* Figure out some builtin capabilities of Yorick. */
  CFG_PROVIDE_ROUND = (is_func(round) == 2);
  CFG_PROVIDE_IDENTOF = (is_func(identof) == 2);
  CFG_PROVIDE_SWAP = (is_func(swap) == 2);
  CFG_PROVIDE_UNREF = (is_func(unref) == 2);
  CFG_PROVIDE_IS_LIST = (is_func(is_list) == 2);
  CFG_PROVIDE_IS_SCALAR = (is_func(is_scalar) == 2);
  CFG_PROVIDE_IS_VECTOR = (is_func(is_vector) == 2);
  CFG_PROVIDE_IS_MATRIX = (is_func(is_matrix) == 2);
  CFG_PROVIDE_IS_INTEGER = (is_func(is_integer) == 2);
  CFG_PROVIDE_IS_REAL = (is_func(is_real) == 2);
  CFG_PROVIDE_IS_COMPLEX = (is_func(is_complex) == 2);
  CFG_PROVIDE_IS_STRING = (is_func(is_string) == 2);
  CFG_PROVIDE_IS_NUMERICAL = (is_func(is_numerical) == 2);
  CFG_PROVIDE_SYMBOL_EXISTS = (is_func(symbol_exists) == 2);
  CFG_PROVIDE_SYMBOL_NAMES = (is_func(symbol_names) == 2);
  CFG_PROVIDE_GET_INCLUDES = (is_func(get_includes) == 2);
  CFG_PROVIDE_CURRENT_INCLUDE = (is_func(current_include) == 2);
  CFG_PROVIDE_WINDOW_GEOMETRY = (is_func(window_geometry) == 2);
  CFG_PROVIDE_WINDOW_EXISTS = (is_func(window_exists) == 2);
  CFG_PROVIDE_WINDOW_SELECT = (is_func(window_select) == 2);
  CFG_PROVIDE_WINDOW_LIST = (is_func(window_list) == 2);
  if (is_func(random_n) != 3) {
    cfg_die, "expecting that random_n() be an autoloaded function";
  } else {
    CFG_YETI_MUST_DEFINE_AUTOLOAD_TYPE = (nameof(random_n) != "random_n");
  }

  /* Byte order. */
  CFG_BYTE_ORDER = 0;
  n = sizeof(int);
  buf = array(char, n);
  for (i = 1; i <= n; ++i) buf(i) = i;
  val = cfg_cast(buf, int);
  if (n == 2) {
    if (val == 0x0102) {
      CFG_BYTE_ORDER = +1;
    } else if (val == 0x0201) {
      CFG_BYTE_ORDER = -1;
    }
  } else if (n == 4) {
    if (val == 0x01020304) {
      CFG_BYTE_ORDER = +1;
    } else if (val == 0x04030201) {
      CFG_BYTE_ORDER = -1;
    }
  } else if (n == 8) {
    if ((val & 0xFFFFFFFF) == 0x05060708) {
      CFG_BYTE_ORDER = -1;
    } else if ((val & 0xFFFFFFFF) == 0x04030201) {
      CFG_BYTE_ORDER = -1;
    }
  }
  if (! CFG_BYTE_ORDER) {
    cfg_die, "unknown byte order";
  }


  /* Greeting message. */
  cfg_prt;
  cfg_prt, "*** This is the configuration script for Yeti ***";
  cfg_prt;
  cfg_prt, "Yeti version is: %d.%d.%d%s",
    CFG_YETI_VERSION_MAJOR, CFG_YETI_VERSION_MINOR,
    CFG_YETI_VERSION_MICRO, CFG_YETI_VERSION_SUFFIX;
  cfg_prt;
  cfg_prt, "Yorick executable is: %s", CFG_YORICK;
  cfg_prt, "Yorick version is: %d.%d.%d%s",
    CFG_YORICK_VERSION_MAJOR, CFG_YORICK_VERSION_MINOR,
    CFG_YORICK_VERSION_MICRO, CFG_YORICK_VERSION_SUFFIX;

  /* Build/fix Makefile and yeti.h in directory yeti. */
  cfg_prt;
  cfg_prt, "Setup for building Yeti...";
  cfg_change_dir, "core";
  cfg_fix_makefile;
  cfg_change_dir, "doc";
  cfg_fix_makefile;

  yeti_pkgs = "core";
  for (i=1 ; i<=numberof(pkg_list) ; ++i) {
    pkg = pkg_list(i);
    PKG = strcase(1, pkg);
    with_pkg = symbol_def("CFG_WITH_" + PKG);
    if ((s = structof(with_pkg)) == string) {
      with_pkg = (strcase(0, with_pkg) == "yes");
    } else {
      with_pkg = !(! with_pkg);
    }
    cfg_prt;
    def_have = (pkg == "fftw" || pkg == "tiff");
    if (with_pkg) {
      defs = symbol_def("CFG_WITH_" + PKG + "_DEFS");
      libs = symbol_def("CFG_WITH_" + PKG + "_LIBS");
      if (def_have) defs += " -DHAVE_" + PKG + "=1";
      cfg_prt, "Package \"%s\" will be built with the following settings:", pkg;
      cfg_prt, "  PKG_CFLAGS  = %s", defs;
      cfg_prt, "  PKG_DEPLIBS = %s", libs;
      yeti_pkgs += " " + pkg;
    } else {
      cfg_prt, "Package \"%s\" will not be built.", pkg;
      defs = (def_have ? "-DHAVE_" + PKG + "=0" : "");
      libs = "";
    }

    /* Build/fix package Makefile. */
    cfg_change_dir, pkg;
    cfg_fix_makefile;
    cfg_update, "Makefile", "make",
      "PKG_CFLAGS", defs,
      "PKG_DEPLIBS", libs;
  }


  /* Fix top level Makefile and creates config.h. */
  cfg_prt;
  cfg_prt, "Setting up top-level Makefile...";
  cd, CFG_DIR;
  cfg_update, "Makefile.in", "make",
    "YETI_PKGS", yeti_pkgs,
    "YETI_VERSION_MAJOR", CFG_YETI_VERSION_MAJOR,
    "YETI_VERSION_MINOR", CFG_YETI_VERSION_MINOR,
    "YETI_VERSION_MICRO", CFG_YETI_VERSION_MICRO,
    "YETI_VERSION_SUFFIX", "\"" + CFG_YETI_VERSION_SUFFIX + "\"",
    "YORICK_VERSION_MAJOR", CFG_YORICK_VERSION_MAJOR,
    "YORICK_VERSION_MINOR", CFG_YORICK_VERSION_MINOR,
    "YORICK_VERSION_MICRO", CFG_YORICK_VERSION_MICRO,
    "YORICK_VERSION_SUFFIX", "\"" + CFG_YORICK_VERSION_SUFFIX + "\"";
  cfg_fix_makefile;
  cfg_update, "config.h.in", "cpp",
    "YETI_VERSION_MAJOR", CFG_YETI_VERSION_MAJOR,
    "YETI_VERSION_MINOR", CFG_YETI_VERSION_MINOR,
    "YETI_VERSION_MICRO", CFG_YETI_VERSION_MICRO,
    "YETI_VERSION_SUFFIX", "\"" + CFG_YETI_VERSION_SUFFIX + "\"",
    "YORICK_VERSION_MAJOR", CFG_YORICK_VERSION_MAJOR,
    "YORICK_VERSION_MINOR", CFG_YORICK_VERSION_MINOR,
    "YORICK_VERSION_MICRO", CFG_YORICK_VERSION_MICRO,
    "YORICK_VERSION_SUFFIX", "\"" + CFG_YORICK_VERSION_SUFFIX + "\"",
    "YETI_BYTE_ORDER", CFG_BYTE_ORDER,
    "YETI_CHAR_SIZE", sizeof(char),
    "YETI_SHORT_SIZE", sizeof(short),
    "YETI_INT_SIZE", sizeof(int),
    "YETI_LONG_SIZE", sizeof(long),
    "YETI_FLOAT_SIZE", sizeof(float),
    "YETI_DOUBLE_SIZE", sizeof(double),
    "YETI_POINTER_SIZE", sizeof(pointer),
    "YETI_MUST_DEFINE_AUTOLOAD_TYPE", CFG_YETI_MUST_DEFINE_AUTOLOAD_TYPE;
  cfg_prt, "  created config.h";

  /* Final message. */
  cfg_prt;
  cfg_prt, "OK all done, you can build and install Yeti plugin(s) by:";
  cfg_prt, "  make all";
  cfg_prt, "  make install";
  cfg_prt;
}

/*---------------------------------------------------------------------------*/
/* UTILITY FUNCTIONS */

func cfg_prt(x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13)
/* DOCUMENT cfg_prt, str;
       -or- cfg_prt, fmt, arg1, arg2, ...;
     Print out string STR (with no leading space and a terminating newline)
     or arguments ARG1, ARG2, etc with format FMT.

   SEE ALSO: write. */
{
  w = write; /* short alias */
  if (is_void(x0)) { w, format="%s\n", ""; return; }
  if (is_void(x1)) { w, format="%s\n", x0; return; }
  if (strpart(x0, 0:0) != "\n") x0 += "\n";
  /**/ if (is_void(x2))  w,format=x0,x1;
  else if (is_void(x3))  w,format=x0,x1,x2;
  else if (is_void(x4))  w,format=x0,x1,x2,x3;
  else if (is_void(x5))  w,format=x0,x1,x2,x3,x4;
  else if (is_void(x6))  w,format=x0,x1,x2,x3,x4,x5;
  else if (is_void(x7))  w,format=x0,x1,x2,x3,x4,x5,x6;
  else if (is_void(x8))  w,format=x0,x1,x2,x3,x4,x5,x6,x7;
  else if (is_void(x9))  w,format=x0,x1,x2,x3,x4,x5,x6,x7,x8;
  else if (is_void(x10)) w,format=x0,x1,x2,x3,x4,x5,x6,x7,x8,x9;
  else if (is_void(x11)) w,format=x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10;
  else if (is_void(x12)) w,format=x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11;
  else if (is_void(x13)) w,format=x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12;
  else w,format=x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13;
}

func cfg_yes_or_no(arg) { return (arg ? "yes" : "no"); }


func cfg_die(msg, ..)
{
  while (more_args()) msg += next_arg();
  if (batch()) {
    write, format="*** ERROR *** %s\n", msg;
    quit;
  } else {
    error, msg;
  }
}

func cfg_parse_version(prefix, version) {
  extern CFG_YORICK_VERSION_MAJOR, CFG_YORICK_VERSION_MINOR,
    CFG_YORICK_VERSION_MICRO, CFG_YORICK_VERSION_SUFFIX;
  extern CFG_YETI_VERSION_MAJOR, CFG_YETI_VERSION_MINOR,
    CFG_YETI_VERSION_MICRO, CFG_YETI_VERSION_SUFFIX;
  major = 0L;
  minor = 0L;
  micro = 0L;
  suffix = dummy = string();
  n = sread(version, format="%d.%d.%d%s%s", major, minor, micro, suffix, dummy);
  if (n != 3 && n != 4) {
    cfg_die, "bad version \"", version, "\" for ", prefix;
  }
  symbol_set, prefix + "_VERSION_MAJOR", major;
  symbol_set, prefix + "_VERSION_MINOR", minor;
  symbol_set, prefix + "_VERSION_MICRO", micro;
  symbol_set, prefix + "_VERSION_SUFFIX", (suffix ? suffix : "");
}

func cfg_load(filename, grain)
/* Load text file. */
{
  buf = [];
  if (! (file = open(filename, "r", 1))) {
    cfg_die, "cannot open file \"" + filename + "\" for reading";
  }
  if (! grain) grain = 10;
  while (1n) {
    tmp = rdline(file, grain);
    i = where(tmp);
    if (! is_array(i)) return buf;
    grow, buf, tmp(1:i(0));
    /* In order to avoid O(N^2) process, adjust number of lines read at a
       time so as to double the total number of lines read after next
       rdline: */
    grain = numberof(buf);
  }
}

func cfg_update(target, style, ..)
{
  if (style == "cpp") {
    fmt1 = "^#[ \t]*define[ \t][ \t]*%s([ \t]|$)";
    fmt2 = "#define %s %s";
  } else if (style == "sh") {
    fmt1 = "^[ \t]*%s=";
    fmt2 = "%s=%s";
  } else if (style == "make") {
    fmt1 = "^[ \t]*%s[ \t]*=";
    fmt2 = "%s = %s";
  } else {
    cfg_die, "cfg_update: bad value for STYLE parameter";
  }

  /* Load text file. */
  buf = cfg_load(target);

  while (more_args()) {
    macro = next_arg();
    if (! more_args()) cfg_die, "cfg_update: missing macro value";
    value = next_arg();
    if ((s = structof(value)) != string) {
      if (s==long || s==int || s==short || s==char) {
        value = swrite(format="%d", value);
      } else {
        cfg_die, "unexpected data type for value of macro ", macro;
      }
    }
    pat = swrite(format=fmt1, macro);
    i = where(strgrep(pat, buf)(2,) > 0);
    if (is_array(i)) buf(i) = swrite(format=fmt2, macro, value);
  }

  /* Replace old file by new one. */
  if (strlen(target) > 3 && strpart(target, -3:-3) != "/"
      && strpart(target, -2:0) == ".in") {
    dest = strpart(target, 1:-3);
    write, open(dest, "w"), format="%s\n", buf;
  } else {
    backup = target + ".bak";
    temporary = target + ".tmp";
    write, open(temporary, "w"), format="%s\n", buf;
    rename, target, backup;
    rename, temporary, target;
    if (! CFG_DEBUG) remove, backup;
  }
}

func cfg_split(arg, prefix)
{
  len = strlen(prefix);
  if (strpart(arg, 1:len) == prefix) {
    return strpart(arg, len+1:0);
  }
}

func cfg_define(name, value)
{
  c = strchar(strcase(1, name));
  if (is_array((i = where((c == '-'))))) c(i) = '_';
  symbol_set, strchar(c), value;
}

func cfg_change_dir(dir)
{
  write, format="  entering directory: %s\n", dir;
  cd, CFG_DIR;
  cd, dir;
}

func cfg_fix_makefile
{
  write, format="%s", "  ";
  system, "\"" + CFG_YORICK + "\" -batch make.i";
}

func cfg_cast(a, type, dimlist)
/* DOCUMENT cfg_cast(a, type, dims)
     This function returns array A reshaped to an array with given TYPE and
     dimension list.

   SEE ALSO: reshape.
 */
{
  local r;
  reshape, r, &a, type, dimlist;
  return r;
}

/*---------------------------------------------------------------------------*/
/* CLOSURE (run the configuration script if in batch mode) */

if (batch()) {
  cfg_configure, get_argv();
  quit;
}
