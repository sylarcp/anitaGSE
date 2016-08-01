/* shmem.c - shared memory, semaphores as IPC
 *
 * Marty Olevitch, Aug 1999
 * 	Sep, 2001
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>	/* strncpy, memset, strlen */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
//#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//#include "pxsv.h"
#include "sem.h"

#include "shmem.h"
/*#include "have_ltrunc.h"*/

static char *Shmem_version = "3.09";
/****
    03.09	27 Oct 03	Added SEM_PERMS and SHMEM_PERMS so can
				    easily change shared memory and
				    semaphore permissions.
    03.08	30 Dec 02	Move slot_avail flags out of each seg. header.
				    Have only one Data_avail semaphore per
				    reader. In shmem_read(), the reader
				    checks its new_data flag.
    03.07	27 Dec 02	Back to using Data_avail semaphores.
    03.06	27 Dec 02	Removed some unnecessary code. Encapsulated
				    code for opening semaphores. Fixed some
				    memory leaks in shmem_unlink().
    03.05	 4 Nov 02	Eliminating Data_avail semaphores.
    03.04	 4 Nov 02	Exceeding MAX_SEGS error mesg fixed. Use
				    sem.h (implemented using fifos) instead
				    of pxsv.h (using sys5 semaphores).
    03.03	 5 Dec 01	Data_avail semaphores for each reader for each
				    segment.
    03.02	22 Nov 01	Use a semaphore to tell if data is
				    available for a given segment.
    03.01	20 Sep 01	divide the shared memory into many regions,
				    each protected with a semaphore
    03.00	?? Sep 01	uses posix semaphore implemented with Sys V
				    semaphore and Sys V shared memory.
****/

enum { 
    MAX_NAME_LEN = 1024,
    MAX_READERS = 50,
    MAX_SEGS = 500,
    SLOT_AVAIL = 0,	// slot available
    SLOT_IN_USE = 1,	// slot in use
};

/* "header" for each batch of data in shared memory */
struct Shmem_hdr {
    unsigned char new_data[MAX_READERS];// new data for reader r
    unsigned long nbytes;		// number of bytes of data 
};

/* Mem_lock - controls access to the memory for a particular data segment */
static sem_t *Mem_lock[MAX_SEGS+1];
#define MAIN_SEM MAX_SEGS	/* index in Mem_lock of semaphore for the
				 * shared memory as a whole. */

/* Data_avail - array of semaphores per segment per reader */
static sem_t *Data_avail[MAX_READERS];

static char Mem_lock_name[MAX_NAME_LEN];	/* name of semaphore */

static void *Shmem;		/* ptr to start of shared memory */
static int Shmem_id;		/* "id" of shared memory */
static unsigned char  *Data[MAX_SEGS];	/* start of data within shared memory */
static struct Shmem_hdr *Hdr[MAX_SEGS];	/* ptr to header within shared memory */
static char *Slot_avail;	// embedded in shared mem.

static int Shmem_create = 0;	/* nonzero if this process is the "creator" */
static unsigned long Shmem_size = 0L;	/* total length (bytes) shared memory */
static unsigned long Data_size[MAX_SEGS];	/* length (bytes) of data parts
						 * of shared memory. */
static int Nseg;
static int Reader_id = -1;
static int Verbose = 0;
#define ERRMESG_LEN     512     /* length of error message string */
static char _Errmesg[ERRMESG_LEN];
static void set_errmesg(char *fmt, ...);

#define SEM_PERMS	0644
#define SHMEM_PERMS	0644

static void close_data_avail_sems(void);
static void close_main_sem(void);
static void close_seg_sems(void);
static int open_data_avail_sems(void);
static int open_main_sem(void);
static int open_seg_sems(void);
static int set_sem_name(char *name);

/* shmem_init - initialize the shared memory and semaphores. If 'create' is
 * set, then create these objects. Only one of the cooperating processes
 * should create them. "id" is a value between 0 to MAX_READERS-1 and is
 * used to determine whether data has been accessed by this particular
 * process already. Each of the cooperating processes must be given a
 * unique id. However, the writer process requires no id. 'size' is the
 * maximum length of the data in bytes. Returns 0 if all went well, else
 * -1. */
