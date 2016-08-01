// Main, lddl driven, loop for unpacking using threads to handle multipacket data

#ifdef __linux__
#  define _REENTRANT
#  define _POSIX_SOURCE
#endif // __linux__

extern "C"{
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>	/* usleep */
#include <time.h>
#include <libpq-fe.h>   /* PostgreSQL header */

#include <lddl.h>

static int evt(char *name, int argc, char **argv);
static int group(int argc, char **argv, char *grpname, char *evtname);
static int node(char *inp, char *swapp, int type, int count, int nbytes, int
    argc, char **argv, int native, char *group_name, char *evt_name);
static int post_evt(char *name, int argc, char **argv);

static void process_evt(char *name);
static void usage(void);
}

#include <list>
#include "unpackd_testlite.h"

static unsigned char *Curp;	/* ptr to current location in Data buffer */
static unsigned char *Data;	/* ptr to start of event's accumulated data */
static unsigned long Evtbytes = 0L;
static unsigned long Evtcnt = 0L;
static int Evt_overflow;	/* nonzero if too much data in evt */
static long Maxdata;		/* max no. of bytes in any event */
static char *Progname;
static int Use_debug_datadir = 0;

// Bigest packet size
#define MAX_DATA 1024

// PIDFILE - file containing our process ID
#define PIDFILE "/tmp/lddl_unpackd.pid"

// Database locale
#define DBSTRING "dbname=testlite"
PGconn *DBconn;
pthread_mutex_t dblock;  // Use this to lock connections to database

// Thread accounting 
list<pthread_t> threads_started; // list of threads started
list<pthread_t> threads_ended; // list of threads that reported their exit
pthread_mutex_t threadlock=PTHREAD_MUTEX_INITIALIZER;
#define MAX_THREAD_TRY 10  // Number of tries to start a thread before giving up
#define THREAD_SLEEP_CYCLE 1  // Amount of seconds to sleep in a thread
#define THREAD_TIMEOUT 5      // Maximum wait time for WvPacket to fill

// Hash table keeping track of active threads working on multipacket data
hash_map<int,WvPacket*> wvThreadMap;
pthread_mutex_t wvmaplock=PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP; // Lock access to thread map
// Error structure
struct MapInsertionError{
  int evnum;
  MapInsertionError(int n){evnum=n;}
};

/* Signaling variables need to suspend parent thread until new WvPacket object is
   generated and ready to go. */
bool WvPacketReady;
pthread_mutex_t WvReadyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t WvReadyCond = PTHREAD_COND_INITIALIZER;

struct pt_data_struct{
  unsigned char data[MAX_DATA];
  int n;
  pt_data_struct(unsigned char *d,int N){
    memcpy(data,d,N);
    n=N;
  }   
};

// Flag signifying main thread exit
bool quit=false;

// gotsig - for QUIT or INT - causes us to quit
void gotsig(int sig){
    printf("%s: Arghhh! got signal %d - quitting.\n", Progname, sig);
    fflush(stdout);
    quit=true;
    PQfinish(DBconn);
    exit(0);
}

// Function in which waveform assembly thread lives
void *eventd(void *arg){
  // Convert argument to its proper form
  pt_data_struct *pt_data=(pt_data_struct*)arg;

  // Record when we started processing this event 
  time_t start_time=time(NULL);

  // Define variables we need
  pair<hash_map<int,WvPacket*>::iterator,bool> insertionResult;
  WvPacket *wv_p;
  bool success=false;
  bool abort=false;

  // Create new packet
  try{
    wv_p=new WvPacket(pt_data->data,pt_data->n);

    // Place packet pointer into hash_map for access by parent thread
    pthread_mutex_lock(&wvmaplock);
    insertionResult=wvThreadMap.insert(make_pair(wv_p->getEvnum(),wv_p));
    pthread_mutex_unlock(&wvmaplock);
    if(!insertionResult.second){ // Insertion failed, e.g. due to duplicate entry 
      //cerr << "Hash_map has " << wvThreadMap.size() << " elements. ";
      //for(hash_map<int,WvPacket*>::iterator bung=wvThreadMap.begin();
      //bung!=wvThreadMap.end();++bung)
      //cerr << bung->first << " ";
      //      cerr << endl;
      throw MapInsertionError((insertionResult.first)->first);
    }
  }
  catch(MapInsertionError &e){ // Catch map insertion error
    fprintf(stderr,"Unable to insert evnum %d into WvPacket hash_map. Giving up!\n",
	    e.evnum);
    delete wv_p;
    abort=true;
  }
  catch(Error::BaseError &e){  // Catch packet object errors creation 
    fprintf(stderr,"In thread: %s %s\n",e.message,e.action);
    if(e.info[0]) fprintf(stderr,"%s\n",e.info);
    abort=true;
  }
  
  // Signal that new object is ready
  pthread_mutex_lock(&WvReadyMutex);
  WvPacketReady=true;
  pthread_cond_signal(&WvReadyCond);
  pthread_mutex_unlock(&WvReadyMutex);

  if(abort){
    pthread_mutex_lock(&threadlock);
    threads_ended.push_back(pthread_self());
    pthread_mutex_unlock(&threadlock);
    pthread_exit((void*)&success);
  }

  // Loop that checks whether we have all data for this event
  while(!quit && !wv_p->done() && (time(NULL)-start_time)<THREAD_TIMEOUT) 
    sleep(THREAD_SLEEP_CYCLE);

  // All data arrived or we gave up, so store packet and exit
  pthread_mutex_lock(&dblock);
  success=wv_p->store(DBconn);
  pthread_mutex_unlock(&dblock);

  // Remove object from packet hash
  pthread_mutex_lock(&wvmaplock);
  wvThreadMap.erase(insertionResult.first);
  pthread_mutex_unlock(&wvmaplock);
  // Delete opacket object
  delete(wv_p);

  // Exit thread with success
  pthread_mutex_lock(&threadlock);
  threads_ended.push_back(pthread_self());
  pthread_mutex_unlock(&threadlock);
  pthread_exit((void*)&success);
}

