bin/syntaxhighlighter:bin src/syntaxhighlighter.l
	flex -o src/lex.yy.cc src/syntaxhighlighter.l
	g++ -o $@ src/lex.yy.cc -lfl
bin:
	mkdir -p bin