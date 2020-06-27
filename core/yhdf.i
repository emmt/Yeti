/*
 * yhdf.i --
 *
 * Implement support for Yeti Hierarchical Data File.
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

local yhd_format;
/* DOCUMENT         DESCRIPTION OF YHD FILE FORMAT

     A YHD file consists in a header (256 bytes) followed by any number of
     records (one record for each member of the saved hash_table object).

     The file header is a 256 character array filled with a text string
     padded with nulls:
         YetiHD-VERSION (DATE)\n
         ENCODING\n
         COMMENT\n
     where VERSION is the version number (an integer); DATE is the creation
     date of the file (see Yorick built-in timestamp); ENCODING is a
     human-readable array of 32 integers separated by commas and enclosed
     in square brackets (ie.: [n1,n2,....,n32]); COMMENT is an optional
     comment string.

     All binary data of a YHD file is written following the ENCODING format
     of the file.

     The format of a record for an array member is as follow:
     | Number Type  Name     Description
     | -----------------------------------------------------------------------
     |      1 long  TYPE     data type of record
     |      1 long  IDSIZE   number of bytes in member identifier (may be 0)
     |      1 long  RANK     number of dimensions, 0 if scalar/non-array
     |   RANK long  DIMLIST  dimension list (absent if RANK is 0)
     | IDSIZE char  IDENT    identifier of record (see below)
     |   *special*  DATA     binary data of the record

     TYPE is: <0 - string array      5 - float array      11 - range
     |         0 - void              6 - double array     12 - evaluator as function
     |         1 - char array        7 - complex array    13 - evaluator as symbol name
     |         2 - short array       8 - pointer array
     |         3 - int array         9 - function
     |         4 - long array       10 - symbolic link
     For string array, TYPE is strictly less than zero and is minus the
     number of characters needed to represent all elements of the array in
     packed form (more on this below).  Void objects are also used to
     represent NULL pointer data -- this means that a NULL pointer element
     takes 3*sizeof(long) bytes to be stored in the file, which may be an
     issue if you use large pointer array sparsely filled with data.

     IDENT is the full name of the member: it is a IDSIZE char array
     where null characters are used to separate submember names and with
     a final null.  If IDSIZE=0, no IDENT is written.

     Arrays of strings are written in packed form, each string being
     prefixed with '\1' (nil string) or '\2' (non-nil string) and suffixed
     with '\0', hence:
     |  '\1' '\0'       (2 bytes) for a nil string
     |  '\2' ... '\0'   (2+LEN bytes) for a string of length LEN
     this is needed to distinguish nil-string from "".

     The data part of an arrays of pointers consists in anonymous records
     (records with IDSIZE=0 and no IDENT) for each element of the array.

     Non-array members such as functions and symbolic links have the
     following record:
     | Number Type  Name     Description
     | -----------------------------------------------------------------------
     |      1 long  TYPE     data type of record (9 or 10)
     |      1 long  IDSIZE   number of bytes in member identifier (may be 0)
     |      1 long  LENGTH   number of bytes for the name of the function
     | IDSIZE char  IDENT    identifier of record (see above)
     | LENGTH char  NAME     name of function or symbolic link
     Note that the final '\0' of the name is not saved in the file.

     Range members have the following record:
     | Number Type  Name     Description
     | -----------------------------------------------------------------------
     |      1 long  TYPE     data type of record (11)
     |      1 long  IDSIZE   number of bytes in member identifier (may be 0)
     |      1 long  FLAGS    flags of range
     | IDSIZE char  IDENT    identifier of record (see above)
     |      3 long  RANGE    MIN,MAX,STEP

     Evaluators have the following record:
     | Number Type  Name     Description
     | -----------------------------------------------------------------------
     |      1 long  TYPE     data type of record (12 or 13)
     |      1 long  IDSIZE   number of bytes in member identifier (may be 0)
     |      1 long  LENGTH   number of bytes for the name of the evaluator
     | IDSIZE char  IDENT    identifier of record (see above)
     | LENGTH char  NAME     name of function or symbolic link
     Note that the final '\0' of the name is not saved in the file.  The last
     component of identifier (the member name) must be empty for an evaluator.
 */

