%{
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
extern int yylex();
extern FILE* yyin;

static void escape(const char* p)
	{
	char *s=(char*)p;
	while(*s!=0)
		{
		switch(*s)
			{
			case '<': fputs("&lt;",stdout); break;
			case '>': fputs("&gt;",stdout); break;
			case '\'': fputs("&apos;",stdout); break;
			case '\"': fputs("&quot;",stdout); break;
			case '&': fputs("&amp;",stdout); break;
			default:fputc(*s,stdout);
			}
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
object_entry: STRING  {fputs("<property name=\"",stdout);escape($1);fputs("\">",stdout);free($1);} ':' any { fputs("</property>",stdout);};

%%

static void runfile(const char* fname)
	{
	errno=0;
	yyin=fopen(fname,"r");
	if(yyin==NULL)
		{
		fprintf(stderr,"Cannot open %s : %s\n",fname,strerror(errno));
		exit( EXIT_FAILURE);
		}
	yyparse();
	fclose(yyin);
	}


int main(int argc,char** argv)
	 {
	 int print_xml_header=0;
	 int optind=1;
         while(optind < argc)
		{
		if(strcmp(argv[optind],"-h")==0)
			{
			fprintf(stderr,"%s: Pierre Lindenbaum PHD. 2010.\n",argv[0]);
			fprintf(stderr,"Compilation: %s at %s.\n",__DATE__,__TIME__);
			fprintf(stderr," --header print XML header.\n");
			exit(EXIT_FAILURE);
			}
		else if(strcmp(argv[optind],"--header")==0)
			{
			print_xml_header=1;
			}
		else if(strcmp(argv[optind],"--")==0)
			{
			++optind;
			break;
			}
		else if(argv[optind][0]=='-')
			{
			fprintf(stderr,"unknown option '%s'\n",argv[optind]);
			exit(EXIT_FAILURE);
			}
		else
			{
			break;
			}
		++optind;
                }
        if(print_xml_header==1)
        	{
        	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",stdout);
        	}
        if(optind==argc)
                {
                yyin=stdin;
                yyparse();
                }
        else if(optind+1==argc)
                {
		runfile(argv[optind]);
                }
        else
        	{
        	fputs("<array>",stdout);
        	while(optind< argc)
        		{
        		runfile(argv[optind++]);
        		}
        	fputs("</array>",stdout);
                }
        fputc('\n',stdout);
        return EXIT_SUCCESS;
	}