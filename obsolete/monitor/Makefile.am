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
# $Header: /cvsroot/anitacode/gse/monitor/Makefile.am,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $

bin_PROGRAMS = datmon

SHMEM_SRC = shmem.c sem.c shmem_def.h shmem.h sem.h

datmon_SOURCES = datmon.c  $(SHMEM_SRC)

LDADD = ../lib/libanitagse.la -lncurses -lm



