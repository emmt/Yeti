#
# Makefile -
#
# Makefile for Yeti main plugin.
#
#------------------------------------------------------------------------------
#
# Copyright (C) 2006-2010, Eric Thiébaut <thiebaut@obs.univ-lyon1.fr>
#
# This software is governed by the CeCILL-C license under French law and
# abiding by the rules of distribution of free software.  You can use, modify
# and/or redistribute the software under the terms of the CeCILL-C license as
# circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info".
#
# As a counterpart to the access to the source code and rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty and the software's author, the holder of the
# economic rights, and the successive licensors have only limited liability.
#
# In this respect, the user's attention is drawn to the risks associated with
# loading, using, modifying and/or developing or reproducing the software by
# the user in light of its specific status of free software, that may mean
# that it is complicated to manipulate, and that also therefore means that it
# is reserved for developers and experienced professionals having in-depth
# computer knowledge. Users are therefore encouraged to load and test the
# software's suitability as regards their requirements in conditions enabling
# the security of their systems and/or data to be ensured and, more
# generally, to use and operate it in the same conditions as regards
# security.
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL-C license and that you accept its terms.
#------------------------------------------------------------------------------

# where are the sources?
srcdir=.

# these values filled in by: yorick -batch make.i
Y_MAKEDIR=
Y_EXE=
Y_EXE_PKGS=
Y_EXE_HOME=
Y_EXE_SITE=
Y_HOME_PKG=

#----------------------------------------------------------- optimization flags

# options for make command line, e.g.-   make COPT=-g TGT=exe
COPT=$(COPT_DEFAULT)
TGT=$(DEFAULT_TGT)

#------------------------------------------------------ macros for this package

PKG_NAME=yeti
PKG_I=$(srcdir)/yeti.i

OBJS = yeti_convolve.o \
       yeti_cost.o \
       yeti_hash.o \
       yeti_sort.o \
       yeti_math.o \
       yeti_misc.o \
       yeti_morph.o \
       yeti_new.o \
       yeti_rgl.o \
       yeti_sparse.o \
       yeti_symlink.o \
       yeti_utils.o

# change to give the executable a name other than yorick
PKG_EXENAME = yorick

# PKG_DEPLIBS=-Lsomedir -lsomelib   for dependencies of this package
PKG_DEPLIBS =
# set compiler (or rarely loader) flags specific to this package
PKG_CFLAGS = -I..
PKG_LDFLAGS =

# list of additional package names you want in PKG_EXENAME
# (typically Y_EXE_PKGS should be first here)
EXTRA_PKGS=$(Y_EXE_PKGS)

# list of additional files for clean
PKG_CLEAN=$(PKG_NAME)$(PLUG_SFX) $(PKG_NAME).dll $(PKG_NAME).so

# autoload file for this package, if any
PKG_I_START=$(srcdir)/yeti-start.i
# non-pkg.i include files for this package, if any
PKG_I_EXTRA=$(srcdir)/yeti_yhdf.i

#-------------------------------------- standard targets and rules (in Makepkg)

# set macros Makepkg uses in target and dependency names
# DLL_TARGETS, LIB_TARGETS, EXE_TARGETS
# are any additional targets (defined below) prerequisite to
# the plugin library, archive library, and executable, respectively
PKG_I_DEPS=$(PKG_I)
Y_DISTMAKE=distmake

include $(Y_MAKEDIR)/Make.cfg
include $(Y_MAKEDIR)/Makepkg
include $(Y_MAKEDIR)/Make$(TGT)

# override macros Makepkg sets for rules and other macros
# Y_HOME and Y_SITE in Make.cfg may not be correct (e.g.- relocatable)
Y_HOME=$(Y_EXE_HOME)
Y_SITE=$(Y_EXE_SITE)

# reduce chance of yorick-1.5 corrupting this Makefile
MAKE_TEMPLATE = protect-against-1.5

#------------------------------------------- targets and rules for this package

# simple example:
#myfunc.o: myapi.h
# more complex example (also consider using PKG_CFLAGS above):
#myfunc.o: myapi.h myfunc.c
#	$(CC) $(CPPFLAGS) $(CFLAGS) -DMY_SWITCH -o $@ -c myfunc.c

yeti_convolve.o: yeti.h
#yeti_cost.o:
yeti_eigen.o: yeti_eigen.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DYORICK -o $@ -c $<
yeti_hash.o: yeti.h ../config.h
yeti_heapsort.o: yeti.h ../config.h
yeti_math.o: yeti.h ../config.h
yeti_misc.o: yeti.h ../config.h
yeti_morph.o: yeti.h ../config.h
yeti_rgl.o: yeti_rgl.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DYORICK -o $@ -c $<
yeti_utils.o: yeti.h ../config.h
#yeti_new.o:
