#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
static char* eol=NULL;

static void run(FILE* in)
	{
	int at_begin=1;
	int c=0;
	while((c=fgetc(in))!=EOF)
		{
		if(at_begin==1) fputc('\"',stdout);
		if(c!='\n') at_begin=0;
		switch(c)
			{
			case '\n':
				{
				fputs("\\n\"",stdout);
				if(eol!=NULL)  fputs(eol,stdout);
				fputc('\n',stdout);
				at_begin=1;
				break;
				}
			case '\r': fputs("\\r",stdout);break;
			case '\b': fputs("\\b",stdout);break;
			case '\'': fputs("\\\'",stdout);break;
			case '\"': fputs("\\\"",stdout);break;
			case '\t': fputs("\\t",stdout);break;
			case '\\': fputs("\\\\",stdout);break;
			default: fputc(c,stdout);break;
			}
		}
	if(at_begin!=1)
		{
		fputs("\"\n",stdout);
		}
	}

int main(int argc,char** argv)
         {
         int optind=1;
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        fprintf(stderr,"%s: Pierre Lindenbaum PHD. 2010.\n",argv[0]);
                        fprintf(stderr,"Compilation: %s at %s.\n",__DATE__,__TIME__);
                        fprintf(stderr," -n <string> trailing string (default empty) \n");
                        exit(EXIT_FAILURE);
                        }
                else if(strcmp(argv[optind],"-n")==0 && optind+1<argc)
                        {
                        eol=argv[++optind];
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
                run(stdin);
                }
        else
                {
                while(optind< argc)
                        {
                        FILE* in=NULL;
                        char* fname=argv[optind++];
                        errno=0;
			in=fopen(fname,"r");
			if(in==NULL)
				{
				fprintf(stderr,"Cannot open %s : %s\n",fname,strerror(errno));
				exit( EXIT_FAILURE);
				}
			
			run(in);
			fclose(in);
                        }
                }
        return EXIT_SUCCESS;
        }


