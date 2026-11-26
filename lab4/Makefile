CFLAGS = -std=c11 -O2 -Wall -Wextra

ifeq ($(shell uname -s),Darwin)
LIBFLAGS = -dynamiclib -Wl,-install_name,@loader_path/$@
else
LIBFLAGS = -shared -fPIC
RPATH = -Wl,-rpath,'$$ORIGIN'
DL = -ldl
endif

all: linked runtime lib2.so

lib1.so: lib1.c convert.c lab.h
	$(CC) $(CFLAGS) $(LIBFLAGS) -DBASE=2 lib1.c convert.c -o $@

lib2.so: lib2.c convert.c lab.h
	$(CC) $(CFLAGS) $(LIBFLAGS) -DBASE=3 lib2.c convert.c -o $@

linked: linked.c commands.c lab.h lib1.so
	$(CC) $(CFLAGS) linked.c commands.c -L. -l1 $(RPATH) -o $@

runtime: runtime.c commands.c lab.h
	$(CC) $(CFLAGS) runtime.c commands.c $(DL) -o $@

.PHONY: all
