CC = g++
FLAGS = -Wall -Werror # -D_SHOW_COMMAND_INFO_

all: wish

wish: wish.cpp
	$(CC) -o wish wish.cpp $(FLAGS)

clean:
	rm -rf *.o wish