func yhd_save(filename, obj, keylist, .., comment=, encoding=, overwrite=)
/* DOCUMENT yhd_save, filename, obj;
       -or- yhd_save, filename, obj, keylist, ...;
     Save contents of hash object OBJ into the Yeti Hierarchical Data (YHD)
     file FILENAME.  If additional arguments are provided, they are the
     names of members to save.  The default is to save every member.

     Keyword COMMENT can be used to store a (short) string comment in the
     file header.  The comment is truncated if it is too long (more than
     about 130 bytes) to fit into the header.  COMMENT must not contain
     any DEL (octal 177) character.

     Keyword ENCODING can be used to specify a particular binary data
     format for the file; ENCODING can be the name of some known data
     format (see get_encoding) or an array of 32 integers (see
     set_primitives).  The default is to use the native data format.

     If keyword OVERWRITE is true and file FILENAME already exists, the new
     file will (silently) overwrite the old one; othwerwise, file FILENAME
     must not already exist (default behaviour).


   SEE ALSO yhd_restore, yhd_info, yhd_check, yhd_format,
            get_encoding, set_primitives, h_new. */
{
  /* Declaration of variables that will be inherited by subroutines called
     by this routine (not really necessary, but just to make this clear). */
  local file, address, elsize;

  /* Set some 'constants'. */
  YHD_HEADER_SIZE = 256;
  YHD_VERSION = 2; // version number

  /* Get list of members to save. */
  if (! is_hash(obj)) error, "expecting hash_table object";
  while (more_args()) grow, keylist, next_arg();
  if (! is_void(keylist) && structof(keylist) != string)
    error, "invalid member list";

  /* Check COMMENT string. */
  if (is_void(comment)) comment = "";
  else if (strmatch(comment, "\177")) error, "invalid character in COMMENT";

  /* Create binary file with correct primitives and avoid log-file. */
  if (! overwrite && open(filename,"r",1))
    error, "file \"" + filename + "\" already exists";
  logname = filename + "L";
  remove_log = (open(logname, "r", 1) ? 0n : 1n);
  file = open(filename, "wb");
  if (remove_log) remove, logname;
  if (is_void(encoding)) encoding = "native";
  if (structof(encoding) == string) encoding = get_encoding(encoding);
  install_encoding, file, encoding;
  save, file, complex; /* install the definition of a complex */

  /* Write header. */
  ident = swrite(format="YetiHD-%d (%s)\n[%d",
                 YHD_VERSION, timestamp(), encoding(1));
  for (i = 2; i <= 32; ++i) ident += swrite(format=",%d", encoding(i));
  maxlen = YHD_HEADER_SIZE - 3 - strlen(ident);
  if (strlen(comment) > maxlen) {
    __yhd_warn, "too long comment get truncated";
    comment = strpart(comment, 1:maxlen);
  }
  ident += swrite(format="]\n%s\n", comment);
  len = strlen(ident);
  (hdr = array(char, YHD_HEADER_SIZE))(1:len) = (*pointer(ident))(1:len);
  address = 0;
  _write, file, address, hdr;
  address += YHD_HEADER_SIZE;

  /* Build table of size of primary data types in file encoding. */
  elsize = [encoding(1), encoding(4), encoding(7), encoding(10),
            encoding(13), encoding(16), 2*encoding(16)];

  /* Save members. */
  __yhd_save_hash, obj, [], keylist;
}

