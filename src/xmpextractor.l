%{
/**
 * Author:
 *      Pierre Lindenbaum PhD
 * WWW
 *      http://plindenbaum.blogspot.com
 * Motivation:
 *       extract XMP fragments from any kind of file
 * Compilation:
 *       flex -f -B --read  xmpextractor.l
 *       gcc -Wall -o xmpextractor lex.yy.c
 * Example:
 *       curl -s "http://www.pnas.org/content/108/8/3130.full.pdf" | ./xmpextractor 
 */
#include <stdlib.h>
static char* encoding=NULL;


%}

%option noyywrap
%option 8bit
%option batch
%option never-interactive

%s xmp

prefix        [a-zA-Z_][a-zA-Z_0-9]*
ws            [ \t]
xmpmeta       {prefix}\:xmpmeta

%%



<xmp>{


\<\/{xmpmeta}{ws}*>  {
               fputs(yytext,stdout);
               fputc('\n',stdout);
               BEGIN(INITIAL);
               }
.|\n  {
   fputc(yytext[0],stdout);
   }
}

\<{xmpmeta}({ws}|\>)  {
     
      fputs(yytext,stdout);
      BEGIN(xmp);
      }

.|\n   ;

%%


int main(int argc,char** argv)
         {
         int optind=1;
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        fprintf(stderr,"%s: Pierre Lindenbaum PHD. 2011.\n",argv[0]);
                        fprintf(stderr,"Compilation: %s at %s.\n",__DATE__,__TIME__);
                        fprintf(stderr," -e <encoding> default=\"UTF-8\"\n");
                        exit(EXIT_FAILURE);
                        }
                else if(strcmp(argv[optind],"-e")==0 || strcmp(argv[optind],"--encoding")==0)
                        {
                        encoding=argv[++optind];
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
       
        if(optind==argc)
                {
                yyin=stdin;
                printf("<?xml version=\"1.0\" encoding=\"%s\"?>\n",
                  (encoding==NULL?"UTF-8":encoding)
                  );
                fputs("<XMP>",stdout);
                yylex();
                fputs("</XMP>\n",stdout);
		}
        else if(optind+1==argc)
               {
               yyin=fopen(argv[optind],"rb");
               if(yyin==NULL)
                        {
                        fprintf(stderr,
                           "Cannot open %s : %s\n",
                           argv[optind],
                           strerror(errno)
                           );
                        exit( EXIT_FAILURE);
                        }
               printf("<?xml version=\"1.0\" encoding=\"%s\"?>\n",
                  (encoding==NULL?"UTF-8":encoding)
                  );
               fputs("<XMP>",stdout);
               yylex();
               fputs("</XMP>\n",stdout);
               fclose(yyin);
               yyin=NULL;
               }
       else
         {
         fprintf(stderr,"Illegal number of arguments.\n");
         }
        return EXIT_SUCCESS;
        }