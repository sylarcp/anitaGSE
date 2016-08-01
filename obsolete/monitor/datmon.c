/*  Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu> 

  This file is free software; as a special exception the author gives
  unlimited permission to copy and/or distribute it, with or without 
  modifications, as long as this notice is preserved. 

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY, to the extent permitted by law; without even the 
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   ANITA LOS data pipe monitoring tool   

   $Header: /cvsroot/anitacode/gse/monitor/datmon_raw.c,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $ 
*/

#include <stdio.h>
#include <ncurses.h> 
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>

#include "../lib/anitaGSE.h"

/* Shared memory definitions */
#include "shmem.h"
#include "shmem_def.h"
static int nBuf=0;
static unsigned long Shmem_Length[NBUF];
typedef enum{InLOS,InSIPHR,InSIPLR,InSIPIR} Interface_t;
static Interface_t interface=InLOS;

/* Pausing/quitting control */
static int mypause=1;
static int run=1;

/* Toggles pause state */
static void pauseIt(int sig){
  mypause = !mypause;
}

/* Quit */
static void quit(int sig){
  run=0;
}

void usage(char *progname){
  printf("\nusage: %s [-i <n>] [-s <n>] [-h]\n",progname);
  printf("     -i <n>     Interface; LOS = 0, SIPHR = 1, SIPLR = 2, SIPIR = 3.\n");
  printf("                Default LOS\n");
  printf("     -s <n>     Start with buffer <n>. Default 0\n");
  printf("     -h         This help\n");
  fflush(stdout);
}

