/*
 * ytiff.c -
 *
 * Implement support for TIFF images in Yorick.
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "yapi.h"
#include "pstdlib.h"
#include "yio.h"

/* Expand_name returns NAME after expansion. */
static char* expand_name(const char *name);

/* Push an array of a given type. */
static void* push_array(int typeid, long *dims);

/* Push a scalar string. */
static void push_string(const char *value);

static char* expand_name(const char *name)
{
  return YExpandName(name);
}

static void* push_array(int typeid, long *dims)
{
  switch (typeid) {
  case Y_CHAR:    return (void*)ypush_c(dims);
  case Y_SHORT:   return (void*)ypush_s(dims);
  case Y_INT:     return (void*)ypush_i(dims);
  case Y_LONG:    return (void*)ypush_l(dims);
  case Y_FLOAT:   return (void*)ypush_f(dims);
  case Y_DOUBLE:  return (void*)ypush_d(dims);
  case Y_COMPLEX: return (void*)ypush_z(dims);
  case Y_STRING:  return (void*)ypush_q(dims);
  case Y_POINTER: return (void*)ypush_p(dims);
  }
  y_error("(BUG) non-array type number");
  return 0;
}

static void push_string(const char *value)
{
  ypush_q(NULL)[0] = (value ? p_strcpy((char *)value) : NULL);
}

#if 0 /* hack for old Yorick versions */
/* better if there exists an yget_any (not specially array) */
static int yarg_true(int iarg);
static int yarg_true(int iarg)
{
  int typeid;
  long dims[Y_DIMSIZE];
  void *ptr;

  typeid = yarg_typeid(iarg);
  if (typeid >= Y_CHAR && typeid <= Y_POINTER) {
    ptr = ygeta_any(iarg, NULL, dims, NULL);
    if (! dims[0]) {
      switch (typeid) {
      case Y_CHAR:    return (*(char   *)ptr != 0);
      case Y_SHORT:   return (*(short  *)ptr != 0);
      case Y_INT:     return (*(int    *)ptr != 0);
      case Y_LONG:    return (*(long   *)ptr != 0L);
      case Y_FLOAT:   return (*(float  *)ptr != 0.0F);
      case Y_DOUBLE:  return (*(double *)ptr != 0.0);
      case Y_COMPLEX: return (((double *)ptr)[0] != 0.0 ||
                              ((double *)ptr)[1] != 0.0);
      case Y_STRING:  return (*(ystring_t *)ptr != NULL);
      case Y_POINTER: return (*(ypointer_t *)ptr != NULL);
      }
    }
  } else if (typeid == Y_VOID) {
    return 0;
  } else {
    return 1;
  }
  y_error("bad non-boolean argument");
  return 0; /* avoid compiler warning */
}
#endif /* old yorick */

/*---------------------------------------------------------------------------*/
/* PUBLIC ROUTINES */
extern ybuiltin_t Y_tiff_open;
extern ybuiltin_t Y_tiff_read_directory;
extern ybuiltin_t Y_tiff_read_image;
extern ybuiltin_t Y_tiff_read_pixels;
extern ybuiltin_t Y_tiff_debug;

/*---------------------------------------------------------------------------*/
#ifndef HAVE_TIFF
# define HAVE_TIFF 0
#endif
#if HAVE_TIFF
#include <tiff.h>
#include <tiffio.h>

/*---------------------------------------------------------------------------*/
/* DATA TYPES */
typedef struct _tag    tag_t;
typedef struct _object object_t;

/* PRIVATE DATA */
static char message[2048];
static int debug = 0;

/* PRIVATE ROUTINES */
static void *push_workspace(long nbytes);
static void  load_pixels(TIFF *tiff);
static void  error_handler(const char* module, const char* fmt, va_list ap);
static void  warning_handler(const char* module, const char* fmt, va_list ap);
static int cmapbits(unsigned int n,
                    const uint16 r[],
                    const uint16 g[],
                    const uint16 b[]);
static void missing_required_tag(const char *tagname);

/*---------------------------------------------------------------------------*/
/* OPAQUE OBJECTS */

static void on_free(void*);
static void on_print(void*);
static void on_eval(void*, int);
static void on_extract(void*, char*);
static object_t *get_object(int iarg);

static y_userobj_t tiff_type = {
  "TIFF file handle", on_free, on_print, on_eval, on_extract, NULL
};

struct _object {
  TIFF* handle; /* TIFF file handle */
  char* path;   /* full path */
  char* mode;   /* mode when TIFFOpen was called */
};

