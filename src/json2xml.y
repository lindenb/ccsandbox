%{
#include <stdio.h>

extern int yylex();

static void escape(const char* p)
	{
	char *s=(char*)p;
	while(*s!=0)
		{
		fputc(*s,stdout);
		++s;
		}
	}

int yywrap() { return 1;}
void yyerror(const char* s) {fprintf(stderr,"Error:%s.\n",s);}
%}

%union  {
	char* s;
	}


%token TRUE FALSE NIL   
%token<s> FLOATING
%token<s> INTEGER
%token<s> STRING
%start any
%%
any: boolean | array | object | string | nil | integer | floating;
boolean: TRUE {fputs("<bool>true</bool>",stdout);}
	| FALSE {fputs("<bool>false</bool>",stdout);}
	;
nil: NIL  {fputs("<null/>",stdout);} ;
integer: INTEGER {fprintf(stdout,"<integer>%s</integer>",$1); free($1);} ;
floating: FLOATING {fprintf(stdout,"<float>%s</float>",$1);free($1);} ;
string: STRING { fputs("<string>",stdout);escape($1);fputs("</string>",stdout); free($1); };
array: '[' {fputs("<array>",stdout);} array_items ']' {fputs("</array>",stdout);}  |
	'[' ']' { fputs("<array/>",stdout);}
	;
array_items: any | array_items ',' any;
object: '{'  '}' { fputs("<object/>",stdout);}
	| '{' {fputs("<object>",stdout);} object_items '}' {fputs("</object>",stdout);}
	;
object_items: object_entry | object_items ',' object_entry;
object_entry: STRING  {fprintf(stdout,"<property name=\"%s\">",$1);free($1);} ':' any { fputs("</property>",stdout);};

%%

int main(int argc,char** argv)
	{
	return yyparse();
	}