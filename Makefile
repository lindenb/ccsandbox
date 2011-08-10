CPP=g++
CC=gcc
CFLAGS=-Wall -O3
all:bin
	(cd src; make)

bin:
	mkdir -p bin
clean:
	rm -rf bin; (cd src; make clean)