/*
 * The tags understood by libtiff,  the number of parameter values, and the
 * expected types for the parameter values are shown below.  The data types
 * are: char* is  null-terminated string and corresponds to  the ASCII data
 * type; uint16 is  an unsigned 16-bit value; uint32  is an unsigned 32-bit
 * value; uint16* is an array of unsigned 16-bit values.  void* is an array
 * of data values of unspecified type.
 *
 * Consult the  TIFF specification for  information on the meaning  of each
 * tag.
 *
 * Tag Name                        Count  Types              Notes
 * TIFFTAG_ARTIST                  1      char*
 * TIFFTAG_BADFAXLINES             1      uint32
 * TIFFTAG_BITSPERSAMPLE           1      uint16             -
 * TIFFTAG_CLEANFAXDATA            1      uint16
 * TIFFTAG_COLORMAP                3      uint16*            1<<BitsPerSample arrays
 * TIFFTAG_COMPRESSION             1      uint16             -
 * TIFFTAG_CONSECUTIVEBADFAXLINES  1      uint32
 * TIFFTAG_COPYRIGHT               1      char*
 * TIFFTAG_DATETIME                1      char*
 * TIFFTAG_DOCUMENTNAME            1      char*
 * TIFFTAG_DOTRANGE                2      uint16
 * TIFFTAG_EXTRASAMPLES            2      uint16,uint16*     - count & types array
 * TIFFTAG_FAXMODE                 1      int                - G3/G4 compression pseudo-tag
 * TIFFTAG_FAXFILLFUNC             1      TIFFFaxFillFunc    G3/G4 compression pseudo-tag
 * TIFFTAG_FILLORDER               1      uint16             -
 * TIFFTAG_GROUP3OPTIONS           1      uint32             -
 * TIFFTAG_GROUP4OPTIONS           1      uint32             -
 * TIFFTAG_HALFTONEHINTS           2      uint16
 * TIFFTAG_HOSTCOMPUTER            1      char*
 * TIFFTAG_IMAGEDESCRIPTION        1      char*
 * TIFFTAG_IMAGEDEPTH              1      uint32             -
 * TIFFTAG_IMAGELENGTH             1      uint32
 * TIFFTAG_IMAGEWIDTH              1      uint32             -
 * TIFFTAG_INKNAMES                1      char*
 * TIFFTAG_INKSET                  1      uint16             -
 * TIFFTAG_JPEGTABLES              2      uint32*,void*      - count & tables
 * TIFFTAG_JPEGQUALITY             1      int                JPEG pseudo-tag
 * TIFFTAG_JPEGCOLORMODE           1      int                - JPEG pseudo-tag
 * TIFFTAG_JPEGTABLESMODE          1      int                - JPEG pseudo-tag
 * TIFFTAG_MAKE                    1      char*
 * TIFFTAG_MATTEING                1      uint16             -
 * TIFFTAG_MAXSAMPLEVALUE          1      uint16
 * TIFFTAG_MINSAMPLEVALUE          1      uint16
 * TIFFTAG_MODEL                   1      char*
 * TIFFTAG_ORIENTATION             1      uint16
 * TIFFTAG_PAGENAME                1      char*
 * TIFFTAG_PAGENUMBER              2      uint16
 * TIFFTAG_PHOTOMETRIC             1      uint16
 * TIFFTAG_PLANARCONFIG            1      uint16             -
 * TIFFTAG_PREDICTOR               1      uint16             -
 * TIFFTAG_PRIMARYCHROMATICITIES   1      float*             6-entry array
 * TIFFTAG_REFERENCEBLACKWHITE     1      float*             - 2*SamplesPerPixel array
 * TIFFTAG_RESOLUTIONUNIT          1      uint16
 * TIFFTAG_ROWSPERSTRIP            1      uint32             - must be > 0
 * TIFFTAG_SAMPLEFORMAT            1      uint16             -
 * TIFFTAG_SAMPLESPERPIXEL         1      uint16             - value must be <= 4
 * TIFFTAG_SMAXSAMPLEVALUE         1      double
 * TIFFTAG_SMINSAMPLEVALUE         1      double
 * TIFFTAG_SOFTWARE                1      char*
 * TIFFTAG_STONITS                 1      double             -
 * TIFFTAG_SUBFILETYPE             1      uint32
 * TIFFTAG_SUBIFD                  2      uint16,uint32*     count & offsets array
 * TIFFTAG_TARGETPRINTER           1      char*
 * TIFFTAG_THRESHHOLDING           1      uint16
 * TIFFTAG_TILEDEPTH               1      uint32             -
 * TIFFTAG_TILELENGTH              1      uint32             - must be a multiple of 8
 * TIFFTAG_TILEWIDTH               1      uint32             - must be a multiple of 8
 * TIFFTAG_TRANSFERFUNCTION        1 or 3 = uint16*1<<BitsPerSample entry arrays
 * TIFFTAG_XPOSITION               1      float
 * TIFFTAG_XRESOLUTION             1      float
 * TIFFTAG_WHITEPOINT              1      float*             2-entry array
 * TIFFTAG_YCBCRCOEFFICIENTS       1      float*             - 3-entry array
 * TIFFTAG_YCBCRPOSITIONING        1      uint16             -
 * TIFFTAG_YCBCRSAMPLING           2      uint16             -
 * TIFFTAG_YPOSITION               1      float
 * TIFFTAG_YRESOLUTION             1      float
 * TIFFTAG_ICCPROFILE              2      uint32,void*       count, profile data*
 * - Tag may not have its values changed once data is written.
 * =  If  SamplesPerPixel is one, then a single array is passed; otherwise
 * three arrays should be passed.
 * * The contents of this field are quite complex.  See  The  ICC  Profile
 * Format  Specification, Annex B.3 "Embedding ICC Profiles in TIFF Files"
 * (available at http://www.color.org) for an explanation.
 */

