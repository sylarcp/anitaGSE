
/*
 Copyright (C) 2003 Martin A. Olevitch
 Written by Martin A. Olevitch (marty@cosray.wustl.edu)

 This file is part of LDDL.

 LDDL is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 LDDL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with LDDL, see the file COPYING; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* dataprint.c
 *
 * Marty Olevitch, Feb 2000
 * 
 * Modification of dataprint to print to shared memory instead of screen
 * 
 * Predrag Miocinovic Jun 2005
 */

#include <stdio.h>
#include <unistd.h>
static char *Progname;

static void
usage(void)
{
    fprintf(stderr,
    	"\nusage: %s -f fmtfile [-b rdsize] [-N] [-n] [-d n] [file ...]\n",
    	Progname);
    fprintf(stderr, "     -b rdsize     initial size of lddl read buffer\n");
    fprintf(stderr, "     -f fmtfile    name of format file\n");
    fprintf(stderr, "     -N            data IS in native endianness\n");
    fprintf(stderr, "     -n            data IS NOT in native endianness\n");
    fprintf(stderr, "     -d <n>        node processing delay; default 0 us\n");
    fprintf(stderr, "     file          input data file(s) (def. stdin)\n");
}

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <lddl.h>
#include "parsenum.h"
#include "shmem.h"

static char *cvsid = "$Id: shmem_dataprint.c,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $";

#ifdef USE_INT_FOR_LONG
/* dec alpha long is 8 bytes, but int is 4 */
#define long int
#define u_long int
#endif

#ifndef STREQ
#define STREQ(a, b)	(strcmp((a), (b)) == 0)
#endif

int node(char *inp, char *swapp, int type, int count, int nbytes, int
    argc, char **argv, int native, char *group_name, char *evt_name);

int post_evt(char *name, int argc, char **argv);

int group(int argc, char **argv, char *grpname, char *evtname);

static char * estrdup(char *s);
static int node_parse(char *p, int type, int count, int argc, char**argv);
static int pack_parse(char *p, int argc, char**argv);
static void packprint(char *fmt, char *p);
static void printem(char *fmt, char *spec, char *p, int type, int count);
static void usage(void);


static int Evtcnt = 0;
static int Printed_something = 0;

/* states for the automatons */
enum {
    START,
    PRINT,
    NSPEC,
    PSPEC,
    GSPEC,
};

/* String to store output data into before shifting to shared memory */
#define MAX_DATA 1024
static char OutputStr[MAX_DATA];
static int nChar=0;

/* Shared memory definitions */
#define SHMEM_SEM_FILE "/tmp/dataprint"
#define SHMEM_KEY_DATAPRINT 5761L
#define NBUF 50  // Length of shared memory buffer
static int nBuf=0;
static unsigned long Shmem_Length[NBUF];

static long nodeDelay=0; // Delay in processing nodes to simulate slower data pipe; us

int
main(int argc, char *argv[])
{
  int c,i;
    int errflg = 0;
    char *fmtfile = NULL;
    extern int optind;
    extern char *optarg;

    Progname = argv[0];

    while ((c = getopt(argc, argv, "b:f:d:Nn")) != EOF) {
	switch (c) {
	    case 'b':
		{
		    unsigned long b = atol(optarg);
		    fprintf(stderr, "%s: using buffer size of %lu\n",
			Progname, b);
		    lddl_set_buf_size(b);
		}
		break;
	    case 'f':
	    	fmtfile = optarg;
		break;
	    case 'N':
	    	/* assume native endianness */
		lddl_set_native(1);
		break;
	    case 'n':
	    	/* assume non-native endianness */
		lddl_set_native(0);
		break;
	    case 'd':
	      nodeDelay=atol(optarg);
	      break;
	    case '?':
	    default:
	    	errflg++;
		break;
	}
    }

    if (errflg) {
	usage();
	exit(1);
    }

    /* Setup shared memory */
    for(i=0;i<NBUF;++i) Shmem_Length[i]=MAX_DATA;
    if(shmem_init(SHMEM_SEM_FILE, SHMEM_KEY_DATAPRINT, NBUF, Shmem_Length, 1)){
      fprintf(stderr, "%s: ERROR! bad shmem_init (%s). \n",
		Progname, shmem_strerror());
      exit(1);
    }

    lddl_set_group_func(group);
    lddl_set_post_evt_func(post_evt);
    lddl_set_node_func(node);
    (void)lddl_search_for_evt_start(1);
    lddl_start(fmtfile, argc-optind, argv+optind, Progname);
    //printf("%d\n", Evtcnt); 

    shmem_unlink();

    return 0;
}

