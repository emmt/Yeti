
# Yeti: a Yorick extension

Yeti is an extension of Yorick (a fast interpreted interactive data processing
language written by David Munro) which implements (see "*Quick Reference*" below
for a list of additional functions):
 * hash table objects
 * regular expressions
 * complex, real-complex and complex-real FFT by FFTW (the Fastest Fourier
   Transform in the West -- version 2)
 * wavelet filtering ("*à trou*" method)
 * fast convolution along a chosen dimension with various border conditions
 * more string functions
 * memory hacking routines
 * more math functions (sinc, round, arc)
 * generalized matrix-vector multiplication (with possibly sparse matrix)
 * routines to query/check Yorick's symbols
 * support for reading TIFF images
 * morpho-math operators
 * ...

This distribution of Yeti may come with several extensions (depending
whether corresponding directories exist or not):
 * `fftw` ..... support for FFTW
 * `regex` .... support for POSIX regular expressions
 * `tiff` ..... support for reading TIFF images


## Compilation and Installation

Starting with version 6.0.0, Yeti is built as a regular Yorick plugin and some
of the Yeti extensions are built as standalone Yorick plugins (they do not
require Yeti to be used). The installation of Yeti depends on that of Yorick
which must have been installed prior to Yeti. You'll need at least version 2.2
of Yorick (for Yorick version 1.5, you can install Yeti 5.3 and for Yorick 1.6
to 2.1 you can install Yeti 6.2).

The first installation step consists in the configuration of Yeti software
suite.  This is done via the "configure" script (1).  In order to figure out
the different options (and their default values), you can just do:

    ./configure --help

At least version 2.2 of Yorick is required to configure/build this version of
Yeti.

By default, the script tries to find an axecutable named "yorick" in your path
(according to your environment variable PATH).  You can however specify a
different Yorick executable with option:

    ./configure [...] --yorick=PATH_TO_YORICK_EXECUTABLE [...]

with `PATH_TO_YORICK_EXECUTABLE` the full path to Yorick executable.

If you want Yeti plugin for extended regular support, you'll have the choice
to use a system provided POSIX REGEX library or the GNU REGEX library which
can be built into the plugin. This is achieved by defining the preprocessor
macro `HAVE_REGEX` to true if you trust your system REGEX library.  To use the
POSIX REGEX library of your system:

    ./configure [...] --with-regex-defs='-DHAVE_REGEX=1' [...]

and to build the GNU REGEX library into the plugin (this is the default
behaviour):

    ./configure [...] --with-regex-defs='-DHAVE_REGEX=0' [...]

For instance, here is how to call the configure script to use builtin REGEX
support and to enable plugins for FFTW (installed in /usr/local) and TIFF
(installed in standard locations):

    ./configure --with-regex \
        --with-fftw --with-fftw-defs="-I/usr/local/include" \
        --with-fftw-libs="-L/usr/local/lib -lrfftw -lfftw" \
        --with-tiff --with-tiff-libs="-ltiff"

In order to check your configuration settings, you can add `--help` as the
last argument of the call to `./configure`.

After configuration, you can build Yeti and all related plugins by just doing:

    make

In order to install Yeti files into Yorick installation directories,
you simply do:

    make install

Note that the `configure` script is actually a wrapper around the command:

    yorick -batch ./config.i [...]

with [...] the arguments passed to `configure`.


## Quick Reference

    h_cleanup ............ delete void members of hash table object
    h_clone .............. clone a hash table
    h_copy ............... duplicate hash table object
    h_delete ............. delete members from a hash table
    h_first .............. get name of first hash table member
    h_functor ............ create "functor" object
    h_get ................ get value of hash table member
    h_has ................ check existence of hash table member
    h_info ............... list contents of hash table
    h_keys ............... get member names of a hash table object
    h_list ............... make a hash table into a list
    h_new. ............... create a new hash table object
    h_next ............... get name of next hash table member
    h_pop ................ pop member out of an hash table object
    h_restore_builtin .... restore builtin functions
    h_save ............... save variables in a hash table
    h_save_symbols ....... save builtin functions
    h_set................. set member of hash table object
    h_set_copy............ set member of hash table object
    h_show ............... display a hash table as an expanded tree
    h_stat ............... get statistics of hash table object


Yeti Hierarchical Data (YHD) files:

    #include "yeti_yhdf.i"

    yhd_check ............ check version of YHD file
    yhd_info ............. print some information about an YHD file
    yhd_restore .......... restore a hash table object from an YHD file
    yhd_save ............. save a hash table object into an YHD file


Regular Expressions:

    #include "yeti_regex.i"

    regcomp ............... compile regular expression
    regmatch .............. match a regular expression against an array of strings
    regmatch_part ......... peek substrings given indices returned by regmatch
    regsub ................ substitute regular expression into an array of strings


Miscellaneous:

    anonymous ............. create anonymous (lambda) functions
    expand_path ........... expand directory names to absolute paths
    heapsort............... sort an array by heap-sort method
    insure_temporary ...... make sure a variable is not referenced
    is_hash ............... check if an object is a hash table
    is_sparse_matrix ...... check if an object is a sparse matrix
    quick_select .......... find K-th smallest element in an array
    quick_median .......... find median value (faster than median function)
    quick_interquartile_range
     ...................... compute inter-quartile range of values
    yeti_init setup ....... setup Yeti internals and query version


