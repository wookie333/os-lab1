#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lab.h"

static int read_numbers(char *line, int values[])
{
    int n = 0;
    char *word = strtok(line, " \t\r\n");
    while (word) {
        char *end;
        errno = 0;
        long value = strtol(word, &end, 10);
        if (n == 3 || *end || errno || value < INT_MIN || value > INT_MAX)
            return -1;
        values[n++] = (int)value;
        word = strtok(NULL, " \t\r\n");
    }
    return n;
}

int commands(Count *count, Convert *convert_number, int can_switch)
{
    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, stdin) >= 0) {
        int v[3];
        int n = read_numbers(line, v);
        if (n == 1 && v[0] == 0 && can_switch) {
            free(line);
            return 1;
        }
        if (n == 3 && v[0] == 1 && v[1] >= 1 && v[1] <= v[2]) {
            int result = count(v[1], v[2]);
            if (result < 0) puts("Недостаточно памяти");
            else printf("%d\n", result);
        } else if (n == 2 && v[0] == 2) {
            puts(convert_number(v[1]));
        } else {
            puts("Введите 1 a b (1 <= a <= b) или 2 x");
        }
    }
    free(line);
    if (!feof(stdin)) {
        perror("getline");
        exit(1);
    }
    return 0;
}
