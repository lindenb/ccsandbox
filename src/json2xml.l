%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json2xml.tab.h"

static const char* copy(const char* s,size_t length)
	{
	char* p= malloc(sizeof(char)*(length+1));
	if(p==NULL)
		{
		fprintf(stderr,"out of memory");
		exit(EXIT_FAILURE);
		}
	strncpy(p,s,length);
	p[length]=0;
	return p;
	}

%}



%%

true return TRUE;
false return FALSE;
[0-9]+ { yylval.s = copy(yytext,yyleng); return INTEGER;}
\[	return '[';
\]	return ']';
\:	return ':';
\{	return '{';
\}	return '}';
,	return ',';
\"[^\"]*\"|\'[^\']*\'	{
		yylval.s = copy(&yytext[1],yyleng-2);
		return STRING;
		}
[ \n\t\r] ;/* ignore */

%%