CC=gcc
CFLAGS=-c -Wall

all: mar

mar: mar.o
	${CC} -lz -o mar mar.o

mar.o: mini_archiver.c
	${CC} ${CFLAGS} -o mar.o mini_archiver.c

clean: 
	rm -f *.o mar

test: all 
	./mar
