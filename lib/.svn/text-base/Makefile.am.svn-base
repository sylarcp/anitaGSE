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
# $Header$


CFLAGS += -O3

pkglib_LTLIBRARIES = libanitagse.la
pkginclude_HEADERS = anitaGSE.h crc_simple.h

# Files for main library
libanitagse_la_INCLUDES = $(pkginclude_HEADERS) 
libanitagse_la_SOURCES = pckTools.c crc_simple.c $(libanitagse_la_INCLUDES)

libanitagse_la_LDFLAGS = -version-info 1:1:0

EXTRA_DIST =

## Testing
#LDADD = libanitagse.la
#LDFLAGS = -static

#noinst_PROGRAMS = test_blah


