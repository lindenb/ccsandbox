CFLAGS=-Wall -I../../cclindenb/src/core
CC=gcc
CPP=g++
FLEX=flex
all:	../bin/mean ../bin/json2xml \
	../bin/fasta2term ../bin/text2html \
	../bin/text2c ../bin/xmlpath \
	../bin/opensearch \
	../bin/escapeuri

../bin/escapeuri:escapeuri.c
	$(CC) -o $@ -Wall `curl-config --cflags  --libs ` $< 
../bin/opensearch:opensearch.cpp
	$(CPP) -o $@ -Wall `curl-config --cflags  --libs ` `xml2-config --cflags  --libs` $< 
../bin/mean:mean.c ../bin
	$(CC) -o $@ $(CFLAGS) $<
../bin/geneticpainting01:geneticpainting01.cpp
	$(CPP) -o $@ -O3 $<  -lgd -lpthread
	
../bin/text2html:text2html.l Makefile
	flex  text2html.l
	$(CC) $(CFLAGS) -o $@ lex.yy.c

../bin/text2c:text2c.c
	$(CC) $(CFLAGS) -o $@ $<

../bin/json2xml:json2xml.l json2xml.y Makefile
	flex  json2xml.l
	bison -d json2xml.y
	$(CC) $(CFLAGS) -o $@ json2xml.tab.c lex.yy.c

../bin/fasta2term:fasta2term.c
	$(CC) $(CFLAGS) -o $@ fasta2term.c

../bin/xmlpath:xmlpath.cpp
	$(CPP) $(CFLAGS) -o $@ xmlpath.cpp `xml2-config --cflags --libs`

../bin/xmpextractor:xmpextractor.l
	$(FLEX) -f -B --read  $< 
	$(CC) -o $@ lex.yy.c
	rm lex.yy.c
../bin/syntaxhighlighter:syntaxhighlighter.l
	flex -o lex.yy.cc $<
	g++ -o $@ src/lex.yy.cc -lfl
	
a.out:jsonql.cpp jsonql.h json2xml.l
	flex  json2xml.l
	$(CPP)  -o $@ jsonql.cpp lex.yy.c -lz
	
test:curvi

../bin:
	mkdir $@

curvi:curviligne.cpp
	g++ -o a.out curviligne.cpp $(CFLAGS) `gdlib-config --cflags --libs ` -lm
	./a.out
align01:aligner01.cpp
	g++ -Wall aligner01.cpp -lz
	./a.out -r ${HOME}/nsp3.fasta ${HOME}/jeter.fa
	