func __yhd_save_member(data, ident)
{
  /**/extern file, address, elsize, long_size;
  idsize = numberof(ident);
  if (is_array(data)) {
    if ((s = structof(data)) == char) {
      type = 1;
    } else if (s == short) {
      type = 2;
    } else if (s == int) {
      type = 3;
    } else if (s == long) {
      type = 4;
    } else if (s == float) {
      type = 5;
    } else if (s == double) {
      type = 6;
    } else if (s == complex) {
      type = 7;
    } else if (s == pointer) {
      type = 8;
    } else if (s == string) {
      /* For string arrays, TYPE is minus the number of character written
         to the file. */
      type = -(2*numberof(data) + sum(strlen(data)));
    } else {
      /* Unsupported array type. */
      type = 0;
    }
    if (type) {
      /* Write array record header. */
      dimlist = dimsof(data);
      number = numberof(data);
      header = array(long, 2 + numberof(dimlist));
      header(1) = type;
      header(2) = idsize;
      header(3:) = dimlist;
      _write, file, address, header;
      address += long_size*numberof(header);
      if (idsize) {
        _write, file, address, ident;
        address += idsize;
      }

      /* Write data array. */
      if (type < 0) {
        /* string array */
        nil = ['\1', '\0'];
        for (i = 1; i <= number; ++i) {
          c = *pointer(data(i));
          n =  numberof(c);
          if (n) {
            _write, file, address++, '\2';
            _write, file, address, c;
            address += n;
          } else {
            _write, file, address, nil;
            address += 2;
          }
        }
      } else if (type == 8) {
        /* pointer data array */
        for (i = 1; i <= number; ++i) {
          __yhd_save_member, *data(i); /* no ident */
        }
      } else {
        /* Numerical data array. */
        _write, file, address, data;
        address += elsize(type)*number;
      }
      return; /* end for supported array types */
    }
  }

  /* Non-array types. */
  if (is_hash(data)) {
    __yhd_save_hash, data, ident;
  } else if (is_func(data)) {
    __yhd_save_named, ident, 9, nameof(data);
  } else if (is_symlink(data)) {
    __yhd_save_named, ident, 10, name_of_symlink(data);
  } else if (is_range(data)) {
    temp = parse_range(data);
    header = array(long, 3);
    header(1) = 11; /* type */
    header(2) = idsize;
    header(3) = temp(1); /* flags */
    _write, file, address, header;
    address += long_size*numberof(header);
    if (idsize) {
      _write, file, address, ident;
      address += idsize;
    }
    _write, file, address, temp(2:4);
    address += long_size*3;
  } else {
    /* Void or unsupported data type. */
    if (! is_void(data)) {
      if (idsize) {
        /* hash table member */
        __yhd_warn, "unsupported data type: ", typeof(data),
          " for member \"", __yhd_member_name(ident),
          "\" - replaced by void data";
      } else {
        /* element of a pointer array: save NULL pointer */
        __yhd_warn, "unsupported data type: ", typeof(data),
          " - replaced by NULL pointer element";
      }
    }
    _write, file, address, long([0, idsize, 0]); /* type, idsize, rank */
    address += 3*long_size;
    if (idsize) {
      _write, file, address, ident;
      address += idsize;
    }
  }
}

func __yhd_save_hash(hash, prefix, keylist)
{
  /* Declaration and initialization of shared variables. */
  /**/extern address, file, elsize;
  long_size = elsize(4); /* sizeof(long) in file encoding */

  /* Deal with the evaluator if any. */
  evl = h_evaluator(hash);
  if (evl) {
    ident = grow(prefix, '\0'); /* the evaluator has an empty member name */
    if (is_func(evl)) {
      /* The evaluator is given by a function object (FIXME: check this). */
      __yhd_save_named, ident, 12, nameof(evl);
    } else {
      /* The evaluator is given by its name. */
      __yhd_save_named, ident, 13, evl;
    }
  }

  /* Recursively save all/some chidren of current member. */
  if (is_void(keylist)) {
    keylist = h_keys(hash);
    if (is_array(keylist)) keylist = keylist(sort(keylist));
  }
  n = numberof(keylist);
  for (i = 1; i <= n; ++i) {
    key = keylist(i);
    ident = grow(prefix, strchar(key));
    __yhd_save_member, h_get(hash, key), ident;
  }
}

func __yhd_save_named(ident, type, name)
{
  /**/extern address, file, long_size;
  idsize = numberof(ident);
  length = strlen(name);
  header = array(long, 3);
  header(1) = type;
  header(2) = idsize;
  header(3) = length;
  _write, file, address, header;
  address += long_size*numberof(header);
  if (idsize) {
    _write, file, address, ident;
    address += idsize;
  }
  if (length) {
    _write, file, address, strchar(name)(1:length);
    address += length;
  }
}