int 
post_evt(char *name, int argc, char **argv)
{
  int i,ret; 

    Evtcnt++;
    if (Printed_something) {
      OutputStr[nChar++]='\0';
      for (i=0; i<5; i++) {
	ret = shmem_write(nBuf, OutputStr, (unsigned long)nChar);
	if (ret == 0) break;
	usleep(10L);
      }
      if(ret == -2) fprintf(stderr,"[shmem buffer %d locked]\n",nBuf);
      else if(ret == -1) fprintf(stderr,"[shmem buffer %d error]\n",nBuf);
      else{
	if(++nBuf>=NBUF) nBuf=0;
      }
      Printed_something = 0;
      nChar=0;
    }
}

int
group(int argc, char **argv, char *group_name, char *evtname)
{
    return 0;
}

int
node(char *inp, char *swapp, int type, int count, int nbytes, int
    argc, char **argv, int native, char *group_name, char *evt_name)
{
    char *p;
    if(nodeDelay) 
      usleep(nodeDelay); // slow it down a bit to simulate slower data pipe

    p = native ? inp : swapp;

    if (group_name != NULL && STREQ(group_name, "pack")) {
	return(pack_parse(p, argc, argv));
    } else {
	return(node_parse(p, type, count, argc, argv));
    }
}

static int
node_parse(char *p, int type, int count, int argc, char**argv)
{
    int a;
    char *fmt = NULL;
    char *spec = NULL;
    int state = START;

    for (a=0; a < argc; a++) {
	switch (state) {
	    case START:
	    	if (STREQ(argv[a], "print")) {
		    state = PRINT;
		} else if (STREQ(argv[a], ";")) {
		    ;
		} else {
		    fprintf(stderr,
		    	"node: expecting 'print' or ';', got '%s'\n", argv[a]);
		    return -1;
		}
		break;
	    case PRINT:
	    	if (STREQ(argv[a], "-n")) {
		    state = NSPEC;
		} else if (STREQ(argv[a], ";")) {
		    state = START;
		    printem(fmt, spec, p, type, count);
		} else {
		    fmt = argv[a];
		}
		break;
	    case NSPEC:
	    	spec = argv[a];
		state = PRINT;
		break;
	    default:
	    	fprintf(stderr, "node: bad state - shouldn't happen\n");
		return -1;
	}
    }

    if (state == PRINT) {
	printem(fmt, spec, p, type, count);
    } else if (state == NSPEC) {
	fprintf(stderr, "Not enough arguments for -n\n");
	return -1;
    }

    return 0;
}