struct _tag {
  void (*push)(TIFF *tiff, int tag);
  char *name;
  int   tag;
  long  index; /* in globTab */
};

#define PUSH_TAG(name, type_t, pusher)				\
static void name(TIFF *tiff, int tag)				\
{								\
  type_t value;							\
  if (TIFFGetFieldDefaulted(tiff, tag, &value)) pusher(value);	\
  else ypush_nil();						\
}
PUSH_TAG(push_tag_string, char *, push_string)
PUSH_TAG(push_tag_uint16, uint16, ypush_long)
PUSH_TAG(push_tag_uint32, uint32, ypush_long)
PUSH_TAG(push_tag_int,    int,    ypush_long)
PUSH_TAG(push_tag_float,  float,  ypush_double)
PUSH_TAG(push_tag_double, double, ypush_double)
#undef PUSH_TAG

static void push_tag_colormap(TIFF *tiff, int tag)
{
  uint16 *r, *g, *b, bitsPerSample;
  unsigned int i, number;
  unsigned char* cmap;
  long dims[Y_DIMSIZE];

  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_COLORMAP, &r, &g, &b)) {
    ypush_nil();
  } else if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE,
                                     &bitsPerSample)) {
    y_error("cannot get TIFF bits per sample");
  } else {
    number = (1 << ((unsigned int)bitsPerSample));
    dims[0] = 2;
    dims[1] = number;
    dims[2] = 3;
    cmap = (unsigned char*)ypush_c(dims);
    if (cmapbits(number, r, g, b) == 8) {
      for (i = 0; i < number; ++i) cmap[i] = r[i];
      cmap += number;
      for (i = 0; i < number; ++i) cmap[i] = g[i];
      cmap += number;
      for (i = 0; i < number; ++i) cmap[i] = b[i];
    } else {
      for (i = 0; i < number; ++i) cmap[i] = r[i]>>8;
      cmap += number;
      for (i = 0; i < number; ++i) cmap[i] = g[i]>>8;
      cmap += number;
      for (i = 0; i < number; ++i) cmap[i] = b[i]>>8;
    }
  }
}

#define TAG_UINT16(name, tag)   {push_tag_uint16,   name, tag, -1}
#define TAG_UINT32(name, tag)   {push_tag_uint32,   name, tag, -1}
#define TAG_INT(name, tag)      {push_tag_int,      name, tag, -1}
#define TAG_FLOAT(name, tag)    {push_tag_float,    name, tag, -1}
#define TAG_DOUBLE(name, tag)   {push_tag_double,   name, tag, -1}
#define TAG_STRING(name, tag)   {push_tag_string,   name, tag, -1}
#define TAG_COLORMAP(name, tag) {push_tag_colormap, name, tag, -1}