func yhd_check(file, &version, &date, &encoding, &comment)
/* DOCUMENT yhd_check(file);
       -or- yhd_check(file, version, date, encoding, comment);
     Return 1 (true) if FILE is a valid YHD file; otherwise return 0
     (false).  The nature of FILE is guessed by reading its header.  Input
     argument FILE can be a file name (scalar string) of a binary file
     stream opened for reading; all other arguments are pure outputs and
     may be omitted (if result is false, the contents of these outputs is
     undetermined).

   SEE ALSO yhd_info, yhd_save, yhd_restore, yhd_format. */
{
  /* Read header array. */
  YHD_HEADER_SIZE = 256;
  if (structof(file) == string) file = open(file, "rb");
  hdr = array(char, YHD_HEADER_SIZE);
  if (_read(file, 0, hdr) != YHD_HEADER_SIZE) return 0n;
  hdrstr = string(&hdr);

  /* Parse header string (hopefully the DEL character (octal 177) is
     not part of the comment string). */
  comment = date = token = string();
  encoding = array(long, 32);
  version = 0;
  if (sread(hdrstr, format="YetiHD-%d (%[^)])\n[%[^]]]\n%[^\177]",
            version, date, token, comment) >= 3) {
    /* Parse encoding array. */
    value = 0;
    for (i = 1; i < 32; ++i) {
      if (sread(token, format="%d ,%[^]]", value, token) != 2) break;
      encoding(i) = value;
    }
    if (i==32 && sread(token, format="%d %[^]]", value, token) == 1) {
      /* Finalize encoding array and comment string. */
      encoding(i) = value;
      if (strpart(comment, 0:0) == "\n") comment = strpart(comment, :-1);
      return 1n;
    }
  }
  return 0n;
}

func yhd_info(file)
/* DOCUMENT yhd_info, file;
     Print out some information about YHD file.  FILE can be a file
     name (scalar string) of a binary file stream opened for reading.

   SEE ALSO yhd_check, yhd_restore, yhd_save, yhd_format. */
{
  local version, date, encoding, comment;
  if (! yhd_check(file, version, date, encoding, comment)) {
    error, (structof(file) == string ? "\""+file+"\" is not a valid YHD file"
            : "invalid YHD file");
  }
  write, format="%s:\n  version = %d\n  date    = %s\n  comment = %s\n",
    (structof(file) == string ? file : "YHD file"), version, date, comment;
}

