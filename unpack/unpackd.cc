// Main, lddl driven, loop for unpacking 

extern "C"{
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

static void process_evt();
static void monitor_evt(const void *pckStruct_p,unsigned long bufferSize);
static void usage(void);
}

#include <list>
#include "unpackClasses.h"
#include "fileHandler.h"

// Bigest packet size
#define MAX_DATA 9000

static unsigned char *Curp;	/* ptr to current location in Data buffer */
static unsigned char *Data;	/* ptr to start of event's accumulated data */
static unsigned long Evtbytes = 0L;
static unsigned long Evtcnt = 0L;
static int Evt_overflow;	/* nonzero if too much data in evt */
static unsigned long Maxdata = MAX_DATA;		/* max no. of bytes in any event */
static char *Progname;
static int Use_debug_datadir = 0;

// PIDFILE - file containing our process ID
//#define PIDFILE "/tmp/unpackd.pid"

#define DEFAULTDB "anita_test"
#define DEFAULTMONITOR "/tmp/anita_monitor.txt"
#define DEFAULT_UPDATE_STEP 5 // monitor file every 5 seconds

// Monitor file
bool domonitor=false;
char monitorFileName[1024];
int update_step=DEFAULT_UPDATE_STEP;

// Database locale
char dbstring[1024];
PGconn *DBconn=NULL;
pthread_mutex_t dblock;  // Use this to lock connections to database
fileHandler *fHandle=NULL;

// Flag signifying main thread exit
bool quit=false;

// gotsig - for QUIT or INT - causes us to quit
void gotsig(int sig){
  fprintf(stderr,"%s: Arghhh! got signal %d - quitting.\n", Progname, sig);
  fflush(stdout);
  quit=true;
  if(DBconn) PQfinish(DBconn);
  if(fHandle) delete fHandle;
  exit(0);
}