static long filename_index = -1L;
static long filemode_index = -1L;
static tag_t tag_table[] = {
  TAG_STRING(                 "artist", TIFFTAG_ARTIST),
  TAG_UINT16(          "bitspersample", TIFFTAG_BITSPERSAMPLE),
  TAG_UINT16(           "cleanfaxdata", TIFFTAG_CLEANFAXDATA),
  TAG_COLORMAP(             "colormap", TIFFTAG_COLORMAP),
  TAG_UINT16(            "compression", TIFFTAG_COMPRESSION),
  TAG_UINT32( "consecutivebadfaxlines", TIFFTAG_CONSECUTIVEBADFAXLINES),
  TAG_STRING(              "copyright", TIFFTAG_COPYRIGHT),
  TAG_UINT16(               "datatype", TIFFTAG_DATATYPE),
  TAG_STRING(               "datetime", TIFFTAG_DATETIME),
  TAG_STRING(           "documentname", TIFFTAG_DOCUMENTNAME),
  TAG_INT(                   "faxmode", TIFFTAG_FAXMODE),
  TAG_UINT16(              "fillorder", TIFFTAG_FILLORDER),
  TAG_UINT32(          "group3options", TIFFTAG_GROUP3OPTIONS),
  TAG_UINT32(          "group4options", TIFFTAG_GROUP4OPTIONS),
  TAG_STRING(           "hostcomputer", TIFFTAG_HOSTCOMPUTER),
  TAG_UINT32(             "imagedepth", TIFFTAG_IMAGEDEPTH),
  TAG_STRING(       "imagedescription", TIFFTAG_IMAGEDESCRIPTION),
  TAG_UINT32(            "imagelength", TIFFTAG_IMAGELENGTH),
  TAG_UINT32(             "imagewidth", TIFFTAG_IMAGEWIDTH),
  TAG_STRING(               "inknames", TIFFTAG_INKNAMES),
  TAG_UINT16(                 "inkset", TIFFTAG_INKSET),
  TAG_INT(               "jpegquality", TIFFTAG_JPEGQUALITY),
  TAG_INT(             "jpegcolormode", TIFFTAG_JPEGCOLORMODE),
  TAG_INT(            "jpegtablesmode", TIFFTAG_JPEGTABLESMODE),
  TAG_STRING(                   "make", TIFFTAG_MAKE),
  TAG_UINT16(               "matteing", TIFFTAG_MATTEING),
  TAG_UINT16(         "maxsamplevalue", TIFFTAG_MAXSAMPLEVALUE),
  TAG_UINT16(         "minsamplevalue", TIFFTAG_MINSAMPLEVALUE),
  TAG_STRING(                  "model", TIFFTAG_MODEL),
  TAG_UINT16(            "orientation", TIFFTAG_ORIENTATION),
  TAG_STRING(               "pagename", TIFFTAG_PAGENAME),
/*TAG_UINT16(             "pagenumber", TIFFTAG_PAGENUMBER),*/
  TAG_UINT16(            "photometric", TIFFTAG_PHOTOMETRIC),
  TAG_UINT16(           "planarconfig", TIFFTAG_PLANARCONFIG),
  TAG_UINT16(              "predictor", TIFFTAG_PREDICTOR),
  TAG_UINT16(         "resolutionunit", TIFFTAG_RESOLUTIONUNIT),
  TAG_UINT32(           "rowsperstrip", TIFFTAG_ROWSPERSTRIP),
  TAG_UINT16(           "sampleformat", TIFFTAG_SAMPLEFORMAT),
  TAG_UINT16(        "samplesperpixel", TIFFTAG_SAMPLESPERPIXEL),
  TAG_DOUBLE(        "smaxsamplevalue", TIFFTAG_SMAXSAMPLEVALUE),
  TAG_DOUBLE(        "sminsamplevalue", TIFFTAG_SMINSAMPLEVALUE),
  TAG_STRING(               "software", TIFFTAG_SOFTWARE),
  TAG_FLOAT(               "xposition", TIFFTAG_XPOSITION),
  TAG_FLOAT(             "xresolution", TIFFTAG_XRESOLUTION),
  TAG_FLOAT(               "yposition", TIFFTAG_YPOSITION),
  TAG_FLOAT(             "yresolution", TIFFTAG_YRESOLUTION),
  TAG_DOUBLE(                "stonits", TIFFTAG_STONITS),
  TAG_UINT32(            "subfiletype", TIFFTAG_SUBFILETYPE),
  TAG_STRING(          "targetprinter", TIFFTAG_TARGETPRINTER),
  TAG_UINT16(          "threshholding", TIFFTAG_THRESHHOLDING),
  TAG_UINT32(              "tiledepth", TIFFTAG_TILEDEPTH),
  TAG_UINT32(             "tilelength", TIFFTAG_TILELENGTH),
  TAG_UINT32(              "tilewidth", TIFFTAG_TILEWIDTH),
  TAG_UINT16(       "ycbcrpositioning", TIFFTAG_YCBCRPOSITIONING),
/*TAG_UINT16(          "ycbcrsampling", TIFFTAG_YCBCRSAMPLING),*/
  {0, 0, 0, 0},
};

#undef TAG_UINT16
#undef TAG_UINT32
#undef TAG_INT
#undef TAG_FLOAT
#undef TAG_DOUBLE
#undef TAG_STRING
#undef TAG_COLORMAP

static void tag_error(const char *errmsg, const char *tagname);
static void tag_error(const char *errmsg, const char *tagname)
{
  if (tagname) {
    sprintf(message, "%s \"%.40s%s\"", errmsg,
            tagname, (strlen(tagname)>40 ? "..." : ""));
    errmsg = message;
  }
  y_error(errmsg);
}

static void push_tag(object_t *this, long index)
{
  tag_t *entry;

  /* find TIFF tag entry in table */
  if (index == filename_index) {
    push_string(this->path);
  } else if (index == filemode_index) {
    push_string(this->mode);
  } else {
    for (entry = tag_table; entry -> name; ++entry) {
      if (entry->index == index) {
        entry->push(this->handle, entry->tag);
        return;
      }
    }
    tag_error("non-existing TIFF tag", yfind_name(index));
  }
}

static void on_extract(void* addr, char* name)
{
  push_tag((object_t *)addr, yget_global(name, 0));
}

