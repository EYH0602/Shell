CC = g++
FLAGS = -Wall -Werror

all: wish

wish: wish.cpp
	$(CC) -o wish wish.cpp $(FLAGS)

dish: dish.cpp
	$(CC) -o dish dish.cpp $(FLAGS)

clean:
	rm -rf *.o wish


