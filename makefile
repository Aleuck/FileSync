CC = g++
CFLAGS = -std=c++11 -Wall -g
LFLAGS = -pthread -lpthread -lncurses -ltinfo
OBJ = $(SRC:.c=.o)

all: server client

server: server.o serverUI.o userInterface.o util.o tcp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

client: client.o serverUI.o userInterface.o util.o tcp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

testtcp: testtcp.o util.o tcp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm server client testtcp *.o


love:
	make clean
	make server
