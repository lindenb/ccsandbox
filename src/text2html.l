%{
#include <stdio.h>
#include <string.h>
#include <errno.h>
int crlf2br=0;
static void escape2(char* s,int len)
	{
	int i=0;
	for(i=0;i < len;++i)
		{
		switch(s[i])
			{
			case '&':fputs("&amp;",stdout);break;
			case '<':fputs("&lt;",stdout);break;
			case '>':fputs("&gt;",stdout);break;
			case '\'':fputs("&apos;",stdout);break;
			case '\"':fputs("&quot;",stdout);break;
			default:fputc(s[i],stdout);break;
			}
		}
	}
static void escape()
	{
	escape2(yytext,yyleng);
	}
%}
%option noyywrap

%%
((http[s]?|ftp)\:\/\/|mailto\:)[\?\&%a-zA-Z\+0-9_@=\.\:\/\#]+	{
	fputs("<a href=\"",stdout);
	escape();
	fputs("\">",stdout);
	escape();
	fputs("</a>",stdout);
	}
\>	fputs("&gt;",stdout);
\<	fputs("&lt;",stdout);
\&	fputs("&amp;",stdout);
\'	fputs("&apos;",stdout);
\"	fputs("&quot;",stdout);
\n	fputs((crlf2br==1?"<br/>":"\n"),stdout);
\r	;
.	fputc(yytext[0],stdout);

%%

int main(int argc,char** argv)
         {
         int optind=1;
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        fprintf(stderr,"%s: Pierre Lindenbaum PHD. 2010.\n",argv[0]);
                        fprintf(stderr,"Compilation: %s at %s.\n",__DATE__,__TIME__);
                        fprintf(stderr," --br|-n replace CR by <br/>.\n");
                        exit(EXIT_FAILURE);
                        }
                else if(strcmp(argv[optind],"--br")==0 || strcmp(argv[optind],"-n")==0)
                        {
                        crlf2br=1;
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
        fputs("<html><body>",stdout);
        if(optind==argc)
                {
                yyin=stdin;
                printf("\n<div>");
		yylex();
		printf("</div>\n");
                }
        else
                {
                while(optind< argc)
                        {
                        char* fname=argv[optind++];
                        errno=0;
			yyin=fopen(fname,"r");
			if(yyin==NULL)
				{
				fprintf(stderr,"Cannot open %s : %s\n",fname,strerror(errno));
				exit( EXIT_FAILURE);
				}
			
			printf("\n<h4>");
			escape2(fname,strlen(fname));
			printf("</h4><div>");
			yylex();
			printf("</div>\n");
			fclose(yyin);
                        }
                }
        fputs("</body></html>\n",stdout);
        return EXIT_SUCCESS;
        }


