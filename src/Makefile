CFLAGS=-Wall -I../../cclindenb/src/core
CC=gcc
all:../bin/json2xml ../bin/fasta2term
	
	

../bin/json2xml:json2xml.l json2xml.y Makefile
	flex -l  json2xml.l
	bison -d json2xml.y
	$(CC) $(CFLAGS) -o $@ json2xml.tab.c lex.yy.c

../bin/fasta2term:fasta2term.c
	$(CC) $(CFLAGS) -o $@ fasta2term.c
	
test:curvi

curvi:curviligne.cpp
	g++ -o a.out curviligne.cpp $(CFLAGS) `gdlib-config --cflags --libs ` -lm
	./a.out
	