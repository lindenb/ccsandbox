%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json2xml.tab.h"

static  void* saferealloc(void *ptr, size_t size)
	{
	char* p= realloc(ptr,sizeof(char)*(size));
	if(p==NULL)
		{
		fprintf(stderr,"out of memory");
		exit(EXIT_FAILURE);
		}
	return p;
	}
static char* copy(const char* s,size_t length)
	{
	char* p= saferealloc(NULL,sizeof(char)*(length+1));
	strncpy(p,s,length);
	p[length]=0;
	return p;
	}
	
static char* append(const char* s2)
	{
	size_t len=(yylval.s==NULL?0:strlen(yylval.s));
	size_t len2= strlen(s2);
	yylval.s= saferealloc( yylval.s,sizeof(char)*(len+len2+1));
	strncpy(&yylval.s[len],s2,len2);
	yylval.s[len+len2]=0;
	return yylval.s;
	}

%}

%s apos
%s quot


%%
<quot>{
\\\"	append("\"");
\'	append("\'");
\"	{BEGIN(INITIAL);return STRING;}
}

<apos>{
\\\'	append("\'");
\"	append("\"");
\'	{BEGIN(INITIAL);return STRING;}
}

<apos,quot>{
\\n	append("\n");
\\t	append("\t");
\\\\	append("\\");
([^\"\'\\]|\n)+	append(yytext);
	}

null return NIL;
true return TRUE;
false return FALSE;
[\+|\-]?[0-9]+\.[0-9]+([e|E][0-9]+)? { yylval.s = copy(yytext,yyleng); return FLOATING;}
[\+|\-]?[0-9]+ { yylval.s = copy(yytext,yyleng); return INTEGER;}
[\[\]\:\{\},]	return yytext[0];
\"	{yylval.s=NULL; BEGIN(quot); }
\'	{yylval.s=NULL; BEGIN(apos); }
[ \n\t\r]+ ;/* ignore */
.	{
	fprintf(stderr,"Bad input \"%s\".\n",yytext);
	exit(EXIT_FAILURE);
	}
%%