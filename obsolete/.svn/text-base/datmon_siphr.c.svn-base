/*  Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu> 

  This file is free software; as a special exception the author gives
  unlimited permission to copy and/or distribute it, with or without 
  modifications, as long as this notice is preserved. 

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY, to the extent permitted by law; without even the 
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   ANITA fast TDRSS data pipe monitoring tool   

   $Header$
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
#include "shmem_siphr.h"
static int nBuf=0;
static unsigned long Shmem_Length[NBUF];

/* Pausing/quitting control */
static int mypause=0;
static int run=1;

/* Toggles pause state */
void pauseIt(int sig){
  mypause = !mypause;
}

/* Quit */
void quit(int sig){
  run=0;
}

void usage(char *progname){
  printf("\nusage: %s [-s <n>] [-h]\n",progname);
  printf( "     -s <n>     Start wiht buffer <n>. Default 0\n");
  printf( "     -h         This help\n");
  fflush(stdout);
}

int main(int argc, char *argv[]){	
  WINDOW *infowin,*statwin,*dumpwin;
  int ch,i,row,col,srow;
  time_t pcktime=0;
  int badpck=0,npck=0,nerr=0; /* Packet counters */
  int ntype[6]={0,0,0,0,0,0};
  int ret,bytes;
  unsigned long n,last_n=0,totBytes=0,burstBytes=0,currBytes=0;
  unsigned char pck[MAX_DATA];
  time_t startTime,lastTime,burstTime,currStart;
  double lat=0,lon=0,alt=0;
  float currRate;
  extern int optind;
  extern char *optarg;
  int c,errflg=0;

  while ((c = getopt(argc, argv, "s:h")) != EOF) {
    switch (c) {
    case 's':
      nBuf = atoi(optarg);
      if(nBuf>=NBUF) nBuf=0;
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

  signal(SIGTSTP,pauseIt);
  signal(SIGQUIT, quit);
  signal(SIGINT, quit);

  /* Setup shared memory */
  for(i=0;i<NBUF;++i) Shmem_Length[i]=MAX_DATA;
  if(shmem_init(SHMEM_SEM_FILE, SHMEM_KEY_DATAPRINT, NBUF, Shmem_Length, 0)){
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
  infowin=newwin(4,COLS-21,0,0);
  wborder(infowin, ' ', ' ', ' ','-',' ',' ','-','-');
  wnoutrefresh(infowin);
  
  /* stat window */
  statwin=newwin(LINES-1,21,0,COLS-21);
  wborder(statwin, '|', ' ', ' ','-','|',' ','-','-');
  mvwaddstr(statwin,0,1,"Data rate");
  mvwaddstr(statwin,5,1,"Packet fractions");
  wnoutrefresh(statwin);

  /* data dump window takes, remaining space, except for last 2 lines */
  dumpwin=newwin(LINES-6,COLS-21,4,0);
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
	int type;
	unsigned char crcNew,crcOld;
	int nData;             // Length of packet data
	void *pckStruct_p;     // Pointer to packet data structure

	lastTime=currTime; // Record packet arrival time
	/* Line scrollings */
	scroll(dumpwin);
	wmove(dumpwin,row-1,0);
	++npck;
	/* Decode the packet */
	if(pckStruct_p=depackSIPhr(pck,n,&nData)){
	  int dummy;
	  /* Check crc */
	  wrapperSIPhr(pck,n,&dummy,&crcOld);
	  crcNew=crc_char(pckStruct_p,nData);
	  if(crcNew!=crcOld) ++badpck;
	  type=pckType(pckStruct_p);
	  pcktime=pckTime(pckStruct_p);
	  if(type==2){ // This is GPSD Pat stracture with location info
	    FILE *fp;
	    pckLoc(pckStruct_p,&lat,&lon,&alt);
	    if(fp=fopen("/tmp/gpsloc.txt","w")){
	      fprintf(fp,"%d %f %f %f\n",pcktime,lat,lon,alt);
	      fclose(fp);
	    }
	  }
	  /* Packet display */
	  wprintw(dumpwin,pckStr(pckStruct_p));
	  //if(!mypause) mvwprintw(statwin,18,1,"%0x %0x",crcOld,crcNew);
	  //if(!mypause) mvwprintw(statwin,18,1,"%d",pcktime);
	  ++ntype[type];
	  totBytes+=n;
	  burstBytes+=n;
	  currBytes+=n;
	}else{ // Failed to depack
	  ++nerr;
	}
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
      mvwprintw(statwin,srow++,1,"PAT:  %5.1f %%",(double)ntype[2]/npck*100);
      mvwprintw(statwin,srow++,1,"SAT:  %5.1f %%",(double)ntype[3]/npck*100);
      mvwprintw(statwin,srow++,1,"HK:   %5.1f %%",(double)ntype[4]/npck*100);
      mvwprintw(statwin,srow++,1,"???:  %5.1f %%",(double)ntype[5]/npck*100);
      srow++;
      /* Display xfer errors */
      mvwprintw(statwin,srow++,1,"Xfer errors: %d",badpck);
      mvwprintw(statwin,srow++,1,"            %5.3f %%",(double)badpck/npck*100);
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

