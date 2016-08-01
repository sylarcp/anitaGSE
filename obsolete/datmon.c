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

   $Header: /cvsroot/anitacode/gse/monitor/datmon.c,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $ 
*/

#include <stdio.h>
#include <ncurses.h> 
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "shmem.h"

/* Shared memory definitions */
#define MAX_DATA 1024
#define SHMEM_SEM_FILE "/tmp/dataprint"
#define SHMEM_KEY_DATAPRINT 5761L
#define NBUF 50  // Length of shared memory buffer
static int nBuf=0;
static unsigned long Shmem_Length[NBUF];

unsigned short crc_short(char* str,int *bytes){
  unsigned short val,sum = 0;
  unsigned long vall;
  char *tok;
  int first=1;

  *bytes=0;
  val=strtol(strtok(str," "),(char**)NULL,16);
  sum+=val;
  *bytes+=2;
  vall=strtol(strtok(NULL," "),(char**)NULL,10);
  val=vall&0xffff;
  sum+=val;
  *bytes+=2;
  val=(vall&0xffff0000L)>>16;
  sum+=val;
  *bytes+=2;
  vall=strtol(strtok(NULL," "),(char**)NULL,10);
  val=vall&0xffff;
  sum+=val;
  *bytes+=2;
  val=(vall&0xffff0000L)>>16;
  sum+=val;  
  *bytes+=2;
  tok=strtok(NULL," ");
  while(tok!=NULL){
    if(first){
      val=atol(tok);
      first=0;
    }else
      val=strtol(tok,(char**)NULL,16);
    if((tok=strtok(NULL," "))!=NULL){
      val+=strtol(tok,(char**)NULL,16)*256;
      sum+=val;
      *bytes+=2;
      tok=strtok(NULL," ");
    }
  }
  return ((0xffff-sum)+1);
}

