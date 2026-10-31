
#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static ssize_t read_line(int fd, char **line, size_t *cap) {
    size_t len = 0;
    for (;;) {
        char ch;
        ssize_t n = read(fd, &ch, 1);
        if (n == 0) {
            if (len == 0) return -1;
            break;
        }
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (len + 2 > *cap) {
            size_t new_cap = *cap ? *cap * 2 : 128;
            if (new_cap < len + 2) new_cap = len + 2;
            char *p = realloc(*line, new_cap);
            if (p == NULL) return -1;
            *line = p;
            *cap = new_cap;
        }
        (*line)[len++] = ch;
        if (ch == '\n') break;
    }
    (*line)[len] = '\0';
    return (ssize_t)len;
}

static int write_all(int fd, const void *buffer, size_t length) {
    const char *p = buffer;
    while (length > 0) {
        ssize_t n = write(fd, p, length);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }
        p += n;
        length -= (size_t)n;
    }
    return 0;
}

static void print_error(const char *operation) {
    int saved_errno = errno;
    write_all(STDERR_FILENO, operation, strlen(operation));
    write_all(STDERR_FILENO, ": ", 2);
    const char *description = strerror(saved_errno);
    write_all(STDERR_FILENO, description, strlen(description));
    write_all(STDERR_FILENO, "\n", 1);
}

#endif
