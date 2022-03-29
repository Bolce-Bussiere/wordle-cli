CC = gcc
CFLAGS = -Wall -O3

all: main.c hashmap.c reader.c
	$(CC) $(CFLAGS) main.c hashmap.c reader.c -o wordle

clean:
	rm -f wordle
