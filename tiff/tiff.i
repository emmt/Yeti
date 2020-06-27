/*
 * tiff.i --
 *
 * Support for TIFF images in Yorick.
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

/* load dynamic code */
if (is_func(plug_in)) plug_in, "yor_tiff";

extern tiff_open;
/* DOCUMENT tiff_open(filename)
         or tiff_open(filename, filemode)
     Open TIFF file FILENAME with mode FILEMODE (default "r", i.e. reading)
     and return an opaque handle that  can be used to manage the TIFF file.
     The returned  handle can also be  used like a structure  to query TIFF
     field or pseudo-tag value, e.g.:

        obj = tiff_open(filename);
        width = obj.imagewidth;    // width of image
        height = obj.imagelength;  // height of image

     If TIFF field or pseudo-tag MEMBER is undefined, then OBJ.MEMBER gives
     a nil value;  but if the field or pseudo-tag  MEMBER is unsupported or
     does  not  exist in  TIFF  specification,  an  error is  raised.   The
     identifiers of TIFF fields or  pseudo-tags can be guessed from the tag
     macros   in  "tiff.h"   header   file  from   libtiff  library   (e.g.
     TIFFTAG_IMAGEDESCRIPTION  becomes "imagedescription"), this  file also
     gives some description about the tag values.  Additional tags are:

        obj.filename    // full path of file (scalar string)
        obj.filemode    // file mode (scalar string)

     The value of the TIFF tags can also be obtained by indexing the TIFF
     object with the tag name, for instance:

        width = obj("imagewidth");    // width of image
        path = obj("filename");       // path of image file


   SEE ALSO: tiff_debug, tiff_read_directory, tiff_read_image.
 */

extern tiff_debug;
/* DOCUMENT tiff_debug(value)
     Set  debug flag  for TIFF  operations and  returns the  previous debug
     value.   If  VALUE is  true,  TIFF  warning  messages get  printed  to
     standard error; otherwise these messages are never printed.
 */

extern tiff_read_pixels;
/* DOCUMENT tiff_read_pixels(obj)
     Returns  pixel  values  of  image  in  current  "TIFF-directory"  (see
     tiff_read_directory)   of  TIFF   handle   OBJ.   The   result  is   a
     WIDTH-by-HEIGHT array  for a  grayscale or a  colormapped image  and a
     SAMPLESPERPIXEL-by-WIDTH-by-HEIGHT array for an RGB image.

   SEE ALSO: tiff_debug, tiff_open, tiff_read_directory, tiff_read_image.
 */

extern tiff_read_image;
/* DOCUMENT tiff_read_image(obj)
         or tiff_read_image(obj, stop_on_error)
     Returns    image    data     in    current    "TIFF-directory"    (see
     tiff_read_directory)   of  TIFF   handle   OBJ.   The   result  is   a
     WIDTH-by-HEIGHT array for a grayscale image and a 4-by-WIDTH-by-HEIGHT
     array  for a  color image  (the  first dimension  correspond to:  red,
     green, blue  and alpha channels).  If  optional argument STOP_ON_ERROR
     is true  (it is false  by default), then  any error while  reading the
     image get raised; otherwise, a simple warning is printed out (note: in
     any case,  an error is raised if  there is not enough  memory to store
     the image or if the image format is unsupported).

   SEE ALSO: tiff_debug, tiff_open, tiff_read_directory, tiff_read_pixels.
 */

extern tiff_read_directory;
/* DOCUMENT tiff_read_directory(obj)
     Read the next directory in the  specified file and make it the current
     directory.  Applications only need to call tiff_read_directory to read
     multiple subfiles  in a single TIFF  file -- the first  directory in a
     file  is automatically  read when  tiff_open is  called.  If  the next
     directory was  successfully read, 1 is returned.   Otherwise, if there
     are no more directories to be read, 0 is returned.

   SEE ALSO: tiff_debug, tiff_open, tiff_read_directory.
 */


func tiff_read(filename, index=, raw=)
/* DOCUMENT tiff_read(filename)
     Return  a  grayscale  or  RGBA  image from  TIFF  file  FILENAME  (see
     tiff_read_image) or  an array of pixel  values if keyword  RAW is true
     (see tiff_read_pixels).  Keyword INDEX may be used to specify the TIFF
     directory number of the image to read (default INDEX=1).

   SEE ALSO tiff_open, tiff_read_directory, tiff_read_image,
            tiff_read_pixels.
 */
{
  this = tiff_open(filename);
  if (! is_void(index)) {
    if (index < 1) error, "bad index";
    while (--index) {
      if (! tiff_read_directory(this)) error, "index too large";
    }
  }
  //photometric = this.photometric;
  return (raw ? tiff_read_pixels(this) : tiff_read_image(this, 0));
}

func tiff_check(filename)
/* DOCUMENT tiff_check(filename)
     Check whether or not FILENAME is an existing TIFF file, returned value
     is: 1 if FILENAME is a readable big endian TIFF file,
         2 if FILENAME is a readable little endian TIFF file,
         0 otherwise (FILENAME unreadable or not a TIFF file).
     Note  that the  check is  intended to  be fast,  it is  not completely
     reliable since only the 2 first bytes of FILENAME are used.

   SEE ALSO: tiff_open.
 */
{
  magic = array(char, 2);
  if ((f = open(filename, "rb", 1)) &&
      _read(f, 0, magic) == sizeof(magic) &&
      (m = magic(1)) == magic(2)) {
    if (m == 0x4d) return 1; /* big endian */
    if (m == 0x49) return 2; /* little endian */
  }
  return 0;
}
