/* fast_tdrss_lddl.c - parse and store the fast_tdrss data.
 *
 * Marty Olevitch, May 01
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>	/* usleep */
#include <time.h>

#include <lddl.h>
#include "shmem.h"


static int evt(char *name, int argc, char **argv);
static int group(int argc, char **argv, char *grpname, char *evtname);
static int node(char *inp, char *swapp, int type, int count, int nbytes, int
    argc, char **argv, int native, char *group_name, char *evt_name);
static int post_evt(char *name, int argc, char **argv);

static void process_evt(char *name);
static void usage(void);

static unsigned char *Curp;	/* ptr to current location in Data buffer */
static unsigned char *Data;	/* ptr to start of event's accumulated data */
static unsigned long Evtbytes = 0L;
static unsigned long Evtcnt = 0L;
static int Evt_overflow;	/* nonzero if too much data in evt */
static long Maxdata;		/* max no. of bytes in any event */
static char *Progname;
static int Use_debug_datadir = 0;
static struct timespec nodeDelay={0,0}; // ns

/* Shared memory definitions */
#include "shmem_def.h"
static int nBuf=0;
static unsigned long Shmem_Length[NBUF];

// PIDFILE - file containing our process ID
#define PIDFILE "/tmp/shmem_tdrss.pid"

// gotsig - for QUIT or INT - causes us to quit
void
gotsig(int sig)
{
    printf("%s: Arghhh! got signal %d - quitting.\n", Progname, sig);
    fflush(stdout);
    shmem_unlink();
    exit(0);
}

// restart_shmem - on signal ALRM, attempt to restart shmem.
void
restart_shmem(int sig)
{
    int ret;
    ret = shmem_init(SHMEM_SEM_FILE,SHMEM_KEY_DATAPRINT, NBUF, Shmem_Length, 1);
    if (ret) {
	printf( "%s: ERROR! bad shmem_init (%s). \n",
	    Progname, shmem_strerror());
	exit(1);
    } else {
	printf( "%s: successful shmem_init.\n", Progname);
    }
    fflush(stdout);
}

int
main(int argc, char *argv[])
{
    int c;
    int errflg = 0;
    char *fmtfile = NULL;
    extern int optind;
    extern char *optarg;
    int ret;

    signal(SIGALRM, restart_shmem);
    signal(SIGQUIT, gotsig);
    signal(SIGINT, gotsig);

    Progname = argv[0];

    while ((c = getopt(argc, argv, "f:d:Nn")) != EOF) {
	switch (c) {
	    case 'f':
	    	fmtfile = optarg;
		break;
   	    case 'd':
	      nodeDelay.tv_nsec = atoi(optarg);
	      break;
	    case 'N':
	    	/* assume native endianness */
		lddl_set_native(1);
		break;
	    case 'n':
	    	/* assume non-native endianness */
		lddl_set_native(0);
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

    {
	int i;
	for (i=0; i<NBUF; i++) {
	    Shmem_Length[i] = MAX_DATA;
	}
    }

    restart_shmem(0);

    {
	// Store my process ID in PIDFILE. This is so I can be signalled to
	// start a new file, new run, or quit.
	FILE *fp;
	pid_t pid = getpid();
	fp = fopen(PIDFILE, "w");
	if (fp == NULL) {
	    printf("%s: can't open '%s' (%s).\n",
		    Progname, PIDFILE, strerror(errno));
	} else {
	    fprintf(fp, "%d\n", pid);
	    fclose(fp);
	    printf("%s: PID is %d\n", Progname, pid);
	}
	fflush(stdout);
    }

    Maxdata = MAX_DATA;
    Data = (char *)malloc(Maxdata);
    if (Data == NULL) {
	printf( "%s: can't malloc (%s). Exiting.\n",
	    Progname, strerror(errno));
	shmem_unlink();
	exit(1);
    }

    lddl_set_buf_size(MAX_DATA);
    lddl_set_group_func(group);
    lddl_set_evt_func(evt);
    lddl_set_post_evt_func(post_evt);
    lddl_set_node_func(node);
    (void)lddl_search_for_evt_start(1);
    lddl_start(fmtfile, argc-optind, argv+optind, Progname);
    /* printf("%d\n", Evtcnt); */
    shmem_unlink();
    printf( "%s: quitting\n", Progname);
    fflush(stdout);
    return 0;
}

/* evt - start accumulating the data of a new event */
static int 
evt(char *name, int argc, char **argv)
{
    Curp = Data;
    Evtbytes = 0L;
    Evt_overflow = 0;
    return 0;
}

/* post_evt - process the accumulated data of the event */
static int 
post_evt(char *name, int argc, char **argv)
{
    if (Evt_overflow) {
	printf( "%s: event overflow\n", Progname);
	fflush(stdout);
	return 0;
    }
    process_evt(name);
    return 0;
}

/* group - no-op for now */
static int
group(int argc, char **argv, char *group_name, char *evtname)
{
    return 0;
}

/* node - just copy the data */
static int
node(char *inp, char *swapp, int type, int count, int nbytes, int
    argc, char **argv, int native, char *group_name, char *evt_name)
{
    char *p;

    if(nodeDelay.tv_nsec) nanosleep(&nodeDelay,NULL);

    if (Evtbytes + nbytes > Maxdata) {
	Evt_overflow = 1;
    }
    if (Evt_overflow) {
	return 0;
    }
    p = native ? inp : swapp;
    memcpy(Curp, p, nbytes);
    Curp += nbytes;
    Evtbytes += nbytes;
    return 0;
}

static void
usage(void)
{
    printf(
    	"\nusage: %s -f fmtfile [-N] [-n] [-d <n>][file ...]\n",
    	Progname);
    printf( "     -f fmtfile    name of format file\n");
    printf( "     -N            data IS in native endianness\n");
    printf( "     -n            data IS NOT in native endianness\n");
    printf( "     -d <n>        delay node processing by n us\n");
    printf( "     file          input data file(s) (def. stdin)\n");
    fflush(stdout);
}

static void
process_evt(char *name)
{
    static unsigned long N = 0L;
    int ret;
    unsigned long evtno;

    if (Evtbytes == 0) {
	return;
    }

    //     memcpy((char *)&evtno, Data+2, sizeof(long));	/* assuming native */ 

/*     printf( "\t\t%s: %s %d evtno %lu (%lu)\t", */
/*     		Progname, name, Evtbytes, evtno, N++); */

    {
        int i;
	for (i=0; i<5; i++) {
	    ret = shmem_write(nBuf, Data, (unsigned long)Evtbytes);
	    if (ret == 0) {
		break;
	    }
	    usleep(10L);
	}
	/* if (ret == -2) { */
/* 	    printf( "[locked]\n"); */
/* 	} else if (ret == -1) { */
/* 	    printf( "[error]\n"); */
/* 	} else { */
/* 	    printf( "\n"); */
/* 	} */
	if(++nBuf>=NBUF) nBuf=0;
    }
}
