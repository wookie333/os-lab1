#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include "lab.h"

static void *symbol(void *library, const char *name)
{
    void *address = dlsym(library, name);
    if (!address) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    return address;
}

int main(void)
{
    const char *path[] = {"./lib1.so", "./lib2.so"};
    int i = 0;
    for (;;) {
        void *library = dlopen(path[i], RTLD_NOW);
        if (!library) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }
        Count *count = (Count *)symbol(library, "prime_count");
        Convert *convert_number = (Convert *)symbol(library, "convert");
        puts(path[i]);
        int change = commands(count, convert_number, 1);
        if (dlclose(library) != 0) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }
        if (!change) return 0;
        i = 1 - i;
    }
}