int 
shmem_init(char *name, long memkey, int nseg, unsigned long *size, int create)
{
    int flag;
    int i;
    int ret;

    if (nseg > MAX_SEGS) {
	set_errmesg("shmem_init: Too many segments (%d). Maximum = %d",
	    nseg, MAX_SEGS);
	return -1;
    }
    Nseg = nseg;

    if (set_sem_name(name)) {
	return -1;
    }

    if (create) {
	Shmem_create = 1;
    }

    if (open_main_sem()) {
	return -1;
    }

    // The data writer already holds the main semaphore at this point
    // because it created it. The data reader must wait on it at this
    // point.
    if (!create) {
	ret = sem_wait(Mem_lock[MAIN_SEM]);
	if (ret) {
	    set_errmesg("shmem_init: bad sem_wait (%s)", strerror(errno));
	    sem_close(Mem_lock[MAIN_SEM]);
	    return -1;
	}
    }

    if (open_seg_sems()) {
	sem_close(Mem_lock[MAIN_SEM]);
	return -1;
    }

    if (open_data_avail_sems()) {
	int n;
	sem_close(Mem_lock[MAIN_SEM]);
	for (n=0; n<Nseg; n++) {
	    sem_close(Mem_lock[n]);
	}
	return -1;
    }

    /* Get the shared memory. */
    if (create) {
	flag = SHMEM_PERMS | IPC_CREAT;
    } else {
	flag = SHMEM_PERMS;
    }

    // The first MAX_READERS bytes of our shared memory are reserved for
    // the Slot_avail array.
    Shmem_size = MAX_READERS;

    // Now add the sizes of the data areas.
    for (i=0; i<nseg; i++) {
	Data_size[i] = size[i] + sizeof(struct Shmem_hdr);
	Shmem_size += Data_size[i];
    }
    printf("Shmem_size %d bytes, %f MB\n",Shmem_size,Shmem_size/(1024.*1024.));
    Shmem_id = shmget(memkey, Shmem_size, flag);
    if (Shmem_id == -1) {
	set_errmesg("shmem_init: bad shmget (%s)", strerror(errno));
	shmem_unlink();
	return -1;
    }

    Shmem = shmat(Shmem_id, 0, 0);
    if (Shmem == (void *)-1) {
	set_errmesg("shmem_init: bad shmat (%s)", strerror(errno));
	shmem_unlink();
	return -1;
    }

    Slot_avail = (char *)Shmem;
    {
	unsigned char *p = Slot_avail + MAX_READERS;
	for (i=0; i<nseg; i++) {
	    Hdr[i] = (struct Shmem_hdr *)p;
	    Data[i] = (unsigned char *)Hdr[i] + sizeof(struct Shmem_hdr);
	    p = Data[i] + Data_size[i];
	}

    }

    if (create) {
	int n;
	/* Since I'm the data writer, set all the Slot_avail flags to
	 * SLOT_AVAIL to indicate that they are available for readers. */
	for (n=0; n<nseg; n++) {
	    for (i=0; i<MAX_READERS; i++) {
		Slot_avail[i] = SLOT_AVAIL;
		Hdr[n]->new_data[i] = 0;
	    }
	    sem_post(Mem_lock[n]);
	}
    } else {
	/* Since I'm one of possibly several readers, grab the first
	 * available slot. At this point, we hold the Mem_lock[MAIN_SEM]
	 * semaphore. */
	for (i=0; i<MAX_READERS; i++) {
	    if (Slot_avail[i] == SLOT_AVAIL) {
		break;
	    }
	}

	if (i >= MAX_READERS) {
	    /* no slots available */
	    set_errmesg("shmem_init: too many readers!\n");
	    sem_post(Mem_lock[MAIN_SEM]);
	    sem_close(Mem_lock[MAIN_SEM]);
	    shmem_unlink();
	    return -1;
	}
	Reader_id = i;
	if (Verbose) {
	    fprintf(stderr, "               init: Reader_id = %d\n", Reader_id);
	}
	for (i=0; i<nseg; i++) {
	    Slot_avail[Reader_id] = SLOT_IN_USE;
	}
    }

    ret = sem_post(Mem_lock[MAIN_SEM]);
    if (ret) {
	set_errmesg("shmem_init: bad sem_post (%s)", strerror(errno));
	shmem_unlink();
	return -1;
    }

    return 0;
}

/* shmem_read - copy data out of shared memory. If the pointer buf is not
 * NULL, data will be copied to this memory location. The pointer nbytes
 * will be filled in with the number of data bytes. If the function pointer
 * is not NULL, it is taken to refer to a function to call to process the
 * data. Using the callback function should guarantee that no data will be
 * missed while processing the data because the semaphore is not posted
 * until after the callback function returns. Returns 0 if all went well,
 * -1 on error, -2 if the memory was already locked. */