static void on_eval(void *addr, int argc)
{
  long index;
  char *name;

  if (argc != 1) {
    y_error("expecting exactly one scalar string argument");
  }
  name = ygets_q(argc - 1);
  if (name) {
    index = yfind_global(name, 0);
  } else {
    index = -1L;
  }
  push_tag((object_t *)addr, index);
}


static void error_handler(const char* module, const char* fmt, va_list ap)
{
  char *ptr = message;
  strcpy(ptr, "TIFF");
  if (module) {
    strcat(ptr, " [");
    strcat(ptr, module);
    strcat(ptr, "]: ");
  } else {
    strcat(ptr, ": ");
  }
  vsprintf(ptr + strlen(ptr), fmt, ap);
}

static void warning_handler(const char* module, const char* fmt, va_list ap)
{
  if (debug) {
    fputs("TIFF WARNING", stderr);
    if (module) {
      fputs(" [", stderr);
      fputs(module, stderr);
      fputs("]: ", stderr);
    } else {
      fputs(": ", stderr);
    }
    vfprintf(stderr, fmt, ap);
    fputs("\n", stderr);
    fflush(stderr);
  }
}

/* on_free is automatically called by Yorick to cleanup object instance
   data when object is no longer referenced */
static void on_free(void *addr)
{
  object_t *this = (object_t *)addr;
  if (this->handle) TIFFClose(this->handle);
  if (this->path) p_free(this->path);
  if (this->mode) p_free(this->mode);
}

/* on_print is used by Yorick's info command */
static void on_print(void *addr)
{
  object_t *this = (object_t *)addr;
  y_print(tiff_type.type_name, 0);
  y_print(": path=\"", 0);
  y_print(this->path, 0);
  y_print("\"", 1);
}

static void bad_arg_list(const char *function)
{
  sprintf(message, "bad argument list to %s function", function);
  y_error(message);
}

void Y_tiff_debug(int argc)
{
  int prev = debug;
  if (argc!=1) bad_arg_list("tiff_debug");
  debug = yarg_true(0);
  ypush_int(prev);
}

void Y_tiff_open(int argc)
{
  object_t *this;
  char *filename, *filemode;

  /* Initialization. */
  if (filename_index < 0L) {
    tag_t *m;
    TIFFSetErrorHandler(error_handler);
    TIFFSetWarningHandler(warning_handler);
    for (m = tag_table; m->name; ++m) {
      m->index = yget_global(m->name, 0);
    }
    filemode_index = yget_global("filemode", 0);
    filename_index = yget_global("filename", 0);
  }
  message[0] = 0;

  if (argc<1 || argc>2) bad_arg_list("tiff_open");
  filename = ygets_q(argc - 1);
  filemode = (argc >= 2 ? ygets_q(argc - 2) : "r");

  /* Push new opaque object on the stack (which will be automatically
     destroyed in case of error). */
  this = (object_t *)ypush_obj(&tiff_type, sizeof(object_t));
  this->path = expand_name(filename);
  this->mode = p_strcpy(filemode);
  this->handle = TIFFOpen(this->path, filemode);
  if (! this->handle) y_error(message);
}

void Y_tiff_read_directory(int argc)
{
  int result;
  if (argc != 1) bad_arg_list("tiff_read_directory");
  message[0] = 0;
  result = TIFFReadDirectory(get_object(argc - 1)->handle);
  if (! result && message[0]) y_error(message);
  ypush_int(result);
}


void Y_tiff_read_image(int argc)
{
  long dims[Y_DIMSIZE];
  uint16 photometric, bitsPerSample;
  uint32 width, height, depth;
  void *raster;
  object_t *this;
  TIFF *tiff;
  int stopOnError;

  if (argc < 1 || argc > 2) bad_arg_list("tiff_read_image");
  this = get_object(argc - 1);
  tiff = this->handle;
  stopOnError = (argc >= 2 ? yarg_true(argc - 2) : 0);

  /* Get TIFF information to figure out the image type. */
  message[0] = 0; /* reset error message buffer */
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &photometric))
    missing_required_tag("photometric");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGEDEPTH, &depth))
    missing_required_tag("depth");
  if (depth != 1) y_error("TIFF depth != 1 not yet supported");

  /* Push image on top of the stack. */
  switch (photometric) {
  case PHOTOMETRIC_MINISWHITE: /* Grey and binary images. */
  case PHOTOMETRIC_MINISBLACK: /* Grey and binary images. */
    load_pixels(tiff);
    break;

  case PHOTOMETRIC_RGB: /* RGB Full color images */
  case PHOTOMETRIC_PALETTE: /* palette color images */
    /* Read RGBA image (do not stop on error). */
    if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample))
      missing_required_tag("bitsPerSample");
    if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGEWIDTH, &width))
      missing_required_tag("imageWidth");
    if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGELENGTH, &height))
      missing_required_tag("imageLength");
    dims[0] = 3;
    dims[1] = 4;
    dims[2] = width;
    dims[3] = height;
    raster = ypush_c(dims);
    if (! TIFFReadRGBAImage(tiff, width, height, raster, stopOnError)) {
      if (! message[0])
        strcpy(message, "TIFFReadRGBAImage failed to read complete image");
      if (stopOnError) y_error(message);
      fprintf(stderr, "TIFF WARNING: %s\n", message);
    }
    break;

  default:
    y_error("unknown photometric in TIFF file");
  }
}

