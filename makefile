CC = g++
CFLAGS = -g -Wall
LFLAGS = -pthread -lpthread -lncurses
OBJ = $(SRC:.c=.o)

all: server client

server: server.o serverUI.o userInterface.o util.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

client: client.o serverUI.o userInterface.o util.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm server client *.o

love:
	make clean
	make server