int 
shmem_read(int n, int dowait, void *buf, unsigned long *nbytes,
    int (*f)(void *, unsigned long))
{
    int ret = 0;

    if (n >= Nseg || n < 0) {
	set_errmesg("shmem_read: bad n = %d", n);
	return -1;
    }

    if (buf == NULL && f == NULL) {
	set_errmesg("shmem_read: both buf and f are NULL!");
	return -1;
    }

    /* Wait on Data_avail for seg and reader */
wait:
    if (dowait){
      if(sem_wait(Data_avail[Reader_id])) {
	set_errmesg("shmem_read: Data_avail sem_wait (%s)", strerror(errno));
	return -1;
      }
    }else{
      if(sem_trywait(Data_avail[Reader_id])){
	set_errmesg("shmem_read: Data_avail sem_trywait (%s)", strerror(errno));
	if (errno == EAGAIN) {
	  /* semaphore already locked */
	  return -2;
	} else {
	  return -1;
	}
      }	
    }

    /* Wait on the Mem_lock semaphore for the desired segment. */
    if (sem_wait(Mem_lock[n])) {
	set_errmesg("shmem_read: Mem_lock sem_wait (%s)", strerror(errno));
	    return -1;
    }

    *nbytes = 0L;
    if (Hdr[n]->new_data[Reader_id]) {
	if (buf != NULL) {
	    memcpy(buf, Data[n], Hdr[n]->nbytes);
	    ret = 0;
	}
	if (f != NULL) {
	    ret = f(Data[n], Hdr[n]->nbytes);
	}
	*nbytes = Hdr[n]->nbytes;
	Hdr[n]->new_data[Reader_id] = 0;
	sem_post(Mem_lock[n]);
    } else  {
	// No data in this segment.
	sem_post(Mem_lock[n]);
	if(dowait) goto wait;
    }

    return ret;
}

/* shmem_write - copy nbytes of data into shared memory from buf, set up
 * the header. Returns 0 if all went well, -1 on error, -2 if the semaphore
 * was already locked (in which case, the application can try again). */
int
shmem_write(int n, void *buf, unsigned long nbytes)
{
    int i;
    int r;

    if (n >= Nseg || n < 0) {
	set_errmesg("shmem_write: bad n = %d", n);
	return -1;
    }

    if (nbytes > Data_size[n]) {
	set_errmesg("shmem_write: nbytes (%lu) too big (max=%lu)",
	    nbytes, Data_size[n]);
	return -1;
    }

    if (sem_trywait(Mem_lock[n])) {
	set_errmesg("shmem_write: sem_trywait (%s)", strerror(errno));
	if (errno == EAGAIN) {
	    /* semaphore already locked */
	    return -2;
	} else {
	    return -1;
	}
    }

    Hdr[n]->nbytes = nbytes;
    memcpy(Data[n], buf, nbytes);

    for (r=0; r < MAX_READERS; r++) {
	Hdr[n]->new_data[r] = 1;
	if (Slot_avail[r] == SLOT_IN_USE) {
	    sem_post(Data_avail[r]);
	}
    }

    sem_post(Mem_lock[n]);
    return 0;
}

/* shmem_unlink - detach shared memory and and release the semaphore. The
 * creator process takes care of actually removing the semaphore and mem. */
void
shmem_unlink(void)
{
    int n;
    int r;

    if (Shmem_create) {
	close_seg_sems();
	close_data_avail_sems();
	(void)shmdt(Shmem);
	(void)shmctl(Shmem_id, IPC_RMID, NULL);
    } else {
	// Reader_id < 0 means we were not fully initialized.
	if (Reader_id >= 0) {
	    close_seg_sems();
	    close_data_avail_sems();
	}
	(void)shmdt(Shmem);
    }

    close_main_sem();
}

static void
set_errmesg(char *fmt, ...)
{
    va_list ap;
    if (fmt == NULL) {
        _Errmesg[0] = '\0';
    } else {
        va_start(ap, fmt);
        vsprintf(_Errmesg, fmt, ap);
        va_end(ap);
    }
}

char *
shmem_strerror(void)
{
    return _Errmesg;
}

int
shmem_id(void)
{
    return Reader_id;
}

