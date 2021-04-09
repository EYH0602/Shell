CC = gcc
FLAGS = -Wall -Werror

all: wish

wish: wish.c
	$(CC) -o wish wish.c $(FLAGS)

clean:
	rm -rf *.o wish
