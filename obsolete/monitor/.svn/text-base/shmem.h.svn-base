/* shmem.h - shmem library header file - QNX version
 *
 * Marty Olevitch, Aug 1999
 * 	Sep 2001
 */

#ifndef _SHMEM_H
#define _SHMEM_H 1

int shmem_id(void);
int shmem_init(char *name, long memkey, int nseg, unsigned long *size,
    int create);
int shmem_read(int n, int dowait, void *buf, unsigned long *nbytes, 
    int (*f)(void *buf, unsigned long nbytes));
char *shmem_strerror(void);
void shmem_unlink(void);
void shmem_verbose(int val);
char * shmem_version(void);
int shmem_write(int n, void *buf, unsigned long nbytes);

#endif /* _SHMEM_H */
