CC = g++
FLAGS = -g -Wall -pthread -lpthread -lncurses
OBJ = $(SRC:.c=.o)

all: server client

server: server.o serverUI.o userInterface.o util.o
	$(CC) -o $@ $^ $(FLAGS)

client: client.o serverUI.o userInterface.o util.o
	$(CC) -o $@ $^ $(FLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(FLAGS)

clean:
	rm server client *.o

love:
	make clean
	make server
