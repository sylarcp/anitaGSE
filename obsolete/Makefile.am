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

CXXFLAGS += -fno-inline 

noinst_PROGRAMS = lddl_unpackd_testlite shmem_dataprint datmon viewdump shmem_lddl datmon_raw

MYSRC = ../unpack/utilityFunctions.cc ../unpack/value.cc ../unpack/word.cc ../unpack/utilityFunctions.h ../unpack/value.h ../unpack/word.h

lddl_unpackd_testlite_SOURCES = \
	lddl_unpackd_testlite.cc unpackd_testlite.cc unpackd_testlite.h \
	$(MYSRC)
lddl_unpackd_testlite_LDADD =  -llddl -lfl -lpq -lpthread

shmem_dataprint_SOURCES = shmem_dataprint.c parsenum.c ../monitor/shmem.c ../monitor/sem.c
shmem_lddl_SOURCES = shmem_lddl.c ../monitor/shmem.c ../monitor/sem.c ../monitor/shmem_def.h
datmon_SOURCES = datmon.c ../monitor/shmem.c ../monitor/sem.c

viewdump_LDADD = -lncurses
datmon_LDADD = -lncurses
shmem_dataprint_LDADD = -llddl -lfl
shmem_lddl_LDADD = -llddl -lfl

EXTRA_DIST = anitalite.fmt AcqDAQ.h create_tables_testlite.sql