int main(int argc, char *argv[]){
    int c;
    int errflg = 0;
    char fmtfile[1024];  
    extern int optind;
    extern char *optarg;
    bool dodb=false;

    signal(SIGQUIT, gotsig);
    signal(SIGINT, gotsig);

    fmtfile[0]=0;
    Progname = argv[0];
    while ((c = getopt(argc, argv, "Nnf:t:u:b:m::d::")) != EOF) {
      switch (c) {
      case 'b':
	Maxdata = atol(optarg);
	if(Maxdata==0 || Maxdata>MAX_DATA) Maxdata=MAX_DATA;
	break;
      case 'f':
	strcpy(fmtfile,optarg);
	break;
      case 'm':
	domonitor=true;
	strcpy(monitorFileName,optarg?optarg:DEFAULTMONITOR);
	break;
      case 'u':
	update_step=atoi(optarg);
	break;
      case 'd':
	dodb=true;
	sprintf(dbstring,"dbname=%s",optarg?optarg:DEFAULTDB);
	break;
      case 'N':
	/* assume native endianness */
	lddl_set_native(1);
	break;
      case 'n':
	/* assume non-native endianness */
	lddl_set_native(0);
	break;
      case 't':
	fHandle=new fileHandler(optarg);
	break;
      case ':':
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
    
//     {
//       // Store my process ID in PIDFILE. This is so I can be signalled to
//       // start a new file, new run, or quit.
//       FILE *fp;
//       pid_t pid = getpid();
//       fp = fopen(PIDFILE, "w");
//       if (fp == NULL) {
// 	printf("%s: can't open '%s' (%s).\n",
// 	       Progname, PIDFILE, strerror(errno));
//       } else {
// 	fprintf(fp, "%d\n", pid);
// 	fclose(fp);
// 	printf("%s: PID is %d\n", Progname, pid);
//       }
//       fflush(stdout);
//     }

    if(dodb){
      /* Make a connection to the database */
      DBconn = PQconnectdb(dbstring);
      
      /* Check to see that the backend connection was successfully made */
      if (PQstatus(DBconn) != CONNECTION_OK){
	fprintf(stderr, "Connection to database '%s' failed.\n", PQdb(DBconn));
	fprintf(stderr, "%s", PQerrorMessage(DBconn));
	PQfinish(DBconn);
	exit(1);
      }

	// ddm prepare statement
      //          string sqlcmd="PREPARE ddmwv (int, int2, int, int, int2 , int2 , int2 , int2 , int2 , int4, int2[]) AS INSERT INTO wv (nbuf,crc,now,evnum,id,chip,hbwrap,hbstart,hbend,peds,raw) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11);";
      //amirs modification to prepare statement
               string sqlcmd="PREPARE ddmwv (int, int2, int, int, int2 , int2 , int2 , int2 , int2 , int4, real[]) AS INSERT INTO wv (nbuf,crc,now,evnum,id,chip,hbwrap,hbstart,hbend,peds,cal) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11);";
    PQexec(DBconn,sqlcmd.c_str());
    //fprintf(stderr,"ddm prepare\n");
    //fflush(stderr);




    }

    // Allocate working memory
    Data = (unsigned char *)malloc(Maxdata);
    if (Data == NULL) {
	printf( "%s: can't malloc (%s). Exiting.\n",Progname, strerror(errno));
	exit(1);
    }

    lddl_set_buf_size(Maxdata);
    lddl_set_group_func(group);
    lddl_set_evt_func(evt);
    lddl_set_post_evt_func(post_evt);
    lddl_set_node_func(node);
    (void)lddl_search_for_evt_start(1);
    lddl_start(fmtfile, argc-optind, argv+optind, Progname);

    /* printf("%d\n", Evtcnt); */
    fprintf(stderr,"%s: quitting\n", Progname);
    fflush(stderr);

    // Should signal everyone that we're quitting
    quit=true;

    // Close DB connection
    if(DBconn){
      //PQexec(DBconn,"ANALYZE;");  // Re-analyze DB access 
      PQfinish(DBconn);
    }
    // Close file handler
    if(fHandle) delete fHandle;

    exit(0);
}

/* evt - start accumulating the data of a new event */
static int evt(char *name, int argc, char **argv){
//printf("\n\nevent begins.\n");//peng
    Curp = Data;
    Evtbytes = 0L;
    Evt_overflow = 0;
    return 0;
}

/* post_evt - process the accumulated data of the event */
static int post_evt(char *name, int argc, char **argv){
//printf("post event .\n");//peng
    if (Evt_overflow) {
      fprintf(stderr, "%s: event overflow\n", Progname);
      fflush(stderr);
      return 0;
    }
    process_evt();
    return 0;
}

/* group - no-op for now */
static int group(int argc, char **argv, char *group_name, char *evtname){
    return 0;
}

/* node - just copy the data */
static int node(char *inp, char *swapp, int type, int count, int nbytes, int
		argc, char **argv, int native, char *group_name, 
		char *evt_name){
    char *p;
//printf("node: Evetbytes = %d  nbytes = %d\n", Evtbytes, nbytes);//peng
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
	 "\nusage: %s -f fmtfile [-d[dbname]] [-t dir] [-m<filename>] [-u <sec>] [-N] [-n] [file ...]\n",
	 Progname);
  printf( "     -f fmtfile    name of format file\n");
  printf( "     -d[dbname]    name of database to use; default %s\n",DEFAULTDB);
  printf( "     -t dir        name of top directory into which write unpacked files\n");
  printf( "                   Proper subdirectory structure must already exist!\n");
  printf( "     -m<filename>  data rate monitor filename; default %s\n",DEFAULTMONITOR);
  printf( "     -u <sec>      update step for rate monitor file; default %d.\n",DEFAULT_UPDATE_STEP);
  printf( "     -N            data IS in native endianness\n");
  printf( "     -n            data IS NOT in native endianness\n");
  printf( "     file          input data file(s) (def. stdin)\n");
  fflush(stdout);
}

static void process_evt(){
  XferPacket *p=NULL;

//printf("process event.\n");//peng
  try{
    p=new XferPacket(Data,Evtbytes);
    if(p){
      if(DBconn) p->store(DBconn);
      for(vector<Packet>::iterator pp=p->packets.begin();pp!=p->packets.end();++pp){
	if(domonitor) monitor_evt(pp->GetPacketPointer(),pp==p->packets.begin()?Evtbytes:0);
	if(fHandle) fHandle->store(pp->GetPacketPointer());
      }
    }
  }
  catch(Error::BaseError &e){
    cerr << e.message << " " << e.action << endl;
    if(e.info[0]) cerr << e.info << endl;
  }

  if(p) delete p;
}

static void monitor_evt(const void *pckStruct_p,unsigned long bufferSize){
  static bool showme=true;
  static bool first=true;
  static int npck=0;
  static int nerr=0; 
  static int ntype[N_PCKTYPE+1]={0,0,0,0,0,0,0,0,0,0,
				 0,0,0,0,0,0,0,0,0,0,
				 0,0,0,0,0,0,0,0,0,0,
				 0};
  static const char label[N_PCKTYPE+1][10]={
    "HD:      ",
    "WV:      ",
    "PSWV:    ",
    "EPSWV:   ",
    "SURF:    ",
    "PSSURF:  ",
    "ESURF:   ",
    "EPSSURF: ",
    "PED:     ",
    "SURFHK:  ",
    "TURFHK:  ",
    "ADU5_PAT:",
    "ADU5_VTG:",
    "ADU5_SAT:",
    "G12_POS: ",
    "G12_SAT: ",
    "HKD:     ",
    "CMDECHO: ",
    "MON:     ",
    "WAKE_LOS:",
    "WAKE_SIP:",
    "WAKE_CM1:",
    "WAKE_CM2:",
    "SLOW1:   ",
    "SLOW2:   ",
    "SLOWFULL:",
    "ZIPPACK: ",
    "ZIPFILE: ",
    "RUNSTART:",
    "OTHMON:  ",
    "???:     "};
  static int nsize[N_PCKTYPE+1]={
    sizeof(AnitaEventHeader_t),
    sizeof(RawWaveformPacket_t),
    sizeof(PedSubbedWaveformPacket_t),
    sizeof(PedSubbedWaveformPacket_t),
    sizeof(RawWaveformPacket_t),
    sizeof(PedSubbedWaveformPacket_t),
    sizeof(RawWaveformPacket_t),
    sizeof(PedSubbedWaveformPacket_t),
    sizeof(FullLabChipPedStruct_t)/9,  // This structure gets split into 9 packets
    sizeof(FullSurfHkStruct_t),
    sizeof(TurfRateStruct_t),
    sizeof(GpsAdu5PatStruct_t),
    sizeof(GpsAdu5VtgStruct_t),
    sizeof(GpsAdu5SatStruct_t),
    sizeof(GpsG12PosStruct_t),
    sizeof(GpsG12SatStruct_t),
    sizeof(HkDataStruct_t),
    sizeof(CommandEcho_t),
    sizeof(MonitorStruct_t),
    WAKEUP_LOS_BUFFER_SIZE,
    WAKEUP_TDRSS_BUFFER_SIZE,
    WAKEUP_LOW_RATE_BUFFER_SIZE,
    WAKEUP_LOW_RATE_BUFFER_SIZE,
    sizeof(SlowRateType1_t),
    0,
    sizeof(SlowRateFull_t),
    sizeof(ZippedPacket_t),
    sizeof(ZippedFile_t),
    sizeof(RunStart_t),
    sizeof(OtherMonitorStruct_t),
    0};
  static unsigned long long totBytes=0;
  static unsigned long burstBytes=0;
  static unsigned long currBytes=0;
  static time_t startTime=0;
  static time_t lastTime=0;
  static time_t burstTime=0;
  static time_t currStart=0;
  float lat=0,lon=0,alt=0;
  float heading=0,pitch=0,roll=0;
  long tod=0;
  float toti=0,pvv=0,tsbs=0,tplate=0;
  short goodCmd;
  unsigned char cmdStr[128];
  time_t currTime;
  static time_t pcktime=0;
  float currRate,burstRate;
  int type;

  if(first){
    startTime=time(NULL)-1;  // -1 to avoid dt==0 on the first pass
    lastTime=startTime;
    burstTime=startTime;
    currStart=startTime;
    first=false;
  }

  currTime=time(NULL);
  if(currTime>burstTime){
    burstTime=currTime;
    burstRate=burstBytes*8;
    burstBytes=0;
  }

  // Current rate averaged over update step
  if(showme && !(currTime%update_step)){
    currRate=currBytes*8./(currTime-currStart);
    currBytes=0;
    currStart=currTime;
  }

  lastTime=currTime; // Record packet arrival time
  ++npck;
  /* Decode the packet */
  type=pckType(pckStruct_p);
  if(type>=N_PCKTYPE){ // There is error in determining the packet type
    ++ntype[N_PCKTYPE];
    return;
  }

  // Update time if not waveform packet
  if(type!=TYPE_WV            &&
     type!=TYPE_PEDSUB_WV     &&
     type!=TYPE_ENC_WV_PEDSUB &&
     type!=TYPE_SURF          &&
     type!=TYPE_PEDSUB_SURF   &&
     type!=TYPE_ENC_SURF      &&
     type!=TYPE_ENC_SURF_PEDSUB) pcktime=pckTime(pckStruct_p);  

  if(type==TYPE_GPS_ADU5_PAT || type==TYPE_GPS_G12_POS){ // This is GPS pat or pos structure with location info
    FILE *fp;
    pckLoc(pckStruct_p,&lat,&lon,&alt,&tod);
    if(fp=fopen("/tmp/anita_gpsloc.txt","w")){
      fprintf(fp,"%ld %ld %f %f %f\n",pcktime,tod,lat,lon,alt);
      fclose(fp);
    }
  }
  // This might change if use different for orientation
  if(type==TYPE_GPS_ADU5_PAT){ // This is ADU5 pat
    FILE *fp;
    pckOrient(pckStruct_p,&heading,&pitch,&roll);
    if(fp=fopen("/tmp/anita_gpsorient.txt","w")){
      fprintf(fp,"%d %f %f %f\n",pcktime,heading,pitch,roll);
      fclose(fp);
    }
  }
  // Critical housekeeping parameters
  if(type==TYPE_HKD){  // This is HK packet
    if(pckCritical(pckStruct_p,&toti,&pvv,&tsbs,&tplate)){
      FILE *fp;
      if(fp=fopen("/tmp/anita_hkcrit.txt","w")){
	fprintf(fp,"%ld %f %f %f %f\n",pcktime,toti,pvv,tsbs,tplate);
	fclose(fp);
      }
    }
  }
  // Command echo
  if(type==TYPE_CMD_ECHO){  // This is CMD ECHO packet
    FILE *fp;
    pckCmdEcho(pckStruct_p,&goodCmd,cmdStr);
    if(fp=fopen("/tmp/anita_cmdecho.txt","w")){
      fprintf(fp,"%ld %d %s\n",pcktime,goodCmd,cmdStr);
      fclose(fp);
    }
  }
  
  // Update counters
  ++ntype[type];

  totBytes+=bufferSize;
  burstBytes+=bufferSize;
  currBytes+=bufferSize;

  // Display monitoring info
  if(showme && !(currTime%update_step)){
    showme=false;
    FILE *fp;
    if(fp=fopen(monitorFileName,"w")){
      /* Display current time */
      fprintf(fp,"     Current UTC: %s",asctime(gmtime(&currTime)));
      fprintf(fp,"Last packet time: %s",asctime(gmtime(&pcktime)));

      /* Display data rates */
      fprintf(fp,"Burst: %5d b/s\t -- Bits in last second\n",(int)burstRate);
      fprintf(fp,"Curr:  %5d b/s\t -- Bit rate in last %d seconds\n",(int)currRate,update_step);
      fprintf(fp,"Avg:   %5d b/s\t -- Total bit rate since process started\n",(int)(totBytes*8./(currTime-startTime)));
      /* Display packet fractions */
      fprintf(fp,"Total packets:%6d\n",npck);
      if(npck>0){
	for(int i=0;i<=N_PCKTYPE;++i)
	  fprintf(fp,"%s %5.2f %%\t%10d packets  %18lld bytes\n",label[i],(double)ntype[i]/npck*100,ntype[i],(long long)ntype[i]*nsize[i]);
      }
      fclose(fp);
    }
  }

  if(currTime%update_step || (update_step<2 && currTime>currStart)) 
    showme=true;

  return;
}

