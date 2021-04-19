CC = g++
FLAGS = -Wall -Werror

all: wish

wish: wish.cpp
	$(CC) -o wish wish.cpp $(FLAGS)

new: wish_new.cpp
	$(CC) -o wish wish_new.cpp $(FLAGS)

passed: wish_passed.c
	gcc -o wish wish_passed.c $(FLAGS)

clean:
	rm -rf *.o wish