int main(int argc, char *argv[]){	
  WINDOW *infowin,*statwin,*dumpwin;
  int ch,i,row,col,srow;
  int run=1,pause=0;
  time_t pcktime=0;
  int badpck=0,npck=0; /* Packet counters */
  int ntype[8]={0,0,0,0,0,0,0,0};
  int ret,bytes;
  unsigned long n,totBytes;
  char line[1024];
  time_t startTime,timeDiff;
  

  /* Setup shared memory */
  for(i=0;i<NBUF;++i) Shmem_Length[i]=MAX_DATA;
  if(shmem_init(SHMEM_SEM_FILE, SHMEM_KEY_DATAPRINT, NBUF, Shmem_Length, 0)){
    fprintf(stderr, "%s: ERROR %s\n", argv[0], shmem_strerror());
    exit(1);
  }
  
  initscr();			/* Start curses mode 		*/
  halfdelay(1);                 /* User control delay, 0.1 s */
  /*  cbreak();			/* Line buffering disabled, Pass on
				 * everty thing to me 		*/
  keypad(stdscr, TRUE);		/* I need that nifty F1 	*/
  noecho();
  curs_set(0);                  /* Invisible cursor */

  refresh();

  /* info window */
  infowin=newwin(4,COLS-21,0,0);
  wborder(infowin, ' ', ' ', ' ','-',' ',' ','-','-');
  wrefresh(infowin);
  
  /* stat window */
  statwin=newwin(LINES-1,21,0,COLS-21);
  wborder(statwin, '|', ' ', ' ','-','|',' ','-','-');
  mvwaddstr(statwin,0,1,"Data rate");
  mvwaddstr(statwin,5,1,"Packet fractions");
  wrefresh(statwin);

  /* data dump window takes, remaining space, except for last 2 lines */
  dumpwin=newwin(LINES-6,COLS-21,4,0);
  //wborder(dumpwin, ' ', ' ', ' ','-',' ',' ','-','-');
  getmaxyx(dumpwin,row,col); 
  scrollok(dumpwin,1);
  wrefresh(dumpwin);

  /* Usage info */
  move(LINES-2,0);
  for(i=0;i<COLS-21;++i) addch('-');
  mvprintw(LINES-1,0,"P-pause R-resume Z-zero counters q-quit");
  refresh();

  startTime=time(NULL)-1;  // -1 to avoid 0 dt on the first pass
  while(run){
    // char filename[128];

    time_t currTime=time(NULL);
    timeDiff=currTime-startTime;

    /* Get new packet */
    ret = shmem_read(nBuf, 0, line, &n, NULL);
    if (ret == -1) {
      fprintf(stderr, "%s: ERROR %s\n", argv[0], shmem_strerror());
      shmem_unlink();
      exit(1);
    } else if (ret == 0) {  // There was new packet
      if (n > 0) { // New data read
	int type;
	short crcNew,crcOld;
	/* Line scrollings */
	scroll(dumpwin);
	mvwaddstr(dumpwin,row-1,0,line);
	//mvwprintw(dumpwin,row-2,0,"Read from buffer %d, %d bytes",nBuf,n);
	/* Packet statistics */
	++npck;
	type=strtol(&line[22],(char**)NULL,16);
	switch((type&0xf0)>>4){
	case 0x0:
	  ++ntype[0];
	  pcktime=strtol(&line[25],(char**)NULL,16)+
	    strtol(&line[28],(char**)NULL,16)*256+
	    strtol(&line[31],(char**)NULL,16)*256*256+
	    strtol(&line[34],(char**)NULL,16)*256*256*256;
	  break;
	case 0x1:
	  switch(type){
	  case 0x10:
	    ++ntype[1];
	    pcktime=strtol(&line[25],(char**)NULL,16)+
	      strtol(&line[28],(char**)NULL,16)*256+
	      strtol(&line[31],(char**)NULL,16)*256*256+
	      strtol(&line[34],(char**)NULL,16)*256*256*256;
	    break;
	  case 0x11:
	    ++ntype[2];
	    pcktime=strtol(&line[25],(char**)NULL,16)+
	      strtol(&line[28],(char**)NULL,16)*256+
	      strtol(&line[31],(char**)NULL,16)*256*256+
	      strtol(&line[34],(char**)NULL,16)*256*256*256;
	    break;
	  case 0x13:
	    ++ntype[3];
	    pcktime=strtol(&line[34],(char**)NULL,16)+
	      strtol(&line[31],(char**)NULL,16)*256+
	      strtol(&line[28],(char**)NULL,16)*256*256+
	      strtol(&line[25],(char**)NULL,16)*256*256*256;
	    break;
	  case 0x1f:
	    ++ntype[4];
	    pcktime=strtol(&line[34],(char**)NULL,16)+
	      strtol(&line[31],(char**)NULL,16)*256+
	      strtol(&line[28],(char**)NULL,16)*256*256+
	      strtol(&line[25],(char**)NULL,16)*256*256*256;
	    break;
	  default:
	    ++ntype[7];
	    break;
	  }
	  break;
	case 0x5:
	  ++ntype[5];
	  pcktime=strtol(&line[37],(char**)NULL,16)+
	    strtol(&line[40],(char**)NULL,16)*256+
	    strtol(&line[43],(char**)NULL,16)*256*256+
	    strtol(&line[46],(char**)NULL,16)*256*256*256;
	  break;
	case 0x6:
	  ++ntype[6];
	  pcktime=strtol(&line[37],(char**)NULL,16)+
	    strtol(&line[40],(char**)NULL,16)*256+
	    strtol(&line[43],(char**)NULL,16)*256*256+
	    strtol(&line[46],(char**)NULL,16)*256*256*256;
	  break;
	default:
	  ++ntype[7];
	  break;
	}
	/* Check crc */
	crcOld=strtol(&line[n-6],(char**)NULL,16);
	line[n-5]='\0'; // Terminate before crc
	crcNew=crc_short(line,&bytes);
	if(crcNew!=crcOld) ++badpck;
	totBytes+=bytes;
      }
      if(++nBuf>=NBUF) nBuf=0;
    } else if (ret == -2) { // Memory locked, try this one again
      //scroll(dumpwin);
      //mvwprintw(dumpwin,row-2,0,"Buffer locked %d",nBuf);
      ;
    } else {
      fprintf(stderr, "Bad return code!!! (%d)\n", ret);
      shmem_unlink();
      exit(1);
    }

    if(!pause) wrefresh(dumpwin);  /* Show scroll if not paused */

    /* Display current time */
    mvwprintw(infowin,0,0,"             UTC: %s",asctime(gmtime(&currTime)));
    wprintw(infowin,"Last packet time: %s",asctime(gmtime(&pcktime)));
    wprintw(infowin,"Last ballon position: %.2f deg %.2f deg %.1f m",-78.,167.,0.);
    wrefresh(infowin);

    /* Display data rates */
    srow=1;
    mvwprintw(statwin,srow++,1,"Burst: %5d kb/s",(int)((double)rand()/RAND_MAX*10000));
    mvwprintw(statwin,srow++,1,"Avg:   %5d kb/s",(int)(totBytes*8./timeDiff));
    mvwprintw(statwin,srow++,1,"Curr:  %5d kb/s",(int)((double)rand()/RAND_MAX*10000));
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
      mvwprintw(statwin,srow++,1,"            %5.1f %%",(double)badpck/npck*100);
    }

    wrefresh(statwin);

    /* Get user command */
    ch = getch();
    switch(ch){	
    case KEY_F(1):
    case 27:
    case 'q':
    case 'Q':
      run=0;
      break;
    case 'p':
    case 'P':
      pause=1;
      break;
    case 'r':
    case 'R':
      pause=0;
      break;
    case 'z':
    case 'Z':
      npck=0;
      for(i=0;i<8;++i) ntype[i]=0;
      totBytes=0;
      badpck=0;
      pcktime=0;
      startTime=currTime-1;
      break;
    /* case 's': */
/*     case 'S': // Screen dump */
/*       sprintf(filename,"datmon_screen.%d",time(NULL)); */
/*       scr_dump(filename); */
/*       break; */
/*     case 'b': */
/*     case 'B': */
/*       scr_restore(filename); */
/*       refresh(); */
/*       sleep(5); */
/*       break; */
    case ERR: // No user input
      break;
    default:
      //      mvwprintw(dumpwin,2,0,"Received %c (%d)\n",ch,ch);
      //      wrefresh(dumpwin);
      break;
    }

  }
  
  endwin();			/* End curses mode		  */

  shmem_unlink();
  return 0;
}

