// sem.h - posix semaphores implemented as fifos
// from Stevens, Unix Network Programming, Vol 2., Ch 10.14
// and modified by
// Marty Olevitch, Nov '02

#ifndef _SEM_H
#define _SEM_H

typedef struct {
    int sem_fd[2];	// 2 fds: 0 for reading, 1 for writing
    int sem_magic;	// magic no. if open
} sem_t;

#ifdef SEM_FAILED
#undef SEM_FAILED
#endif
#define SEM_FAILED ((sem_t *)(-1))

int sem_close(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
sem_t *sem_open(const char *pathname, int oflag, ...);
int sem_post(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_unlink(const char *pathname);
int sem_wait(sem_t *sem);

#endif // _SEM_H
