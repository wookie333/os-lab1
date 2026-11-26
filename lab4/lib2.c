#include <stdlib.h>
#include "lab.h"

int prime_count(int a, int b)
{
    if (a < 1 || a > b) return -1;
    char *composite = calloc((size_t)b + 1, sizeof(char));
    if (!composite) return -1;
    composite[1] = 1;
    for (long long p = 2; p * p <= b; p++) {
        if (composite[p]) continue;
        for (long long n = p * p; n <= b; n += p) composite[n] = 1;
    }
    int count = 0;
    for (long long n = a; n <= b; n++)
        if (!composite[n]) count++;
    free(composite);
    return count;
}
