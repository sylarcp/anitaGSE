/*  Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu> 

  This file is free software; as a special exception the author gives
  unlimited permission to copy and/or distribute it, with or without 
  modifications, as long as this notice is preserved. 

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY, to the extent permitted by law; without even the 
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
  

/* 
   ANITA data pipe monitoring tool   

   $Header: /cvsroot/anitacode/gse/monitor/datmon_raw.c,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $ 
*/

#include <stdio.h>
#include <ncurses.h> 
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#include "shmem.h"

/* Shared memory definitions */
#include "shmem_def.h"
static int nBuf=0;
static unsigned long Shmem_Length[NBUF];

/* Pausing/quitting control */
static int pause=0;
static int run=1;

/* Toggles pause state */
void pauseIt(int sig){
  pause = !pause;
}

/* Quit */
void quit(int sig){
  run=0;
}

unsigned short crc_short(unsigned char* pck,int n){
  int i;
  unsigned short val,sum = 0;

  for(i=0;i<n;i+=2){
    val=*((unsigned short*)(pck+i));
    sum+=val;
  }
  return ((0xffff-sum)+1);
}

int main(int argc, char *argv[]){	
  WINDOW *infowin,*statwin,*dumpwin;
  int ch,i,row,col,srow;
  time_t pcktime=0;
  int badpck=0,npck=0; /* Packet counters */
  int ntype[8]={0,0,0,0,0,0,0,0};
  int ret,bytes;
  unsigned long n,last_n=0,totBytes=0,burstBytes=0,currBytes=0;
  unsigned char pck[MAX_DATA];
  time_t startTime,lastTime,burstTime,currStart;
  float lat=0,lon=0,alt=0;
  float currRate;

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
	unsigned short crcNew,crcOld;

	lastTime=currTime; // Record packet arrival time
	/* Line scrollings */
	scroll(dumpwin);
	wmove(dumpwin,row-1,0);
	for(i=0;i<n;++i)
	  wprintw(dumpwin,"%02x ",(unsigned char)pck[i]);
	//mvwprintw(dumpwin,row-2,0,"Read from buffer %d, %d bytes",nBuf,n);
	/* Packet statistics */
	++npck;
	type=pck[11];
	switch((type&0xf0)>>4){
	case 0x0:
	  ++ntype[0];
	  pcktime=pck[12]+pck[13]*256+pck[14]*256*256+pck[15]*256*256*256;
	  break;
	case 0x1:
	  switch(type){
	  case 0x10:
	    ++ntype[1];
	    pcktime=pck[12]+pck[13]*256+pck[14]*256*256+pck[15]*256*256*256;
	    break;
	  case 0x11:
	    ++ntype[2];
	    pcktime=pck[12]+pck[13]*256+pck[14]*256*256+pck[15]*256*256*256;
	    break;
	  case 0x13:
	    ++ntype[3];
	    pcktime=pck[15]+pck[14]*256+pck[13]*256*256+pck[12]*256*256*256;
	    break;
	  case 0x1f:
	    ++ntype[4];
	    pcktime=pck[15]+pck[14]*256+pck[13]*256*256+pck[12]*256*256*256;
	    break;
	  default:
	    ++ntype[7];
	    break;
	  }
	  break;
	case 0x5:
	  ++ntype[5];
	  pcktime=pck[16]+pck[17]*256+pck[18]*256*256+pck[19]*256*256*256;
	  lat=*((float*)(pck+84));
	  lon=*((float*)(pck+88));
	  alt=*((float*)(pck+92));
	  break;
	case 0x6:
	  ++ntype[6];
	  pcktime=pck[16]+pck[17]*256+pck[18]*256*256+pck[19]*256*256*256;
	  break;
	default:
	  ++ntype[7];
	  break;
	}
	/* Check crc */
	crcOld=*((unsigned short*)(pck+n-2));
	crcNew=crc_short(pck,n-2);
	if(crcNew!=crcOld) ++badpck;
	//if(!pause) mvwprintw(statwin,18,1,"%0x %0x",crcOld,crcNew);
	//if(!pause) mvwprintw(statwin,18,1,"%d",pcktime);
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

    if(!pause) wnoutrefresh(dumpwin);  /* Show scroll if not paused */

    /* Display current time */
    mvwprintw(infowin,0,0,"             UTC: %s",asctime(gmtime(&currTime)));
    wprintw(infowin,"Last packet time: %s",asctime(gmtime(&pcktime)));
    wprintw(infowin,"Last position: %.2f deg %.2f deg %.1f m\n",
	    ((int)lat/100+(lat>0?1:-1)*(abs(lat)%100)/60.),
	    ((int)lon/100+(lon>0?1:-1)*(abs(lon)%100)/60.),alt);
	    
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
      mvwprintw(statwin,srow++,1,"CMD:  %5.1f %%",(double)ntype[0]/npck*100);
      mvwprintw(statwin,srow++,1,"HK1:  %5.1f %%",(double)ntype[1]/npck*100);
      mvwprintw(statwin,srow++,1,"HK2:  %5.1f %%",(double)ntype[2]/npck*100);
      mvwprintw(statwin,srow++,1,"HK4:  %5.1f %%",(double)ntype[3]/npck*100);
      mvwprintw(statwin,srow++,1,"HK16: %5.1f %%",(double)ntype[4]/npck*100);
      mvwprintw(statwin,srow++,1,"TRG:  %5.1f %%",(double)ntype[5]/npck*100);
      mvwprintw(statwin,srow++,1,"WFM:  %5.1f %%",(double)ntype[6]/npck*100);
      mvwprintw(statwin,srow++,1,"???:  %5.1f %%",(double)ntype[7]/npck*100);
      srow++;
      /* Display xfer errors */
      mvwprintw(statwin,srow++,1,"Xfer errors: %d",badpck);
      mvwprintw(statwin,srow++,1,"            %5.3f %%",(double)badpck/npck*100);
    }

    wnoutrefresh(statwin);
    doupdate();
  }
  
  endwin();			/* End curses mode		  */

  shmem_unlink();
  return 0;
}

