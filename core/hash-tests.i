require, "yeti-testing.i";

func eval_me(self, x, y)
{
  return abs(self.x - x, self.y - y);
}
func call_me(self, x, y)
{
  return sin(self.x - x)*cos(self.y - y);
}

func test1
{
  /* create a hash table object */
  tab = h_new(x=[1,4], y=[7,9]);

  /* set evaluator (use function name since
     it does not yet exists) */
  h_evaluator, tab, "eval_me";

  tab(4,11);
  h_show, tab;

  other = h_new(x=3.6, y=11.0);
  h_evaluator, other, call_me;
}


func db_get_member(db, name, type=, keys=)
{
  if (is_void(keys)) {
    keys = h_keys(db);
    n = numberof(keys);
    if (n > 1) keys = keys(sort(keys));
  } else {
    n = numberof(keys);
  }
  if (n) {
    x = array((is_void(type) ? double : type), n);
    for (i=1 ; i<=n ; ++i) {
      x(i) = h_get(h_get(db, keys(i)), name);
    }
    return x;
  }
}

func h_analyse(s)
{
  if (is_hash(s)) {
    s = h_stat(s);
  } else if (! is_integer(s)) {
    error, "expecting a hash table or an array with hash statistics";
  }
  n = numberof(s);
  q = double(indgen(0:n-1));
  write, "number = ", sum(q*s);
  write, "empty  = ", s(1);
  write, "avg    = ", sum(q*q*s)/sum(q*s);
}

func _h_test_eval1(self, key) { return h_get(self, key); }

