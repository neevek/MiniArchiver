CC=gcc
CFLAGS=-c -Wall
LIBS=-lz
BIN=mar
INSTALL_DIR=/usr/local/bin
UNAME_S := $(shell uname -s | cut -d- -f1)

ifeq ($(UNAME_S), CYGWIN_NT)
	LIBS = -static -lz	# statically link against zlib when built under cygwin
else
	LIBS = -lz
endif

all: ${BIN}

mar: mar.o
	${CC} mar.o ${LIBS} -o ${BIN}

mar.o: mini_archiver.c
	${CC} ${CFLAGS} -o mar.o mini_archiver.c

install:
	cp ${BIN} ${INSTALL_DIR}/${BIN}

clean: 
	rm -f *.o mar