// Cleanup resources of completed threads
void cleanup(){
  int err,*exit_status;
  pthread_mutex_lock(&threadlock);
  for(list<pthread_t>::iterator t_p=threads_ended.begin(); 
      t_p!=threads_ended.end();++t_p){
    /* join the thread, it should have terminated by now */
    err=pthread_join(*t_p,(void **) &exit_status);
    //err=pthread_join(*t_p,NULL);
    if(err) fprintf(stderr,"Error pthread_join: %s\n",strerror(err));
    if (*exit_status==0) fprintf(stderr,"Thread terminated abnormally\n");
    /* Regardless of success, we remove this thread from the lists 
       (see man page as to why this is valid behavior, it could cause some 
       memory leaks but I see no way around that, PM 2005-07-18) */
    threads_started.erase(find(threads_started.begin(),threads_started.end(),*t_p));
  }
  threads_ended.clear();
  pthread_mutex_unlock(&threadlock);
  return;
}


// Function that launches thread managing new WvPacket object 
void launchThread(unsigned char *data,int n){
  // Place hold on main thread execution
  pthread_mutex_lock(&WvReadyMutex);
  WvPacketReady=false;
  pthread_mutex_unlock(&WvReadyMutex);
  // Launch thread to handle transients associated with this header
  pthread_t new_thread;
  pt_data_struct *pt_data=new pt_data_struct(data,n);
  pthread_attr_t pt_attr;
  pthread_attr_init(&pt_attr);
  //	pthread_attr_setdetachstate(&pt_attr,PTHREAD_CREATE_DETACHED);
  int err;
  int trycnt=0;
  while(err=pthread_create(&new_thread,       /* thread struct             */
			   &pt_attr,               /* default thread attributes */
			   eventd,                 /* start routine             */
			   (void*)pt_data)){       /* arg to routine            */
    // We failed to create thread, check why
    switch(err){
    case EAGAIN: // Too many threads active
    case ENOMEM: // Can't allocate memory
      cleanup();       // join threads that ended
      sleep(THREAD_SLEEP_CYCLE); // Wait a bit before trying again
      break;
    default: // Something very wrong, should never happen 
      fprintf(stderr,"Error pthread_create(%d): %s\n",err,strerror(err));
      throw Error:: PthreadsError(Data,Evtbytes);
    }
    // Check if maximum number of tries excided
    if(++trycnt>=MAX_THREAD_TRY) throw Error:: PthreadsError(Data,Evtbytes); 
  }
  // Record new thread that started
  pthread_mutex_lock(&threadlock);
  threads_started.push_back(new_thread);
  pthread_mutex_unlock(&threadlock);

  // Now wait until new WvPacket object is ready
  pthread_mutex_lock(&WvReadyMutex);
  while(!WvPacketReady) pthread_cond_wait(&WvReadyCond, &WvReadyMutex);
  pthread_mutex_unlock(&WvReadyMutex);
  
  return;
}

