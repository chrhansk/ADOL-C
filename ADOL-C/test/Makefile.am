##############################################################################
## Makefile.am -- Process this file with automake to produce Makefile.in
## Revision: $Id: Makefile.am 61 2009-12-07 14:49:34Z awalther $
##
## Copyright (C) Andrea Walther
##
## This file is part of ADOL-C. This software is provided as open source.
## Any use, reproduction, or distribution of the software constitutes 
## recipient's acceptance of the terms of the accompanying license file.
## 
##############################################################################

noinst_PROGRAMS         = powexam speelpenning

speelpenning_SOURCES    = speelpenning.cpp

powexam_SOURCES         = powexam.cpp


powexam_LDADD = ../src/libadolc.la $(ADDLIBS) $(CXXLIBS)

speelpenning_LDADD = ../src/libadolc.la $(ADDLIBS) $(CXXLIBS)

# Here list all include flags, relative to this "srcdir" directory.  This
# "cygpath" stuff is necessary to compile with native compilers on Cygwin
AM_CPPFLAGS = -I$(srcdir)/../src/ \
	-I$(srcdir)/../src/drivers \
	-I$(srcdir)/../src/sparse \
	-I$(srcdir)/../src/tapedoc 

# This line is necessary to allow VPATH compilation with MS compilers
# on Cygwin
DEFAULT_INCLUDES = -I. -I$(srcdir) -I$(top_builddir)/inc

test:	powexam speelpenning 
	chmod u+x ./run_tests
	./run_tests
