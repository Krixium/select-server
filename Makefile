CC=gcc
CFLAGS=-Wall -ggdb
NAME=select-server.out
DEBUGNAME=select-server.out
LINKS=-lpthread

default: main.o server.o
	$(CC) $(CFLAGS) main.o server.o -o $(NAME) $(LINKS)

debug: main.o server.o
	$(CC) $(CFLAGS) main.o server.o -ggdb -O0 -o $(DEBUGNAME) $(LINKS)

main.o:
	$(CC) $(CFLAGS) -O -c main.c

server.o:
	$(CC) $(CFLAGS) -O -c server.c

clean:
	rm -f *.o *.txt *.bak $(NAME) $(DEBUGNAME)