Memory Hacking:

    mem_base .............. get base address of an array object
    mem_copy .............. copy array data at a given address
    mem_info .............. print memory information
    mem_peek .............. make a new array from a base address, type and
                            dimension list
    native_byte_order ..... compute native byte order


Binary Encoding of Data:

    get_encoding .......... get description of binary encoding for various
                            machines
    install_encoding ...... install binary description into a binary stream
    same_encoding ......... compare two encodings


Math/Numerical:

    arc ................... lengh of arc in radians
    cost_l2 ............... cost function and gradient for l2 norm
    cost_l2l0 ............. cost function and gradient for l2-l0 norm
    cost_l2l1 ............. cost function and gradient for l2-l1 norm
    machine_constant ...... get machine dependent constant (such as EPSILON)
    mvmult ................ (sparse)matrix-vector multiplication
    round ................. round to nearest integer
    sinc .................. cardinal sine: sinc(x) = sin(pi*x)/(pi*x)
    smooth3 ............... smooth an array by 3-element convolution
    sparse_expand ......... convert a sparse matrix array into a regular array
    sparse_grow ........... augment a sparse array
    sparse_matrix ......... create a new sparse matrix
    sparse_squeeze ........ convert a regular array into a sparse one
    yeti_convolve ......... convolution along a given dimension
    yeti_wavelet .......... "à trou" wavelet decomposition


Strings:

    strlower .............. convert array of strings to lower case
    strtrimleft ........... remove leading spaces from an array of strings
    strtrimright .......... remove trailing spaces from an array of strings
    strupper .............. convert array of strings to upper case


Yorick Internals:

    memory_info ........... display memory used by Yorick symbols
    symbol_info ........... get some information about existing Yorick symbols
    nrefsof ............... get number of references of an object


Morpho-math operations:

    morph_black_top_hat ... perform valley detection
    morph_closing ......... perform morpho-math closing operation
    morph_dilation ........ perform morpho-math dilation operation
    morph_erosion ......... perform morpho-math erosion operation
    morph_opening ......... perform morpho-math opening operation
    morph_white_top_hat ... perform summit detection


TIFF images:

    #include "yeti_tiff.i"

    tiff_open ............. open TIFF file
    tiff_debug ............ control printing of TIFF warning messages
    tiff_read_pixels ...... read pixel values in a TIFF file
    tiff_read_image ....... read image in a TIFF file
    tiff_read_directory ... move to next TIFF "directory"
    tiff_read ............. read image/pixels in a TIFF file
    tiff_check ............ check if a file is a readable TIFF file.


FFTW:

    #include "yeti_fftw.i"

    fftw_plan ............. setup a plan for FFTW
    fftw .................. computes FFT of an array according to a plan
    cfftw ................. computes complex FFT of an array
    fftw_indgen ........... generates FFT indices
    fftw_dist ............. computes length of spatial frequencies
    fftw_smooth ........... smooths an array
    fftw_convolve ......... fast convolution of two arrays


## Copyright and Warranty

(See file [LICENSE](./LICENSE.md) for details.)

> Copyright (C), 1996-2015: Éric Thiébaut <thiebaut@obs.univ-lyon1.fr>
>
> This software is governed by the CeCILL-C license under French law and abiding
> by the rules of distribution of free software.  You can use, modify and/or
> redistribute the software under the terms of the CeCILL-C license as
> circulated by CEA, CNRS and INRIA at the following URL
> "http://www.cecill.info".
>
> As a counterpart to the access to the source code and rights to copy, modify
> and redistribute granted by the license, users are provided only with a
> limited warranty and the software's author, the holder of the economic rights,
> and the successive licensors have only limited liability.
>
> In this respect, the user's attention is drawn to the risks associated with
> loading, using, modifying and/or developing or reproducing the software by the
> user in light of its specific status of free software, that may mean that it
> is complicated to manipulate, and that also therefore means that it is
> reserved for developers and experienced professionals having in-depth computer
> knowledge. Users are therefore encouraged to load and test the software's
> suitability as regards their requirements in conditions enabling the security
> of their systems and/or data to be ensured and, more generally, to use and
> operate it in the same conditions as regards security.
>
> The fact that you are presently reading this means that you have had knowledge
> of the CeCILL-C license and that you accept its terms.


## References and notes

1. [Yorick](http://github.com/dhmunro/yorick) is an interpreted programming
   language for scientific simulations or calculations, postprocessing or
   steering large simulation codes, interactive scientific graphics, and
   reading, writing, or translating large files of numbers.

2. [FFTW](http://www.fftw.org/) is *the Fastest Fourier Transform in the
   West*.  Please note that since FFTW API has changed with FFTW version 3,
   only FFTW version 2 (latest is 2.1.5) is supported in Yeti.  A new
   plug-in [XFFT](https://github.com/emmt/xfft) will be soon available for
   FFTW3 in Yorick.

3. To use some special functions of [GSL](http://www.gnu.org/software/gsl/)
   (the GNU Scientific Library) in Yorick, `yeti_gsl` has been abandoned in
   favor of a separate plugin [YGSL](https://github.com/emmt/ygsl).

4. [LibTIFF](http://www.libtiff.org/) is a free TIFF library which provides
   support for the *Tag Image File Format* (TIFF), a widely used format for
   storing image data.