func yhd_restore(filename, keylist, ..)
/* DOCUMENT yhd_restore(filename);
       -or- yhd_restore(filename, keylist, ...);
     Restore and return hash table object saved in YHD file FILENAME.  If
     additional arguments are provided, they are the names of members to
     restore.  The default is to restore every member.

   SEE ALSO yhd_check, yhd_info, yhd_save, yhd_format. */
{
  /* Declaration of variables that will be inherited by subroutines called
     by this routine (not really necessary, but just to make this clear). */
  local file, address, elsize, type, dimlist, ident;

  /* List of members to restore. */
  while (more_args()) grow, keylist, next_arg();
  if (! is_void(keylist) && structof(keylist) != string)
    error, "invalid member list";

  /* Open file, read header and set primitives. */
  local version, date, encoding, comment;
  file = open(filename, "rb");
  if (! yhd_check(file, version, date, encoding, comment))
    error, "\""+filename+"\" is not a valid YHD file";
  if (version != 1 && version != 2) {
    error, swrite(format="unsupported YHD file version: %d", version);
  }
  install_encoding, file, encoding;
  save, file, complex; /* install the definition of a complex */
  address = 256; /* header has already been read */

  /* Build table of size of primary data types in file encoding. */
  elsize = [encoding(1), encoding(4), encoding(7), encoding(10),
            encoding(13), encoding(16), 2*encoding(16)];

  /* Read contents of file. */
  obj = h_new();
  for (;;) {
    /* Read header of next member. */
    if (! __yhd_read_member_header(ident, type, dimlist))
      return obj; /* normal end-of-file */

    /* Skip member if first "path" component not in KEYLIST. */
    path = strchar(ident);
    key = path(1);
    if (! is_void(keylist) && noneof(keylist == key)) {
      __yhd_restore_data, type, dimlist, 1;
      continue;
    }

    /* Convert identifier to OWNER-KEY pair. */
    owner = obj;
    n = numberof(path);
    for (j = 2; j <= n; ++j) {
      if (! key) key = ""; /* deal with NULL strings returned by strchar() */
      if (h_has(owner, key)) {
        owner = h_get(owner, key);
        if (! is_hash(owner)) {
          error, "inconsistent hierarchical member \""+
            __yhd_member_name(path)+"\"";
        }
      } else {
        h_set, owner, key, (tmp = h_new());
        owner = tmp;
      }
      key = path(j);
    }
    if (type == 12 || type ==13) {
      /* Evaluators must have empty member name. */
      if (key) error, ("expecting empty member name for an evaluator, got \""+
                       key + "\" in member \"" + __yhd_member_name(path) + "\"");
      if (h_evaluator(owner)) __yhd_warn, "duplicate evaluator \"",
                               __yhd_member_name(path),
                               "\" in YHD file (last setting overrides previous ones)";
      h_evaluator, owner, __yhd_restore_data(type, dimlist);
    } else {
      /* For normal members, deal with NULL strings returned by strchar() and
         check that the member does not yet exist before restoring it. */
      if (! key) key = "";
      if (h_has(owner, key)) __yhd_warn, "duplicate member \"",
                               __yhd_member_name(path),
                               "\" in YHD file (last value overrides previous ones)";
      h_set, owner, key, __yhd_restore_data(type, dimlist);
    }
  }
  return obj
}

func __yhd_read_member_header(&ident, &type, &dimlist, pt)
{
  /**/extern file, address, elsize;

  /* Figure out if there is anything else to read. */
  tmp = 'a';
  if (! _read(file, address, tmp)) return 0; /* normal end-of-file */

  /* Read record header */
  long_size = elsize(4); /* sizeof(long) in file encoding */
  char_size = elsize(1); /* sizeof(char) in file encoding */
  tmp = __yhd_read(long_size, long, 3);
  type = tmp(1);
  idsize = tmp(2);
  rank = tmp(3);
  if (type != 0) {
    if (rank < 0) error, "bad RANK in record header of YHD file";
    if (type <= 8) {
      dimlist = array(rank, rank+1);
      if (rank) dimlist(2:) = __yhd_read(long_size, long, rank);
    } else {
      dimlist = rank; /* special */
    }
  } else {
    /* Void object. */
    if (rank != 0) error, "bad RANK in record header of YHD file";
    dimlist = [];
  }
  if (pt) {
    /* Element of pointer array: IDSIZE must be 0. */
    if (idsize) error, "unexpected named member in YHD file";
  } else {
    if (idsize < 1) error, "unexpected anonymous member in YHD file";
    ident = __yhd_read(char_size, char, idsize);
  }
  return 1;
}

