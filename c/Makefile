CC=gcc
CFLAGS=-c -Wall
LIBS=-lz
BIN=mar
UNAME_S := $(shell uname -s | cut -d- -f1)

ifeq ($(UNAME_S), CYGWIN_NT)
	LIBS = -static -lz
else
	LIBS = -lz
endif

all: ${BIN}

mar: mar.o
	${CC} mar.o ${LIBS} -o ${BIN}

mar.o: mini_archiver.c
	${CC} ${CFLAGS} -o mar.o mini_archiver.c

clean: 
	rm -f *.o mar

test: all 
	./mar