func h_test(repeat)
{
  require, "utils.i"; // for stat, timer_elapsed, ...
  if (is_void(repeat)) repeat = 1;
  names = ["B_nu", "B_nu_bar", "B_nu_scale", "GISTPATH", "GIST_FORMAT",
           "HX_blkbnd", "HX_block", "LPR_FORMAT", "LUrcond", "LUsolve",
           "PS2EPSI_FORMAT", "QRsolve", "Ray_Path", "SVdec", "SVrank",
           "SVsolve", "TDsolve", "Y_LAUNCH", "Y_SITE", "Y_VERSION", "_",
           "__alpha", "__cray", "__dec", "__i86", "__ibmpc", "__mac",
           "__macl", "__sgi64", "__sun", "__sun3", "__vax", "__vaxg", "__xdr",
           "_car", "_cat", "_cdr", "_cpy", "_dgecox", "_dgelss", "_dgelx",
           "_dgesv", "_dgesvx", "_dgetrf", "_dgtsv", "_get_matrix",
           "_get_msize", "_init_clog", "_init_drat", "_init_pdb", "_jc",
           "_jr", "_jt", "_len", "_lst", "_map", "_not_cdf", "_not_pdb",
           "_not_pdbf", "_nxt", "_pl_init", "_plmk_color", "_plmk_count",
           "_plmk_markers", "_plmk_msize", "_plmk_width", "_prt",
           "_raw1_flat", "_raw1_linear", "_raw2_flat", "_raw2_linear",
           "_raw_pcens", "_raw_track", "_ray_integ", "_ray_reduce", "_read",
           "_rev", "_roll2", "_set_pdb", "_timer_elapsed", "_to_real_system",
           "_write", "a", "abs", "acos", "acosh", "add_member",
           "add_next_file", "add_record", "add_variable", "adjust",
           "adjust_ireg", "akap", "alloc_mesh", "allof", "alpha_primitives",
           "am_subroutine", "anh", "animate", "anorm", "anyof", "apply_funcs",
           "area", "array", "arrowl", "arroww", "asin", "asinh", "aspect",
           "at_pdb_close", "at_pdb_open", "atan", "atanh", "auss_tsigma",
           "avg", "ax", "b", "b_optional", "backup", "base", "batch", "bins",
           "blks", "block", "bn", "bnds", "bndy", "bnu", "boltz", "bookmark",
           "bound", "bounds", "bp", "break", "bytscl", "c", "c_adjust",
           "call", "catch", "cc", "cd", "ceil", "cell", "cells", "changed",
           "char", "clogfile", "close", "close102", "close102_default",
           "closed", "cmax", "cmin", "collect", "color", "color_bar",
           "colors", "command_line", "complex", "conj", "continue", "contour",
           "conv3_rays", "copyright", "cos", "cosh", "cray_primitives",
           "create", "createb", "cs_adjust", "csch", "cum", "current_window",
           "d", "data_align", "dbauto", "dbcont", "dbdis", "dbdt", "dbexit",
           "dbinfo", "dbret", "dbskip", "dbup", "dd", "dec_primitives",
           "default_gate", "default_integrate", "default_ocompute", "dif",
           "digitize", "dims", "dimsof", "dirs", "disassemble", "display",
           "do", "double", "drat_akap", "drat_amult", "drat_backlight",
           "drat_channel", "drat_compress", "drat_ekap", "drat_emult",
           "drat_gate", "drat_gav", "drat_gb", "drat_glist", "drat_integrate",
           "drat_ireg", "drat_ireg_adj", "drat_isymz", "drat_khold",
           "drat_lhold", "drat_linear", "drat_nomilne", "drat_oadjust",
           "drat_ocompute", "drat_omult", "drat_quiet", "drat_rt",
           "drat_start", "drat_static", "drat_stop", "drat_symmetry",
           "drat_zt", "ds", "dummy", "dump", "dump_clog", "dx", "dy", "dz",
           "e", "e30", "ecolor", "edges", "edit_times", "ee", "ekap",
           "elapsed", "else", "eps", "eq_nocopy", "erfc", "error", "ewidth",
           "exist", "exit", "exp", "expm1", "extern", "f", "f__map", "f_save",
           "face", "ff", "fflush", "fft", "fft_braw", "fft_dirs", "fft_fraw",
           "fft_init", "fft_inplace", "fft_raw", "fft_setup", "fi", "file",
           "filename", "final", "find_boundary", "first", "flip", "floor",
           "fma", "font", "for", "form_mesh", "format", "fudge", "full",
           "func", "gauss_gate", "gauss_int", "gauss_norm", "gauss_t0",
           "gauss_tsigma", "gaussian_gate", "gav", "gb", "get_addrs",
           "get_argv", "get_command_line", "get_cwd", "get_env", "get_home",
           "get_kaps", "get_member", "get_ncycs", "get_primitives",
           "get_ray_path", "get_s0", "get_std_limits", "get_times",
           "get_vars", "gotopen", "gridxy", "group", "grow", "guess_symmetry",
           "has_ireg", "has_records", "has_time", "hcp", "hcp_file",
           "hcp_finish", "hcp_out", "hcpoff", "hcpon", "hcps", "height",
           "help", "help_file", "help_topic", "help_worker", "hex24b_track",
           "hex24f_track", "hex5_track", "hex_mesh", "hex_mesh2", "hex_query",
           "hex_startflag", "hex_triang", "hide", "histeq_scale", "histogram",
           "hnu", "hnub", "hollow", "hooks", "how", "hydra_adj", "hydra_blks",
           "hydra_bnd", "hydra_mesh", "hydra_mrk", "hydra_start", "hydra_xyz",
           "i", "i0", "i86_primitives", "ic", "if", "ijk_max", "ijx", "im",
           "im_part", "ims", "imsof", "include", "indent", "indgen", "info",
           "install_struct", "instant", "integ", "integ_flat", "integ_linear",
           "integrator", "intens", "internal_rays", "interp", "irays", "ireg",
           "is_array", "is_complex", "is_func", "is_present", "is_range",
           "is_stream", "is_struct", "is_void", "item", "iwork", "ix", "j",
           "j0", "jc", "jr", "jt", "justify", "jx", "k0", "kc", "keep",
           "keybd_focus", "khold", "kmax", "kt", "kxlm", "l", "label", "labl",
           "labs", "last", "ldims", "ldvt", "legal", "legend", "legends",
           "length", "levs", "lhold", "library", "limits", "line", "list",
           "list__map", "ljdir", "ljoff", "lm", "lmax", "local", "log",
           "log10", "logxy", "long", "lsdir", "lwork", "m", "mac_primitives",
           "macl_primitives", "make_sphere", "mark", "marker", "marks",
           "mask", "max", "max_trans", "mbnds", "mcolor", "median", "merge",
           "merge2", "mesh", "mesh_loc", "min", "mkdir", "mnmax", "more_args",
           "mouse", "moush", "mphase", "msg", "msize", "mspace", "n", "n0",
           "n1", "n2", "n3", "n4", "n5", "n6", "n7", "n8", "n9", "nallof",
           "name", "nameof", "nblk", "nbnds", "nc", "ncyc", "ndb", "ndims",
           "nedges", "next_arg", "ng", "ngroup", "ni", "ni1", "nij", "nj",
           "nj1", "njk", "nk", "nk1", "nki", "nlist", "nolj", "nomilne",
           "noneof", "norj", "nrays", "nrhs", "ntot", "nub", "numberof", "nz",
           "nzones", "o", "odd", "old", "one_norm", "opac", "opaque", "open",
           "open102", "openb", "openb_hooks", "orgs", "orgsof", "orient",
           "orig", "ouble", "outname", "p", "pair", "pairs", "palette", "pat",
           "path", "pause", "pc_primitives", "pcen", "pcen_source",
           "periodic", "phi", "phi12", "phi_up", "pi", "pic3_rays",
           "picture_rays", "pivot", "plc", "pldefault", "pldj", "pledit",
           "plf", "plfc", "plfc_colors", "plfc_levs", "plfp", "plg", "pli",
           "plm", "plmesh", "plmk", "plmk_default", "plq", "plsys", "plt",
           "plt1", "pltitle", "pltitle_font", "pltitle_height", "plv",
           "pointer", "poly", "popen", "port", "power", "pr1", "prev",
           "primitives", "print", "print_format", "process_argv", "psum",
           "pt1", "pt2", "ptcen", "px", "q", "q_up", "qrt", "query", "quit",
           "qx", "qxy", "qy", "qz", "r", "radius", "random", "random_seed",
           "randomize", "range", "raw_collect", "raw_legal", "raw_not_cdf",
           "raw_read_n", "raw_show", "raw_style", "ray", "rays", "rcond",
           "rdline", "rdv", "re", "re_part", "read", "read_clog", "read_n",
           "recover_file", "redraw", "reg_track", "region", "remove",
           "rename", "require", "reset_options", "reshape", "restore",
           "result", "result__map", "return", "reverse", "rgb_read", "rjdir",
           "rjoff", "rmdir", "roll", "rphase", "rspace", "rt", "s", "save",
           "scalar", "sech", "seed", "selfem", "set_blocksize",
           "set_filesize", "set_idler", "set_path", "set_primitives",
           "set_site", "set_tolerances", "set_vars", "setup",
           "sgi64_primitives", "short", "show", "sign", "sin", "sinh",
           "sizeof", "slimits", "slims", "smooth", "snap", "snap_dt",
           "snap_i", "snap_result", "snap_worker", "solid", "sort", "source",
           "span", "spanl", "spann", "split", "sqrt", "sread", "start",
           "stds", "stds1", "stop", "streak", "streak_save", "streak_saver",
           "streak_times", "stride", "string", "strlen", "strmatch",
           "strpart", "strtok", "struct", "struct_align", "structof", "sum",
           "sun3_primitives", "sun_primitives", "swrite", "symbol_def",
           "symbol_set", "sys", "system", "t", "t0", "tail", "tail__map",
           "tan", "tanh", "text", "theta", "theta12", "theta_up", "time",
           "timer", "timer_print", "times", "timestamp", "title", "tmp", "tn",
           "top", "topic", "tops", "tosys", "track_integ", "track_rays",
           "track_reduce", "track_solve", "transp", "transpose", "triangle",
           "tsigma", "tt", "type", "typeof", "u", "ublk", "uncen", "uncp",
           "unit", "unzoom", "update", "update_mesh", "updateb",
           "use_origins", "v", "value", "vars", "vax_primitives",
           "vaxg_primitives", "vert", "viewport", "void", "volume", "vsq",
           "vt", "w", "w2", "warranty", "where", "where2", "which", "while",
           "width", "window", "winkill", "work", "write", "ws", "ww", "x",
           "x3ff", "x3ffe", "x4000", "x401", "x7f", "x81", "xc",
           "xdr_primitives", "xm", "xp", "xpict", "xtitle", "xy", "xytitles",
           "xyz", "y", "yPDBclose", "yPDBopen", "yc", "ym", "ymax", "ymin",
           "yorick_init", "yorick_stats", "ypict", "ytitle", "z", "zcen",
           "zmax", "zmin", "zncen", "zone", "zoom_factor", "zsym", "zt"];

  /* reserved keywords */
  i = 0;
  reserved = h_new("do",++i, "for",++i, "while",++i, "if",++i, "else",++i,
                   "goto",++i, "break",++i, "continue",++i, "func",++i,
                   "return",++i, "struct",++i, "extern",++i, "local",++i,
                   "more_args",++i, "next_arg",++i,
                   /* "char",++i, "short",++i, "int",++i, "long",++i,
                      "float",++i, "double",++i, "complex",++i, "string",++i,
                      "pointer",++i, */
                   "min",++i, "max",++i, "avg",++i, "rms",++i, "sum",++i,
                   "mnx",++i, "mxx",++i, "dif",++i, "pcen",++i, "psum",++i,
                   "zcen",++i, "cum",++i, "ptp",++i, "uncp",++i);

  /* tests */
  local result;
  n = numberof(names);
  values = swrite(format="%s_%d",  names, indgen(1:n));
  tab = h_new();
  for (i = 1; i <= n; ++i) {
      h_set, tab, names(i), values(i);
  }
  code = swrite(format="tab() == %d", n);
  include, ["result = ("+code+")"], 1;
  test_assert, result == 1n, code;

  /* Check h_keys(). */
  temp = h_keys(tab);
  test_assert, (numberof(temp) == numberof(names) &&
                allof(temp(sort(temp)) == names(sort(names)))),
      "h_keys(tab) yields all key names";

  /* Check values stored into hash table. */
  for (i = 1; i <= n; ++i) {
    key = names(i);
    val = values(i);
    code = swrite(format="h_get(tab, \"%s\") == \"%s\"", key, val);
    include, ["result = ("+code+")"], 1;
    test_assert, result == 1n, code;
  }
  for (i = 1; i <= n; ++i) {
    key = names(i);
    val = values(i);
    code = swrite(format="tab(\"%s\") == \"%s\"", key, val);
    include, ["result = ("+code+")"], 1;
    test_assert, result == 1n, code;
  }

  /* Check for tab.key and h_get(tab, key=) syntaxes for non-keyword keys. */
  for (i = 1; i <= n; ++i) {
    key = names(i);
    if (reserved(key)) continue;
    val = values(i);
    code = swrite(format="tab.%s == \"%s\"", key, val);
    include, ["result = ("+code+")"], 1;
    test_assert, result == 1n, code;
  }
  for (i = 1; i <= n; ++i) {
    key = names(i);
    if (reserved(key)) continue;
    val = values(i);
    code = swrite(format="h_get(tab, %s=) == \"%s\"", key, val);
    include, ["result = ("+code+")"], 1;
    test_assert, result == 1n, code;
  }

  /* Check custom evaluator. */
  h_evaluator, tab, "_h_test_eval1";
  for (i = 1; i <= n; ++i) {
    key = names(i);
    val = values(i);
    code = swrite(format="tab(\"%s\") == \"%s\"", key, val);
    include, ["result = ("+code+")"], 1;
    test_assert, result == 1n, code;
  }

  /* Speed test (can also be used to detect memory leaks). */
  write, "";
  timer_start;
  for (k = 1; k <= repeat; ++k) {
    tab = h_new();
    for (i = 1; i <= n; ++i) {
      h_set, tab, names(i), i;
    }
  }
  timer_elapsed, repeat;

  stat = h_stat(tab);
  h_analyse, stat;
  stat(1:max(where(stat)));
}

h_test;
