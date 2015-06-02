/*
 * Yorick script to generate alphabetized listings of all help
 * command documentation.
 */

#include "mkdoc.i"

/* assume current working directory is top level of distribution */
mkdoc, ["../core/yeti.i", "../core/yeti_yhdf.i"], "yeti.doc";
mkdoc, "../fftw/yeti_fftw.i", "yeti_fftw.doc";
mkdoc, "../regex/yeti_regex.i", "yeti_regex.doc";
mkdoc, "../tiff/yeti_tiff.i", "yeti_tiff.doc";

quit;
