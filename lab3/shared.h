#ifndef SHARED_H
#define SHARED_H

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct {
    int size;
    int last;
    char text[4096];
} Message;

static void fail(const char *what)
{
    perror(what);
    exit(1);
}

static void wait_sem(sem_t *sem)
{
    while (sem_wait(sem) == -1)
        if (errno != EINTR) fail("sem_wait");
}

static void post(sem_t *sem)
{
    if (sem_post(sem) == -1) fail("sem_post");
}

static Message *map_message(int fd)
{
    Message *m = mmap(NULL, sizeof(Message), PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) fail("mmap");
    if (close(fd) == -1) fail("close");
    return m;
}

#endif
