
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

/* parsenum.c - parse strings like "2,3,4,8-10"
 *
 * Marty Olevitch, Mar 2000
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parsenum.h"

static char *cvsid = "$Id: parsenum.c,v 1.1.1.1 2005/07/20 03:12:17 predragm Exp $";

enum {
    WANT_DIGIT,
    WANT_COMMA_DASH_DIGIT,
    WANT_ONE_DIGIT,
    POST_DASH_ACCUM,
};

#define COMMA ','
#define DASH  '-'

static int add_store(char c);
static void reset_store(void);
static int insert(int *accumbuf);
static int multi_insert(int *accumbuf);

static int Accumcnt;

static const int STORE_INC = 1024;
static char *Store = NULL;
static int Storecnt;
static int Store_max = 0;

static int In_dash;
static int Max;

int
parsenum(char *str, int *accumbuf, int max)
{
    char *cp = str;
    int state = WANT_DIGIT;

    reset_store();
    Accumcnt = 0;
    In_dash = 0;
    Max = max;

    while (*cp) {
	switch (state) {
	    case WANT_DIGIT:
	    	if (!isdigit(*cp)) {
		    return 0;
		}
		if (add_store(*cp)) {
		    return 0;
		}
		state = WANT_COMMA_DASH_DIGIT;
	    	break;
	    case WANT_COMMA_DASH_DIGIT:
	    	if (isdigit(*cp)) {
		    if (add_store(*cp)) {
			return 0;
		    }
		} else if (*cp == COMMA) {
		    if (insert(accumbuf)) {
			/* overflow */
			return 0;
		    }
		    state = WANT_DIGIT;
		} else if (*cp == DASH) {
		    if (insert(accumbuf)) {
			/* overflow */
			return 0;
		    }
		    In_dash = 1;
		    state = WANT_ONE_DIGIT;
		} else {
		    return 0;
		}
	    	break;
	    case WANT_ONE_DIGIT:
	    	if (isdigit(*cp)) {
		    if (add_store(*cp)) {
			return 0;
		    }
		    state = POST_DASH_ACCUM;
		} else {
		    return 0;
		}
	    	break;
	    case POST_DASH_ACCUM:
	    	if (isdigit(*cp)) {
		    if (add_store(*cp)) {
			return 0;
		    }
		} else if (*cp == COMMA) {
		    if (multi_insert(accumbuf)) {
			/* overflow */
			return -1;
		    }
		    In_dash = 0;
		    state = WANT_DIGIT;
		} else {
		    return 0;
		}
	    	break;
	    default:
	    	break;
	}

	cp++;
    }

    if (In_dash) {
	if (multi_insert(accumbuf)) {
	    /* overflow */
	    return -1;
	}
    } else {
	if (insert(accumbuf)) {
	    /* overflow */
	    return 0;
	}
    }

    return Accumcnt;
}

static void 
reset_store(void)
{
    memset(Store, '\0', Store_max);
    Storecnt = 0;
}

static int
add_store(char c)
{
    if (Storecnt >= Store_max) {
#ifndef REALLOC_NULL
	if (Store == NULL) {
	    Store = malloc(Store_max + STORE_INC);
	} else
#endif
	    Store = realloc(Store, Store_max + STORE_INC);

	if (Store == NULL) {
	    return -1;
	}
	Store_max += STORE_INC;
    }
    Store[Storecnt] = c;
    Storecnt++;
    return 0;
}

static int
insert(int *accumbuf)
{
    if (Storecnt > 0) {
	accumbuf[Accumcnt] = atoi(Store);
	Accumcnt++;
	if (Accumcnt > Max) {
	    return -1;
	}
    }
    reset_store();
    return 0;
}

static int
multi_insert(int *accumbuf)
{
    int i;
    int start = accumbuf[Accumcnt-1];
    int end = atoi(Store);
    for (i=start+1; i <= end; i++) {
	accumbuf[Accumcnt] = i;
	Accumcnt++;
	if (Accumcnt > Max) {
	    return -1;
	}
    }
    reset_store();
    return 0;
}

#ifdef NEED_MAIN
int
main(int argc, char *argv[])
{
    int i;
    int accum[1024];
    int j;
    int cnt;

    for (i=1; i<argc; i++) {
	cnt = parsenum(argv[i], accum, 1024);
	printf("%s\t", argv[i]);
	for (j=0; j < cnt; j++) {
	    printf("%d ", accum[j]);
	}
	printf("\n");
    }
}
#endif /* NEED_MAIN */