void Y_tiff_read_pixels(int argc)
{
  if (argc != 1) bad_arg_list("tiff_read_pixels");
  load_pixels(get_object(argc - 1)->handle);
}

#if 0
void Y_tiff_SetField(int argc)
{
  object_t *this;
  TIFF *tiff;

  if (argc < 1 && argc % 2 != 1) bad_arg_list("TIFFSetField");
  this = get_object(argc - 1);
  tiff = this->handle;
}
#endif

static void missing_required_tag(const char *tagname)
{
  if (! message[0]) sprintf(message, "missing required TIFF tag \"%s\"",
                            tagname);
  y_error(message);
}

#if 0 /* unused */
static void set_tag_read_only(object_t *this, tag_t *m, Symbol *s)
{
  sprintf(message, "TIFF field \"%s\" is readonly", m->name);
  y_error(message);
}
#endif /* unused */

#if 0 /* unused */
static void cannot_set_member(tag_t *m);
static void cannot_set_member(tag_t *m)
{
  if (! message[0])
    sprintf(message, "cannot set value of TIFF field \"%s\"", m->name);
  y_error(message);
}
#endif /* unused */

static int cmapbits(unsigned int n,
                    const uint16 r[],
                    const uint16 g[],
                    const uint16 b[])
{
  unsigned int i;
  for (i = 0; i < n; ++i) {
    if (r[i] >= 256 || g[i] >= 256 || b[i] >= 256) return 16;
  }
  return 8; /* assume 8-bit colormap */
}

static object_t *get_object(int iarg)
{
  void *addr = yget_obj(iarg, &tiff_type);
  if (! addr) y_error("expecting TIFF object");
  return (object_t *)addr;
}

/*---------------------------------------------------------------------------*/

static void *push_workspace(long nbytes)
{
  long dims[2];
  dims[0] = 1;
  dims[1] = nbytes;
  return ypush_c(dims);
}

/*
 * TIFFTAG_PHOTOMETRIC             262     // photometric interpretation
 *     PHOTOMETRIC_MINISWHITE      0       // min value is white
 *     PHOTOMETRIC_MINISBLACK      1       // min value is black
 *     PHOTOMETRIC_RGB             2       // RGB color model
 *     PHOTOMETRIC_PALETTE         3       // color map indexed
 *     PHOTOMETRIC_MASK            4       // $holdout mask
 *     PHOTOMETRIC_SEPARATED       5       // !color separations
 *     PHOTOMETRIC_YCBCR           6       // !CCIR 601
 *     PHOTOMETRIC_CIELAB          8       // !1976 CIE L*a*b*
 *     PHOTOMETRIC_ITULAB          10      // ITU L*a*b*
 *     PHOTOMETRIC_LOGL            32844   // CIE Log2(L)
 *     PHOTOMETRIC_LOGLUV          32845   // CIE Log2(L) (u',v')
 */
