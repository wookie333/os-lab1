#include "lab.h"

int prime_count(int a, int b)
{
    if (a < 1 || a > b) return -1;
    int count = 0;
    for (long long n = a; n <= b; n++) {
        if (n == 1) continue;
        int divisor = 2;
        while (divisor < n && n % divisor != 0) divisor++;
        if (divisor == n) count++;
    }
    return count;
}