static void
printem(char *fmt, char *spec, char *p, int type, int count)
{
    unsigned char bval;
    double dval;
    float fval;
    int i;
    unsigned long lval;
    int n;
    int nullfmt = 0;
    int nullspec = 0;
    unsigned short sval;
    int *todo;

    if (spec == NULL) {
	spec = malloc(1024);
	if (spec == NULL) {
	    fprintf(stderr, "can't malloc\n");
	    exit(1);
	}
	sprintf(spec, "0-%d", count-1);
	nullspec = 1;
    }

    todo = (int *)malloc(count * sizeof(int));
    if (todo == NULL) {
	fprintf(stderr, "can't malloc\n");
	exit(1);
    }

    if (fmt == NULL) {
	nullfmt = 1;
	switch (type) {
	    case LDDL_INT8:	fmt = estrdup("%02x "); break;
	    case LDDL_INT16:	fmt = estrdup("%04x "); break;
	    case LDDL_INT32:	fmt = estrdup("%08x "); break;
	    case LDDL_FLOAT32:	fmt = estrdup("%f "); break;
	    case LDDL_FLOAT64:	fmt = estrdup("%f "); break;
	    case LDDL_GROUP:
	    	/* shouldn't happen - fall through */
	    default:
	    	fprintf(stderr, "unknown type (%d)\n", type);
		return;
	}
    }

    Printed_something = 1;
    
    if (strstr(fmt, "%s") != NULL) {
	/* special case for %s to print char string */
      nChar += sprintf(&OutputStr[nChar],fmt, p);
    } else {
	n = parsenum(spec, todo, count);

	for (i=0; i<n; i++) {
	    if (type == LDDL_INT8) {
		memcpy(&bval, p + (todo[i] * sizeof(char)), sizeof(char));
		nChar += sprintf(&OutputStr[nChar],fmt, bval);
	    } else if (type == LDDL_INT16) {
		memcpy(&sval, p + (todo[i] * sizeof(short)), sizeof(short));
		nChar += sprintf(&OutputStr[nChar],fmt, sval);
	    } else if (type == LDDL_INT32) {
		memcpy(&lval, p + (todo[i] * sizeof(long)), sizeof(long));
		nChar += sprintf(&OutputStr[nChar],fmt, lval);
	    } else if (type == LDDL_FLOAT32) {
		memcpy(&fval, p + (todo[i] * sizeof(float)), sizeof(float));
		nChar += sprintf(&OutputStr[nChar],fmt, fval);
	    } else if (type == LDDL_FLOAT64) {
		memcpy(&dval, p + (todo[i] * sizeof(double)), sizeof(double));
		nChar += sprintf(&OutputStr[nChar],fmt, dval);
	    } else if (type == LDDL_GROUP) {
		/* ignore - handled by group() */
		Printed_something = 0;
	    } else {
		fprintf(stderr, "unknown type (%d)\n", type);
		Printed_something = 0;
	    }
	}
    }

    if (nullfmt) {
	free (fmt);
    }
    if (nullspec) {
	free (spec);
    }
    free(todo);
}

static char *
estrdup(char *s)
{
    char *p;
    p = strdup(s);
    if (p == NULL) {
	fprintf(stderr, "estrdup: can't malloc\n");
	exit(1);
    }
    return p;
}

static int
pack_parse(char *p, int argc, char**argv)
{
    int a;
    char *fmt = NULL;
    int state = START;

    for (a=0; a < argc; a++) {
	switch (state) {
	    case START:
	    	if (STREQ(argv[a], "print")) {
		    state = PRINT;
		} else if (STREQ(argv[a], ";")) {
		    ;
		} else {
		    fprintf(stderr,
		    	"group: expecting 'print' or ';', got '%s'\n", argv[a]);
		    return -1;
		}
		break;
	    case PRINT:
		if (STREQ(argv[a], ";")) {
		    state = START;
		    packprint(fmt, p);
		} else {
		    fmt = argv[a];
		}
		break;
	    default:
	    	fprintf(stderr, "group: bad state - shouldn't happen\n");
		return -1;
	}
    }

    if (state == PRINT) {
	packprint(fmt,  p);
    }

    return 0;
}

static void
packprint(char *fmt, char *p)
{
    int j;
    int nullfmt = 0;
    unsigned short sval[4];

    if (fmt == NULL) {
	nullfmt = 1;
	fmt = estrdup("%03x ");
    }
    
    /* 4 12-bit ints packed in 3 16-bit words */
    memcpy(sval, p, sizeof(short));
    sval[3] = (sval[0] & 0xf000) >> 12;
    sval[0] &= 0x0fff;
    p += sizeof(short);
    memcpy(sval+1, p, sizeof(short));
    sval[3] |= ((sval[1] & 0xf000) >> 8);
    sval[1] &= 0x0fff;
    p += sizeof(short);
    memcpy(sval+2, p, sizeof(short));
    sval[3] |= ((sval[2] & 0xf000) >> 4);
    sval[2] &= 0x0fff;

    for (j=0; j<4; j++) {
      nChar+=sprintf(&OutputStr[nChar],fmt, sval[j]);
    }

    Printed_something = 1;

    if (nullfmt) {
	free (fmt);
    }
}
