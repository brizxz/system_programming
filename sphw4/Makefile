CC = gcc
CFLAGS = -I ./ -pthread -std=c11 -O2
FILE = my_pool.c
OBJ = sample bench

all: $(OBJ)

%: testcases/%/main.c $(FILE)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ)

