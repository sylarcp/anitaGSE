// sem.c - posix semaphores implemented as fifos
// from Stevens, Unix Network Programming, Vol 2., Ch 10.14
// and modified by
// Marty Olevitch, Nov '02

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "sem.h"

#define SEM_MAGIC 0x89674523

sem_t *
sem_open(const char *pathname, int oflag, ...)
{
    va_list ap;
    char c;
    int flags;
    mode_t mode;
    sem_t *sem;
    unsigned int value;

    if (oflag & O_CREAT) {
	va_start(ap, oflag);
	mode = va_arg(ap, mode_t);
	value = va_arg(ap, unsigned int);
	va_end(ap);

	if (mkfifo(pathname, mode) < 0) {
	    if (errno == EEXIST && (oflag & O_EXCL) == 0) {
		// already exists, turn off O_CREAT to not do later stuff
		oflag &= ~O_CREAT;
	    } else {
		return SEM_FAILED;
	    }
	}
    }

    if ((sem = (sem_t *)malloc(sizeof(sem_t))) == NULL) {
	return SEM_FAILED;
    }
    sem->sem_fd[0] = sem->sem_fd[1] = -1;

    if ((sem->sem_fd[0] = open(pathname, O_RDONLY | O_NONBLOCK)) < 0) {
	goto error;
    }
    if ((sem->sem_fd[1] = open(pathname, O_WRONLY | O_NONBLOCK)) < 0) {
	goto error;
    }

    // Turn off nonblocking for read fd.
    if ((flags = fcntl(sem->sem_fd[0], F_GETFL, flags)) < 0) {
	goto error;
    }
    flags &= ~O_NONBLOCK;
    if (fcntl(sem->sem_fd[0], F_SETFL, flags) < 0) {
	goto error;
    }

    if (oflag & O_CREAT) {
	// initialize semaphore
	int i;
	for (i=0; i < value; i++) {
	    if (write(sem->sem_fd[1], &c, 1) != 1) {
		goto error;
	    }
	}
    }
    sem->sem_magic = SEM_MAGIC;
    return sem;

error:
    {
	int save_errno;
	save_errno = errno;
	if (oflag & O_CREAT) {
	    // Get rid of fifo we created.
	    unlink(pathname);
	}
	close(sem->sem_fd[0]);
	close(sem->sem_fd[1]);
	free(sem);
	errno = save_errno;
	return SEM_FAILED;
    }
}

int
sem_close(sem_t *sem)
{
    if (sem->sem_magic != SEM_MAGIC) {
	errno = EINVAL;
	return -1;
    }
    sem->sem_magic = 0;
    if (close(sem->sem_fd[0]) == -1 || close(sem->sem_fd[1]) == -1) {
	free(sem);
	return -1;
    }
    free(sem);
    return 0;
}

int
sem_unlink(const char *pathname)
{
    return unlink(pathname);
}

int
sem_post(sem_t *sem)
{
    char c;
    if (sem->sem_magic != SEM_MAGIC) {
	errno = EINVAL;
	return -1;
    }
    if (write(sem->sem_fd[1], &c, 1) == 1) {
	return 0;
    }
    return -1;
}

int
sem_wait(sem_t *sem)
{
    char c;
    if (sem->sem_magic != SEM_MAGIC) {
	errno = EINVAL;
	return -1;
    }
    if (read(sem->sem_fd[0], &c, 1) == 1) {
	return 0;
    }
    return -1;
}

int
sem_trywait(sem_t *sem)
{
    char c;
    int flags;
    int got;
    int ret;

    if (sem->sem_magic != SEM_MAGIC) {
	errno = EINVAL;
	return -1;
    }

    // Turn on nonblocking for read fd.
    if ((flags = fcntl(sem->sem_fd[0], F_GETFL, flags)) < 0) {
	return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sem->sem_fd[0], F_SETFL, flags) < 0) {
	return -1;
    }

    got = read(sem->sem_fd[0], &c, 1);
    if (got == 1) {
	ret = 0;
    } else {
	errno = EAGAIN;
	ret = -1;
    }

    // Turn off nonblocking for read fd.
    if ((flags = fcntl(sem->sem_fd[0], F_GETFL, flags)) < 0) {
	return -1;
    }
    flags &= ~O_NONBLOCK;
    if (fcntl(sem->sem_fd[0], F_SETFL, flags) < 0) {
	return -1;
    }

    return ret;
}

int
sem_getvalue(sem_t *sem, int *sval)
{
    /* sem_getvalue() doesn't work on all systems. I have found that the
     * fstat() call does not report the number of bytes available in the
     * fifo in st_size on Linux 2.4 or OpenBSD 3.1. On these systems, it
     * always returns 0. But it does work on Solaris 5.7. */
    struct stat statbuf;

    if (sem->sem_magic != SEM_MAGIC) {
	errno = EINVAL;
	return -1;
    }

    if (fstat(sem->sem_fd[0], &statbuf) == -1) {
	return -1;
    }

    *sval = (int)statbuf.st_size;
    return 0;
}