static void load_pixels(TIFF *tiff)
{
  long dims[4];
  void *buf, *raster;
  tstrip_t strip, numberOfStrips;
  uint16 orientation;
  uint16 sampleFormat;
  uint16 samplesPerPixel;
  uint16 bitsPerSample;
  uint16 planarConfig;
  uint16 photometric;
  uint32 width, height, depth;
  uint32 rowsPerStrip;
  uint32 stripSize;
  uint32 x, y, y1, y0, rowLength, rowSize;
  int single, complexData = 0, typeid;

  /* Get information about pixels organization. */
  message[0] = 0;
  if (TIFFIsTiled(tiff))
    y_error("reading of tiled TIFF images not yet implemented");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &photometric))
    missing_required_tag("photometric");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel))
    missing_required_tag("samplesPerPixel");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample))
    missing_required_tag("bitsPerSample");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGEWIDTH,  &width))
    missing_required_tag("imageWidth");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGELENGTH, &height))
    missing_required_tag("imageLength");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_PLANARCONFIG, &planarConfig))
    missing_required_tag("planarConfig");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_IMAGEDEPTH,  &depth))
    missing_required_tag("imageDepth");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip))
    missing_required_tag("rowsPerStrip");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat))
    missing_required_tag("sampleFormat");
  if (! TIFFGetFieldDefaulted(tiff, TIFFTAG_ORIENTATION, &orientation))
    missing_required_tag("orientation");

  /* Figure out which data type to use for the result. */
  typeid = Y_VOID; /* deliberately initialize with a non-array type */
  if (sampleFormat == SAMPLEFORMAT_UINT ||
      sampleFormat == SAMPLEFORMAT_INT) {
    if (bitsPerSample == 1 ||
        bitsPerSample == 2 ||
        bitsPerSample == 4 ||
        bitsPerSample == 8) {
      typeid = Y_CHAR;
    } else if (bitsPerSample == 8*sizeof(long)) {
      typeid = Y_LONG;
    } else if (bitsPerSample == 8*sizeof(short)) {
      typeid = Y_SHORT;
    } else if (bitsPerSample == 8*sizeof(int)) {
      typeid = Y_INT;
    }
  } else if (sampleFormat == SAMPLEFORMAT_IEEEFP) {
    if (bitsPerSample == 8*sizeof(float)) {
      typeid = Y_FLOAT;
    } else if (bitsPerSample == 8*sizeof(double)) {
      typeid = Y_DOUBLE;
    }
#ifdef SAMPLEFORMAT_COMPLEXINT
  } else if (sampleFormat == SAMPLEFORMAT_COMPLEXINT) {
    complexData = 1;
    typeid = Y_COMPLEX;
#endif
#ifdef SAMPLEFORMAT_COMPLEXIEEEFP
  } else if (sampleFormat == SAMPLEFORMAT_COMPLEXIEEEFP) {
    complexData = 1;
    typeid = Y_COMPLEX;
#endif
  }
  if (complexData
#ifdef SAMPLEFORMAT_COMPLEXIEEEFP
      && (sampleFormat != SAMPLEFORMAT_COMPLEXIEEEFP ||
          bitsPerSample == 8*sizeof(double))
#endif
      ) {
    y_error("unsupported TIFF complex sample");
  }
  if (typeid == Y_VOID) {
    sprintf(message,
            "unsupported TIFF image data/format (BitsPerSample=%d, SampleFormat=%d)",
            (int)bitsPerSample, (int)sampleFormat);
    y_error(message);
  }

  /* We need to push 2 buffers onto the stack: a scanline/tile buffer and
     an image buffer. */
  ypush_check(2);

  /* Allocate a strip buffer onto the stack. */
  numberOfStrips = TIFFNumberOfStrips(tiff);
  stripSize = TIFFStripSize(tiff);
  buf = push_workspace(stripSize);

  /* Allocate raster image. */
  single = (samplesPerPixel == 1);
  if (single) {
    dims[0] = 2;
    dims[1] = width;
    dims[2] = height;
  } else {
    dims[0] = 3;
    dims[1] = samplesPerPixel;
    dims[2] = width;
    dims[3] = height;
  }
  raster = push_array(typeid, dims);

  rowLength = samplesPerPixel*width;
  rowSize = ((bitsPerSample + 7)/8)*rowLength;
  if (planarConfig == PLANARCONFIG_CONTIG) {
    for (strip=0, y0=0 ; y0<height ; y0=y1, ++strip) {
      /* Localization of next strip. */
      unsigned char *dst = ((unsigned char *)raster) + rowSize*y0;
      unsigned char *src = buf;

      /* Read next strip. */
      if (strip >= numberOfStrips) y_error("bad number of strips");
      TIFFReadEncodedStrip(tiff, strip, buf, stripSize);

      /* Convert every rows in the strip. */
      if ((y1 = y0 + rowsPerStrip) > height) y1 = height;
      if (bitsPerSample%8 == 0) {
        /* Just copy values. */
        memcpy(dst, src, rowSize*(y1 - y0));
      } else if (bitsPerSample == 1) {
        unsigned int mask=1, value;
        uint32 n = (rowLength/8)*8;
        for (y=y0 ; y<y1 ; ++y, dst+=rowLength) {
          for (x = 0; x < n; x+=8) {
            value = *src++;
            dst[x]   =  value     & mask;
            dst[x+1] = (value>>1) & mask;
            dst[x+2] = (value>>2) & mask;
            dst[x+3] = (value>>3) & mask;
            dst[x+4] = (value>>4) & mask;
            dst[x+5] = (value>>5) & mask;
            dst[x+6] = (value>>6) & mask;
            dst[x+7] = (value>>6) & mask;
          }
          if (x < rowLength) {
            for (value = *src++ ; x < rowLength ; ++x, value >>= 1) {
              dst[x] = value & mask;
            }
          }
        }
      } else if (bitsPerSample == 2) {
        unsigned int mask=3, value;
        uint32 n = (rowLength/4)*4;
        for (y=y0 ; y<y1 ; ++y, dst+=rowLength) {
          for (x = 0; x < n; x+=4) {
            value = *src++;
            dst[x]   =  value     & mask;
            dst[x+1] = (value>>2) & mask;
            dst[x+2] = (value>>4) & mask;
            dst[x+3] = (value>>6) & mask;
          }
          if (x < rowLength) {
            for (value = *src++ ; x < rowLength ; ++x, value >>= 2) {
              dst[x] = value & mask;
            }
          }
        }
      } else if (bitsPerSample == 4) {
        unsigned int mask=15, value;
        uint32 n = (rowLength/2)*2;
        for (y=y0 ; y<y1 ; ++y, dst+=rowLength) {
          for (x = 0; x < n; x+=2) {
            value = *src++;
            dst[x]   =  value     & mask;
            dst[x+2] = (value>>4) & mask;
          }
          if (x < rowLength) {
            for (value = *src++ ; x < rowLength ; ++x, value >>= 4) {
              dst[x] = value & mask;
            }
          }
        }
      }
    }
  } else {
    /* Must be PLANARCONFIG_SEPARATE. */
    y_error("unsupported TIFF planar configuration");
  }

  /* FIXME: take orientation into account... */