char *
shmem_version(void)
{
    return Shmem_version;
}

void
shmem_verbose(int val)
{
    Verbose = val;
}

static int
open_main_sem(void)
{
    if (Shmem_create) {
	Mem_lock[MAIN_SEM] = sem_open(Mem_lock_name, O_RDWR | O_CREAT,
	    SEM_PERMS, 0);
    } else {
	Mem_lock[MAIN_SEM] = sem_open(Mem_lock_name, 0);
    }

    if (Mem_lock[MAIN_SEM] == SEM_FAILED) {
	set_errmesg("open_main_sem: bad sem_open (%s)", strerror(errno));
	return -1;
    }
    return 0;
}

/* open_seg_sems - open all of the segment semaphores */
static int
open_seg_sems(void)
{
    int i;
    char sname[MAX_NAME_LEN];

    memset(sname, '\0', MAX_NAME_LEN);

    // Regular Mem_lock semaphores.
    for (i=0; i<Nseg; i++) {
	sprintf(sname, "%s%03d", Mem_lock_name, i);
	if (Shmem_create) {
	    Mem_lock[i] = sem_open(sname, O_RDWR | O_CREAT,
		SEM_PERMS, 0);
	} else {
	    Mem_lock[i] = sem_open(sname, 0);
	}
	if (Mem_lock[i] == SEM_FAILED) {
	    int n;
	    set_errmesg("open_seg_sems: bad sem_open (segment %d) (%s)",
		i, strerror(errno));
	    for (n=0; n<i; n++) {
		sem_close(Mem_lock[n]);
	    }
	    return -1;
	}
    }
    return 0;
}

static int
open_data_avail_sems(void)
{
    int r;
    char sname[MAX_NAME_LEN];

    memset(sname, '\0', MAX_NAME_LEN);

    for (r=0; r<MAX_READERS; r++) {
	sprintf(sname, "%s%03dD", Mem_lock_name, r);
	if (Shmem_create) {
	    Data_avail[r] = sem_open(sname, O_RDWR | O_CREAT,
		SEM_PERMS, 0);
	} else {
	    Data_avail[r] = sem_open(sname, 0);
	}
	if (Data_avail[r] == SEM_FAILED) {
	    int n, p;
	    set_errmesg(
		"open_seg_sems: bad Data_avail sem_open (r%d) (%s)",
		r, strerror(errno));
		for (p=0; p<r; p++) {
		    sem_close(Data_avail[p]);
		}
	    return -1;
	}
    }
    return 0;
}

static int
set_sem_name(char *name)
{
    strncpy(Mem_lock_name, name, MAX_NAME_LEN-20);
    return 0;
}

static void
close_main_sem(void)
{
    if (Shmem_create) {
	sem_post(Mem_lock[MAIN_SEM]);
	sem_close(Mem_lock[MAIN_SEM]);
	(void)sem_unlink(Mem_lock_name);
    } else {
	sem_close(Mem_lock[MAIN_SEM]);
    }
}

static void
close_seg_sems(void)
{
    int n;
    char sname[MAX_NAME_LEN];

    memset(sname, '\0', MAX_NAME_LEN);

    // Regular Mem_lock semaphores.
    for (n=0; n<Nseg; n++) {
	if (Shmem_create) {
	    sem_post(Mem_lock[n]);
	    sem_close(Mem_lock[n]);
	    sprintf(sname, "%s%03d", Mem_lock_name, n);
	    (void)sem_unlink(sname);
	} else {
	    sem_wait(Mem_lock[MAIN_SEM]);
	    Slot_avail[Reader_id] = SLOT_AVAIL;
	    sem_post(Mem_lock[n]);
	    sem_close(Mem_lock[n]);
	    sem_post(Mem_lock[MAIN_SEM]);
	}
    }
}

static void
close_data_avail_sems(void)
{
    int r;
    char sname[MAX_NAME_LEN];

    memset(sname, '\0', MAX_NAME_LEN);

    if (!Shmem_create) {
	sem_wait(Mem_lock[MAIN_SEM]);
    }

    for (r=0; r<MAX_READERS; r++) {
	sem_post(Data_avail[r]);
	sem_close(Data_avail[r]);
	if (Shmem_create) {
	    sprintf(sname, "%s%03dD", Mem_lock_name, r);
	    (void)sem_unlink(sname);
	}
    }

    if (!Shmem_create) {
	sem_post(Mem_lock[MAIN_SEM]);
    }
}
