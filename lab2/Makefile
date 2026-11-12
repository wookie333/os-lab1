CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic

lab2: main.c
	$(CC) $(CFLAGS) main.c -o lab2 -pthread -lm

.PHONY: test
test: lab2
	python3 test.py