#if 0
  int transpose=0, revx=0, revy=0;
  switch (orientation) {
  case ORIENTATION_TOPLEFT:  /* row 0 top, col 0 lhs */
    revx = 0; revy = 1; transpose = 0; break;
  case ORIENTATION_TOPRIGHT: /* row 0 top, col 0 rhs */
    revx = 1; revy = 1; transpose = 0; break;
  case ORIENTATION_BOTRIGHT: /* row 0 bottom, col 0 rhs */
    revx = 1; revy = 0; transpose = 0; break;
  case ORIENTATION_BOTLEFT:  /* row 0 bottom, col 0 lhs */
    revx = 0; revy = 0; transpose = 0; break;
  case ORIENTATION_LEFTTOP:  /* row 0 lhs, col 0 top */
    revx = 0; revy = 0; transpose = 1; break;
  case ORIENTATION_RIGHTTOP: /* row 0 rhs, col 0 top */
    revx = 0; revy = 0; transpose = 1; break;
  case ORIENTATION_RIGHTBOT: /* row 0 rhs, col 0 bottom */
    revx = 0; revy = 0; transpose = 1; break;
  case ORIENTATION_LEFTBOT:  /* row 0 lhs, col 0 bottom */
    revx = 0; revy = 0; transpose = 1; break;
  }
  if (revx) {
    if (samplesPerPixel == 1) {
      for (y = 0; y < height; ++y) {
        type_t *p0 = ((type_t *)raster) + y*rowLength;
        type_t *p1 = p0 + rowLength - 1;
        while (p0 < p1) {
          tmp = *p0;
          *p0++ = *p1;
          *p1-- = tmp;
        }
      }
    } else {
      unsigned int s, n=samplesPerPixel;
      for (y = 0; y < height; ++y) {
        type_t *p0 = ((type_t *)raster) + y*rowLength;
        type_t *p1 = p0 + rowLength - samplesPerPixel;
        while (p0 < p1) {
          for (s = 0; s < n; ++s) {
            type_t tmp = p0[s]; p0[s] = p1[s]; p1[s] = tmp;
          }
          p0 += n;
          p1 -= n;
        }
      }
      }
    }
  }
#endif

  /* Revert to MinIsBlack photometric (if possible). */
  if (photometric == PHOTOMETRIC_MINISWHITE) {
    int warn=0;
    uint32 i, number = width*height*samplesPerPixel;
    if (sampleFormat == SAMPLEFORMAT_UINT ||
        sampleFormat == SAMPLEFORMAT_INT) {
      switch (bitsPerSample) {
#define REVERSE(type_t, ones) {					\
        type_t complement = ones, *ptr = (type_t *)raster;	\
        for (i = 0; i < number; ++i) ptr[i] ^= complement; }
      case   1: REVERSE(unsigned char, 0x00000001) break;
      case   2: REVERSE(unsigned char, 0x00000003) break;
      case   4: REVERSE(unsigned char, 0x0000000f) break;
      case   8: REVERSE(unsigned char, 0x000000ff) break;
      case  16: REVERSE(uint16,        0x0000ffff) break;
      case  32: REVERSE(uint32,        0xffffffff) break;
#undef REVERSE
      default:
        warn=1;
      }
    } else {
      warn=1;
    }
    if (warn) {
      fprintf(stderr, "warning: TIFF photometric MinIsWhite left unchanged\n");
    }
  }
}

/*---------------------------------------------------------------------------*/
#else /* not HAVE_TIFF */

static void no_tiff_support();

void Y_tiff_debug(int argc) { no_tiff_support(); }
void Y_tiff_open(int argc) { no_tiff_support(); }
void Y_tiff_read_directory(int argc) { no_tiff_support(); }
void Y_tiff_read_image(int argc) { no_tiff_support(); }
void Y_tiff_read_pixels(int argc) { no_tiff_support(); }

static void no_tiff_support()
{
  y_error("Yorick extension compiled without support for TIFF files");
}
#endif /* not HAVE_TIFF */
