CFLAGS=-Wall -I../../cclindenb/src/core
CC=gcc

../bin/json2xml:json2xml.l json2xml.y
	flex -l json2xml.l
	bison -d json2xml.y
	$(CC) $(CFLAGS) -o $@ json2xml.tab.c lex.yy.c

test:curvi

curvi:curviligne.cpp
	g++ -o a.out curviligne.cpp $(CFLAGS) `gdlib-config --cflags --libs ` -lm
	./a.out
	