func __yhd_restore_data(type, dimlist, skip)
{
  /**/extern file, address, elsize;

  if (type >= 1 && type <= 7) {
    /* Numerical array data. */
    size = elsize(type);
    if (skip) {
      n = numberof(dimlist);
      for (i = 2; i <= n; ++i) size *= dimlist(i);
      address += size;
      return;
    }
    return __yhd_read(size, (type == 1 ? char :
                             (type == 2 ? short :
                              (type == 3 ? int :
                               (type == 4 ? long :
                                (type == 5 ? float :
                                 (type == 6 ? double : complex)))))), dimlist);
  }

  if (type < 0) {
    /* Restore string array.  For string arrays, TYPE is minus the number
       of character written to the file. */
    char_size = elsize(1); /* sizeof(char) in file encoding */
    count = -type;
    if (skip) {
      address += char_size*count;
      return;
    }
    c = __yhd_read(char_size, char, count);
    j = where(! c);
    data = array(string, dimlist);
    number = numberof(data);
    if (numberof(j) != number) {
      __yhd_warn, "bad string array in file (elements left empty)";
    } else {
      k1 = 1;
      for (i = 1; i <= number; ++i) {
        k2 = j(i);
        if (c(k1) == '\2') data(i) = string(&c(k1+1:k2));
        k1 = k2 + 1;
      }
    }
    return data;
  }

  if (type == 8) {
    /* Pointer array. */
    local etype, edims; /* type and dimension list for every element */
    ptr = array(pointer, dimlist);
    number = numberof(ptr);
    for (i = 1; i <= number; ++i) {
      if (! __yhd_read_member_header(/*not needed*/, etype, edims, 1)) {
        __yhd_warn, "short YHD file (unterminated pointer array)";
        break;
      }
      tmp = __yhd_restore_data(etype, edims, skip);
      if (! skip && etype) ptr(i) = &tmp;
    }
    if (skip) return;
    return ptr;
  }

  if (type == 9 || type == 10) {
    /* Restore function. */
    length = dimlist; /* special */
    if (skip) {
      address += length;
      return;
    }
    if (length <= 0) {
      __yhd_warn, "unexpected lenght for function or symbolic link name";
      return;
    }
    cbuf = array(char, length);
    if (_read(file, address, cbuf) != length) {
      error, "short YHD file (unterminated function or symbolic link)";
    }
    name = strchar(cbuf);
    if (strlen(name) != length) {
      error, "invalid name in function or symbolic link member";
    }
    address += length;
    if (type == 9) {
      /* function member */
      if (symbol_exists(name)) {
        value = symbol_def(name);
        if (is_func(value)) {
          return value;
        }
      }
      __yhd_warn, ("function \"" + name +
                   "\" not defined (will be replaced by a symbolic link");
    }
    /* symbolic link */
    return symlink_to_name(name);
  }

  if (type == 11) {
    /* Restore range. */
    long_size = elsize(4); /* sizeof(long) in file encoding */
    if (skip) {
      address += 3*long_size;
      return;
    }
    temp = array(long, 4);
    temp(1) = dimlist; /* special: "flags" in this context  */
    temp(2:4) = __yhd_read(long_size, long, 3);
    return make_range(temp);
  }

  if (type == 12 || type == 13) {
    /* Restore evaluator. */
    length = dimlist; /* special */
    if (skip) {
      address += length;
      return;
    }
    if (length <= 0) {
      __yhd_warn, "unexpected lenght for evaluator name";
      return;
    }
    cbuf = array(char, length);
    if (_read(file, address, cbuf) != length) {
      error, "short YHD file (unterminated evaluator name)";
    }
    name = strchar(cbuf);
    if (strlen(name) != length) {
      error, "invalid evaluator name";
    }
    address += length;
    if (type == 12) {
      /* evaluator specified as a function */
      if (symbol_exists(name)) {
        value = symbol_def(name);
        if (is_func(value)) {
          return value;
        }
      }
      __yhd_warn, ("evaluator function \"" + name +
                   "\" not defined (will be replaced by a symbolic link");
    }
    return name;
  }

  if (type) {
    error, "invalid TYPE in record header of YHD file";
  }
}

func __yhd_read(element_size, data_type, dimlist)
{
  /**/extern file, address;
  data = array(data_type, dimlist);
  nbytes = element_size*numberof(data);
  if (data_type != char) _read, file, address, data;
  else if (_read(file, address, data) != nbytes) error, "short file";
  address += nbytes;
  return data;
}

func __yhd_warn(s, ..)
{
  while (more_args()) s += next_arg();
  write, format="WARNING - %s\n", s;
}

func __yhd_member_name(ident, separator)
{
  if (structof(ident) == char) {
    ident = strchar(ident);
  }
  name = ident(1);
  if ((n = numberof(i)) >= 2) {
    if (is_void(separator)) separator = ".";
    for (j = 2; j <= n; ++j) {
      name += separator + ident(j);
    }
  }
  return name;
}

func __yhd_insert_dim(dimlist, first_dim)
{
  newlist = 0;
  grow, newlist, dimlist;
  newlist(1) = numberof(newlist)-1;
  newlist(2) = first_dim;
  return newlist;
}
