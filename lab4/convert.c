#include <limits.h>
#include "lab.h"

char *convert(int x)
{
    static char text[sizeof(int) * CHAR_BIT + 2];
    int pos = sizeof(text) - 1;
    long long n = x;
    if (n < 0) n = -n;
    text[pos] = '\0';
    do {
        text[--pos] = '0' + n % BASE;
        n /= BASE;
    } while (n > 0);
    if (x < 0) text[--pos] = '-';
    return &text[pos];
}
