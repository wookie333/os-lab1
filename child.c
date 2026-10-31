
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "io_utils.h"

int main(void) {
    char *line = NULL;
    size_t cap = 0;

    while (read_line(STDIN_FILENO, &line, &cap) != -1) {
        size_t n = strcspn(line, "\n");
        line[n] = '\0';

        for (size_t i = 0; i < n / 2; i++) {
            char t = line[i];
            line[i] = line[n - 1 - i];
            line[n - 1 - i] = t;
        }

        if (write_all(STDOUT_FILENO, line, n) == -1 ||
            write_all(STDOUT_FILENO, "\n", 1) == -1) {
            print_error("write");
            free(line);
            return 1;
        }
    }

    free(line);
    return 0;
}
