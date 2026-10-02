#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *line = NULL;
    size_t cap = 0;

    while (getline(&line, &cap, stdin) != -1) {
        size_t n = strcspn(line, "\n");
        line[n] = '\0';

        for (size_t i = 0; i < n / 2; i++) {
            char t = line[i];
            line[i] = line[n - 1 - i];
            line[n - 1 - i] = t;
        }

        if (printf("%s\n", line) < 0 || fflush(stdout) == EOF) {
            perror("write");
            free(line);
            return 1;
        }
    }

    free(line);
    return 0;
}
