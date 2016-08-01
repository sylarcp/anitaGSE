## Process this file with automake to produce Makefile.in
# Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# $Header: /cvsroot/anitacode/gse/unpack/Makefile.am,v 1.1.1.1 2005/07/20 03:12:18 predragm Exp $

#CXXFLAGS += -O3
CXXFLAGS += -g -O0

bin_PROGRAMS = unpackd

MYSRC = utilityFunctions.cc value.cc word.cc utilityFunctions.h value.h word.h

unpackd_SOURCES = unpackd.cc unpackClasses.cc unpackClasses.h fileHandler.cc fileHandler.h $(MYSRC)
unpackd_LDADD =  ../lib/libanitagse.la -llddl -lfl -lpq ${FLIGHTSOFT_LIBDIR}/libCompress.a ${FLIGHTSOFT_LIBDIR}/libPedestal.a ${FLIGHTSOFT_LIBDIR}/libUtil.a -lz



