#define _POSIX_C_SOURCE 200809L
#include "shared.h"

static void write_all(const char *s, int size)
{
    while (size > 0) {
        ssize_t n = write(STDOUT_FILENO, s, size);
        if (n == -1 && errno == EINTR) continue;
        if (n <= 0) fail("write");
        s += n;
        size -= (int)n;
    }
}

int main(int argc, char **argv)
{
    if (argc != 5) return 1;
    int fd = shm_open(argv[1], O_RDWR, 0);
    if (fd == -1) fail("shm_open");
    Message *m = map_message(fd);
    sem_t *ready = sem_open(argv[2], 0);
    sem_t *ack = sem_open(argv[3], 0);
    if (ready == SEM_FAILED || ack == SEM_FAILED) fail("sem_open");

    int out = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out == -1) fail(argv[4]);
    if (dup2(out, STDOUT_FILENO) == -1) fail("dup2");
    if (out != STDOUT_FILENO && close(out) == -1) fail("close");
    post(ack);

    for (;;) {
        wait_sem(ready);
        if (m->size == -1) break;
        for (int i = 0, j = m->size - 1; i < j; i++, j--) {
            char c = m->text[i];
            m->text[i] = m->text[j];
            m->text[j] = c;
        }
        write_all(m->text, m->size);
        if (m->last) write_all("\n", 1);
        post(ack);
    }

    if (close(STDOUT_FILENO) == -1) fail("close");
    if (munmap(m, sizeof(*m)) == -1) fail("munmap");
    if (sem_close(ready) == -1 || sem_close(ack) == -1) fail("sem_close");
    return 0;
}
