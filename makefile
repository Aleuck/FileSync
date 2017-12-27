CC = g++
CFLAGS = -std=c++11 -Wall -g
#LFLAGS = -lpthread -lrt -lncurses -ltinfo
LFLAGS = -lpthread -lrt -lncurses -lssl -lcrypto
OBJ = $(SRC:.c=.o)

all: server client

server: server.o serverUI.o userInterface.o util.o tcp.o
	$(CC) -o $@ $^ $(LFLAGS)

client: client.o clientUI.o userInterface.o util.o tcp.o
	$(CC) -o $@ $^ $(LFLAGS)

testtcp: testtcp.o util.o tcp.o
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm server client *.o


love:
	make clean
	make server