int main(int argc, char *argv[]){
    int c;
    int errflg = 0;
    char *fmtfile = NULL;
    extern int optind;
    extern char *optarg;
    int ret;

    signal(SIGQUIT, gotsig);
    signal(SIGINT, gotsig);

    Progname = argv[0];

    while ((c = getopt(argc, argv, "f:Nn")) != EOF) {
	switch (c) {
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

    /* Make a connection to the database */
    DBconn = PQconnectdb(DBSTRING);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(DBconn) != CONNECTION_OK){
      fprintf(stderr, "Connection to database '%s' failed.\n", PQdb(DBconn));
      fprintf(stderr, "%s", PQerrorMessage(DBconn));
      PQfinish(DBconn);
      exit(1);
    }

    // Allocate working memory
    Maxdata = MAX_DATA;
    Data = (unsigned char *)malloc(Maxdata);
    if (Data == NULL) {
	printf( "%s: can't malloc (%s). Exiting.\n",Progname, strerror(errno));
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
    printf( "%s: quitting\n", Progname);
    fflush(stdout);

    // Should signal everyone that we're quitting
    quit=true;

    // Wait for all remaining threads handling WvPackets to finish
    for(list<pthread_t>::iterator t_p=threads_started.begin(); 
	t_p!=threads_started.end();++t_p){
      int errcode,*exit_status;
      /* wait for thread to terminate */
      if (pthread_join(*t_p,(void **) &exit_status))
	fprintf(stderr,"Error pthread_join: %s",strerror(errcode));
      /* check thread's exit status and release its resources */
      if (*exit_status==0) fprintf(stderr,"Thread terminated abnormally\n");
    }

    // Close DB connection
    PQfinish(DBconn);

    exit(0);
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

static void usage(void){
  printf(
	 "\nusage: %s -f fmtfile [-N] [-n] [file ...]\n",
	 Progname);
  printf( "     -f fmtfile    name of format file\n");
  printf( "     -N            data IS in native endianness\n");
  printf( "     -n            data IS NOT in native endianness\n");
  printf( "     file          input data file(s) (def. stdin)\n");
  fflush(stdout);
}

static void process_evt(char *name){
  Packet *p=NULL;
  unsigned char type=Data[11];

  try{
    int errcode;
    switch(type){
    case 0x00:
      CmdPacket *cmd_p=new CmdPacket(Data,Evtbytes);
      p=static_cast<Packet*>(cmd_p);
      break;
    case 0x10:
      Hk1Packet *hk1_p=new Hk1Packet(Data,Evtbytes);
      p=static_cast<Packet*>(hk1_p);
      break;
    case 0x11:
      Hk2Packet *hk2_p=new Hk2Packet(Data,Evtbytes);
      p=static_cast<Packet*>(hk2_p);
      break;
    case 0x13:
      Hk4Packet *hk4_p=new Hk4Packet(Data,Evtbytes);
      p=static_cast<Packet*>(hk4_p);
      break;
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:{
      // can have any other trig level, I guess...
      HdPacket *hd_p=new HdPacket(Data,Evtbytes);
      p=static_cast<Packet*>(hd_p);
      // See if there is a thread handling this event already
      pthread_mutex_lock(&wvmaplock);
      //cerr << "Found header event " << hd_p->getEvnum() << endl;
      hash_map<int,WvPacket*>::const_iterator wv_p=wvThreadMap.find(hd_p->getEvnum());
      if(wv_p != wvThreadMap.end()){  // There is a thread handling this event number
	wv_p->second->insertHeader(Data,Evtbytes);
	pthread_mutex_unlock(&wvmaplock);
      }else{ // No threat exists, so create one
	pthread_mutex_unlock(&wvmaplock);
	launchThread(Data,Evtbytes);
      }
      break;
    }
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:{
      p=NULL;
      // See if there is a thread handling this event already
      pthread_mutex_lock(&wvmaplock);
      //cerr << "Found transient event " << *((int*)(Data+12)) << endl;
      hash_map<int,WvPacket*>::const_iterator wv_p=wvThreadMap.find(*((int*)(Data+12)));
      if(wv_p != wvThreadMap.end()){  // There is a thread handling this event number
	//cerr << "Sending " << *((int*)(Data+12)) << " to existing thread\n";
	wv_p->second->insertTransient(Data,Evtbytes,false);
	pthread_mutex_unlock(&wvmaplock);
      }else{ // No threat exists, so create one
	//cerr << "Creating new thread for  " << *((int*)(Data+12)) << endl;
	pthread_mutex_unlock(&wvmaplock);
	launchThread(Data,Evtbytes);
      }
      break;
    }
    case 0x1f:
      p=NULL;
      break;
    default:
      throw Error::UnmatchedPacketType(Data,Evtbytes);
    }
    pthread_mutex_lock(&dblock);
    if(p && !p->store(DBconn))
      fprintf(stderr,"Failure to store packet! Ignoring it!\n");
    pthread_mutex_unlock(&dblock);
  }
  catch(Error::BaseError &e){
    fprintf(stderr,"%s %s\n",e.message,e.action);
    if(e.info[0]) fprintf(stderr,"%s\n",e.info);
    // In case we got here with locked wvmaplock
    pthread_mutex_unlock(&wvmaplock);
  }
  
  if(p) delete p;
}
