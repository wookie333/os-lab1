#ifndef LAB_H
#define LAB_H

int prime_count(int a, int b);
char *convert(int x);

typedef int Count(int a, int b);
typedef char *Convert(int x);

int commands(Count *count, Convert *convert_number, int can_switch);

#endif
