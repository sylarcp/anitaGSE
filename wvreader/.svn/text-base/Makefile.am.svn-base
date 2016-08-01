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

CFLAGS += -O3

bin_PROGRAMS = wvreader

wvreader_SOURCES = wvreader.c

LDFLAGS = -all-static
wvreader_LDADD = ${FLIGHTSOFT_LIBDIR}/libCompress.a ${FLIGHTSOFT_LIBDIR}/libPedestal.a ${FLIGHTSOFT_LIBDIR}/libUtil.a -lz -lm