int main(int argc, char *argv[]){	
  WINDOW *infowin,*statwin,*dumpwin;
  int ch,i,row,col,srow;
  time_t pcktime=0;
  int badpck=0,npck=0,nerr=0; /* Packet counters */
  int ntype[N_PCKTYPE+1]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int ret,bytes;
  unsigned long n,last_n=0,totBytes=0,burstBytes=0,currBytes=0;
  unsigned char pck[8192];
  time_t startTime,lastTime,burstTime,currStart;
  double lat=0,lon=0,alt=0;
  double az_seavey=0,az_true=0,bh=0;
  float currRate;
  extern int optind;
  extern char *optarg;
  int c,errflg=0;
  // Interface variables
  int max_data;
  long shmem_key; 
  char sem_file[1024];

  while ((c = getopt(argc, argv, "i:s:h")) != EOF) {
    switch (c) {
    case 's':
      nBuf = atoi(optarg);
      if(nBuf>=NBUF) nBuf=0;
      break;
    case 'i':
      interface = atoi(optarg);
      break;
    case 'h':
    case '?':
    default:
      errflg++;
      break;
    }
  }

  if (errflg) {
    usage(argv[0]);
    exit(1);
  }

  // Setup interface connections
  switch(interface){
  case InLOS:
    max_data = LOS_MAX_DATA;
    shmem_key = LOS_SHMEM_KEY;
    sprintf(sem_file,LOS_SHMEM_SEM_FILE);
    break;
  case InSIPHR:
    max_data = SIPHR_MAX_DATA;
    shmem_key = SIPHR_SHMEM_KEY;
    sprintf(sem_file,SIPHR_SHMEM_SEM_FILE);
    break;
  case InSIPLR:
    max_data = SIPLR_MAX_DATA;
    shmem_key = SIPLR_SHMEM_KEY;
    sprintf(sem_file,SIPLR_SHMEM_SEM_FILE);
    break;
  case InSIPIR:
    max_data = SIPIR_MAX_DATA;
    shmem_key = SIPIR_SHMEM_KEY;
    sprintf(sem_file,SIPIR_SHMEM_SEM_FILE);
    break;
  default:
    usage(argv[0]);
    exit(1);
  }

  signal(SIGTSTP,pauseIt);
  signal(SIGQUIT, quit);
  signal(SIGINT, quit);

  /* Setup shared memory */
  for(i=0;i<NBUF;++i) Shmem_Length[i]=max_data;
  if(shmem_init(sem_file, shmem_key, NBUF, Shmem_Length, 0)){
    fprintf(stderr, "%s: ERROR %s\n", argv[0], shmem_strerror());
    exit(1);
  }
  
  initscr();			/* Start curses mode 		*/
  cbreak();			/* Line buffering disabled, Pass on
				 * everty thing to me 		*/
  //keypad(stdscr, TRUE);		/* I need that nifty F1 	*/
  noecho();
  curs_set(0);                  /* Invisible cursor */

  refresh();

  /* info window */
  infowin=newwin(5,COLS-21,0,0);
  wborder(infowin, ' ', ' ', ' ','-',' ',' ','-','-');
  wnoutrefresh(infowin);
  
  /* stat window */
  statwin=newwin(LINES-1,21,0,COLS-21);
  wborder(statwin, '|', ' ', ' ','-','|',' ','-','-');
  mvwaddstr(statwin,0,1,"Data rate");
  mvwaddstr(statwin,5,1,"Packet fractions");
  wnoutrefresh(statwin);

  /* data dump window takes, remaining space, except for last 2 lines */
  dumpwin=newwin(LINES-7,COLS-21,5,0);
  //wborder(dumpwin, ' ', ' ', ' ','-',' ',' ','-','-');
  getmaxyx(dumpwin,row,col); 
  scrollok(dumpwin,1);
  wnoutrefresh(dumpwin);

  /* Usage info */
  move(LINES-2,0);
  for(i=0;i<COLS-21;++i) addch('-');
  mvprintw(LINES-1,0,"^Z-pause/resume ^C-quit");
  refresh();

  doupdate();

  startTime=time(NULL)-1;  // -1 to avoid 0 dt on the first pass
  lastTime=startTime;
  burstTime=startTime;
  currStart=startTime;
  while(run){
    // char filename[128];

    time_t currTime=time(NULL);
    if(currTime-lastTime>1){
      burstTime=currTime-1;
      burstBytes=0;
    }

    if(!(currTime%5)  && currTime-lastTime>0){
      if(currStart<currTime){
	currRate=currBytes*8./5;
	currBytes=0;
	currStart=currTime;
      }
    }

    /* Get new packet */
    ret = shmem_read(nBuf, 0, pck, &n, NULL);
    if (ret == -1) {
      fprintf(stderr, "%s: ERROR %s\n", argv[0], shmem_strerror());
      shmem_unlink();
      exit(1);
    } else if (ret == 0) {  // There was new packet
      if (n > 0) { // New data read
	int type,size;
	//	unsigned short crcNew,crcOld;
	int nData=0;           // Length of packet data processed
	void *pckStruct_p;     // Pointer to packet data structure

	lastTime=currTime; // Record packet arrival time
	/* Line scrollings */
	scroll(dumpwin);
	wmove(dumpwin,row-1,0);
	++npck;
	/* Decode the packet, there can be many data structures in a single packet */
	while(nData<=n){
	  pckStruct_p=(void*)(&pck[nData]);
	  type=pckType(pckStruct_p);
	  size=pckSize(pckStruct_p);
	  if(!size){ // There is error in determining the data size
	    ++nerr;
	    break;
	  }
	  nData+=size;
	  pcktime=pckTime(pckStruct_p);
	  if(type==7 || type==10){ // This is GPS pat or pos structure with location info
	    FILE *fp;
	    pckLoc(pckStruct_p,&lat,&lon,&alt);
	    if(fp=fopen("/tmp/gpsloc.txt","w")){
	      fprintf(fp,"%d %f %f %f\n",pcktime,lat,lon,alt);
	      fclose(fp);
	    }
	  }
	  // This might change if use different for orientation
	  if(type==12){ // This is HK structure with magnetic info data
	    pckOrient(pckStruct_p,&az_seavey,&az_true,&bh);
	  }
	  /* Packet display */
	  wprintw(dumpwin,pckStr(pckStruct_p));
	  //if(!mypause) mvwprintw(statwin,18,1,"%0x %0x",crcOld,crcNew);
	  //if(!mypause) mvwprintw(statwin,18,1,"%d",pcktime);
	  // Update counters
	  ++ntype[type];
	}
	totBytes+=n;
	burstBytes+=n;
	currBytes+=n;
      }
      if(++nBuf>=NBUF) nBuf=0;
    } else if (ret == -2) { // Memory locked, try this one again
      //scroll(dumpwin);
      //mvwprintw(dumpwin,row-2,0,"Buffer locked %d",nBuf);
    } else {
      fprintf(stderr, "Bad return code!!! (%d)\n", ret);
      shmem_unlink();
      exit(1);
    }

    if(!mypause) wnoutrefresh(dumpwin);  /* Show scroll if not paused */

    /* Display current time */
    mvwprintw(infowin,0,0,"             UTC: %s",asctime(gmtime(&currTime)));
    if(pcktime)
      wprintw(infowin,"Last packet time: %s",asctime(gmtime(&pcktime)));
    wprintw(infowin,"Last position: %.2f deg %.2f deg %.1f m\n",lat,lon,alt);
    wprintw(infowin,"Last orientation: %.2f deg %.2f deg %.3f G\n",az_seavey,az_true,bh);
	    
    wnoutrefresh(infowin);

    /* Display data rates */
    srow=1;
    mvwprintw(statwin,srow++,1,"Burst: %5d kb/s",(int)(burstBytes*8./(currTime-burstTime)));
    mvwprintw(statwin,srow++,1,"Avg:   %5d kb/s",(int)(totBytes*8./(currTime-startTime)));
    mvwprintw(statwin,srow++,1,"Curr:  %5d kb/s",(int)currRate);
    srow++;
    /* Display packet fractions */
    mvwprintw(statwin,srow++,1,"Total packets:%6d",npck);
    if(npck>0){
      mvwprintw(statwin,srow++,1,"HD:   %5.1f %%",(double)ntype[0]/npck*100);
      mvwprintw(statwin,srow++,1,"WV:   %5.1f %%",(double)ntype[1]/npck*100);
      mvwprintw(statwin,srow++,1,"SURF: %5.1f %%",(double)ntype[2]/npck*100);
      mvwprintw(statwin,srow++,1,"HKS:  %5.1f %%",(double)ntype[3]/npck*100);
      mvwprintw(statwin,srow++,1,"TURF: %5.1f %%",(double)ntype[4]/npck*100);
      mvwprintw(statwin,srow++,1,"EWV:  %5.1f %%",(double)ntype[5]/npck*100);
      mvwprintw(statwin,srow++,1,"ESURF:%5.1f %%",(double)ntype[6]/npck*100);
      mvwprintw(statwin,srow++,1,"PAT:  %5.1f %%",(double)ntype[7]/npck*100);
      mvwprintw(statwin,srow++,1,"VTG:  %5.1f %%",(double)ntype[8]/npck*100);
      mvwprintw(statwin,srow++,1,"ASAT: %5.1f %%",(double)ntype[9]/npck*100);
      mvwprintw(statwin,srow++,1,"POS:  %5.1f %%",(double)ntype[10]/npck*100);
      mvwprintw(statwin,srow++,1,"SAT:  %5.1f %%",(double)ntype[11]/npck*100);
      mvwprintw(statwin,srow++,1,"HK:   %5.1f %%",(double)ntype[12]/npck*100);
      mvwprintw(statwin,srow++,1,"CMD:  %5.1f %%",(double)ntype[13]/npck*100);
      mvwprintw(statwin,srow++,1,"MON:  %5.1f %%",(double)ntype[14]/npck*100);
      mvwprintw(statwin,srow++,1,"???:  %5.1f %%",(double)ntype[15]/npck*100);
      srow++;
      /* Display xfer errors */
      //      mvwprintw(statwin,srow++,1,"Xfer errors: %d",badpck);
      //mvwprintw(statwin,srow++,1,"            %5.3f %%",(double)badpck/npck*100);
      mvwprintw(statwin,srow++,1,"Dpck errors: %d",nerr);
      mvwprintw(statwin,srow++,1,"            %5.3f %%",(double)nerr/npck*100);
    }

    wnoutrefresh(statwin);
    doupdate();
  }
  
  endwin();			/* End curses mode		  */

  shmem_unlink();
  return 0;
}

