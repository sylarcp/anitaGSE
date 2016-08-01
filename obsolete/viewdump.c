/*  Copyright (C) 2005 Predrag Miocinovic <predrag@phys.hawaii.edu> 

  This file is free software; as a special exception the author gives
  unlimited permission to copy and/or distribute it, with or without 
  modifications, as long as this notice is preserved. 

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY, to the extent permitted by law; without even the 
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
  

/* 
   Viewer for datmon dumps

   $Header: /cvsroot/anitacode/gse/monitor/viewdump.c,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $ 
*/

#include <stdio.h>
#include <ncurses.h> 

int main(int argc, char *argv[]){	
  initscr();			/* Start curses mode 		*/
  cbreak();			/* Line buffering disabled, Pass on
				 * everty thing to me 		*/
  keypad(stdscr, TRUE);		/* I need that nifty F1 	*/
  curs_set(0);                  /* Invisible cursor */

  refresh();
  scr_init(argv[1]);
  refresh();
  getch();
  endwin();

  return 0;
}
