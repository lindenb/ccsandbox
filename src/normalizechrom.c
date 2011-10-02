/**
 * Author:
 *	Pierre Lindenbaum PhD
 * Contact:
 *	plindenbaum@yahoo.fr
 * Date:
 *	Oct 2011
 * WWW:
 *	http://plindenbaum.blogspot.com
 * Motivation:
 *	normlize chrom column to/from UCSC/ENSEMBL
 * Compilation:
 *	 g++ -o normalizechrom -Wall -O3 normalizechrom.cpp -lz
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

typedef struct
	    {
	    char delim;
	    char* ignore;
	    FILE* in;
	    int chrom_col;
	    int to_ucsc;
	    }NormalizeChrom;
static void normalize(NormalizeChrom* app)
    {
    char* line=malloc(BUFSIZ);
    size_t buffer=BUFSIZ;
    size_t size=0;
    size_t len_ignore=(app->ignore==NULL?0:strlen(app->ignore));
    char delim[2]={app->delim,0};
    int c;
    char* p;
    int column;
    if(line==NULL)
	{
	fprintf(stderr,"Out of memory.\n");
	exit(EXIT_FAILURE);
	}
    while(!feof(app->in))
	{
	size=0;
	while((c=fgetc(app->in))!=EOF && c!='\n')
	    {
	    if(size+2<=buffer)
		{
		buffer=(buffer==0?BUFSIZ:buffer*2);
		if((line=(char*)realloc(line,buffer*sizeof(char)))==NULL)
		    {
		    fprintf(stderr,"Out of memory.\n");
		    exit(EXIT_FAILURE);
		    }
		}
	    line[size++]=c;
	    }
	line[size]=0;
	if(app->ignore!=NULL && strncmp(line,app->ignore,len_ignore)==0)
	    {
	    fputs(line,stdout);
	    fputc('\n',stdout);
	    continue;
	    }
	column=0;
	p=strtok(line,delim);
	while(p!=NULL)
	    {
	    if(column>0) fputs(delim,stdout);
	    if(column==app->chrom_col)
		{
		char *p2=p;
		while(*p2!=0) {*p2=tolower(*p2);++p2;}
		while(*p!=0 && isspace(*p)) p++;
		if(strncmp(line,"chrom",5)==0)
		    {
		    p+=5;
		    }
		else if(strncmp(line,"chr",3)==0)
		    {
		    p+=3;
		    }
		else if(strncmp(line,"k",1)==0)
		    {
		    p++;
		    }
		while(*p!=0 && isspace(*p)) p++;
		if(app->to_ucsc==1)
		    {
		    if(strcmp(p,"x")==0)
			{
			fputs("chrX",stdout);
			}
		    else if(strcmp(p,"y")==0)
			{
			fputs("chrY",stdout);
			}
		    else if(strcmp(p,"m")==0 || strcmp(p,"mt")==0)
			{
			fputs("chrM",stdout);
			}
		    else
			{
			fprintf(stdout,"chr%s",p);
			}
		    }
		else /* to ENSEMBL */
		    {
		    if(strcmp(p,"x")==0)
			{
			fputs("X",stdout);
			}
		    else if(strcmp(p,"y")==0)
			{
			fputs("Y",stdout);
			}
		    else if(strcmp(p,"m")==0 || strcmp(p,"mt")==0)
			{
			fputs("MT",stdout);
			}
		    else
			{
			fputs(p,stdout);
			}
		    }
		}
	    else
		{
		fputs(p,stdout);
		}
	    ++column;
	    p=strtok(NULL,delim);
	    }
	fputc('\n',stdout);
	}
    }
int main(int argc,char** argv)
	{
	NormalizeChrom app;
	memset((void*)&app,0,sizeof(NormalizeChrom));
	app.delim='\t';
	app.chrom_col=0;
	app.to_ucsc=1;
	int optind=1;
	while(optind < argc)
	    {
	    if(strcmp(argv[optind],"-h")==0)
		    {
		    fprintf(stderr,"%s: Pierre Lindenbaum PHD. October 2011.\nNormalize a chromosome name to/from UCSC/Ensembl\n",argv[0]);
		    fprintf(stderr,"Compilation: %s at %s.\n",__DATE__,__TIME__);
		    fprintf(stderr,"Options:\n");
		    fprintf(stderr," -i <string> ignore lines starting with this string.\n");
		    fprintf(stderr," -d <string> column delimiter (default:tab).\n");
		    fprintf(stderr," -c <int> column index (+1) (default:%d).\n",app.chrom_col+1);
		    fprintf(stderr," -E convert to ENSEMBL syntax (default is UCSC).\n");
		    fprintf(stderr,"\n(stdin|files)\n\n");
		    exit(EXIT_FAILURE);
		    }
	    else if(strcmp(argv[optind],"-d")==0 && optind+1< argc)
		{
		char* p=argv[++optind];
		if(strlen(p)!=1)
		    {
		    fprintf(stderr,"Bad delimiter \"%s\"\n",p);
		    return (EXIT_FAILURE);
		    }
		app.delim=p[0];
		}
	    else if(strcmp(argv[optind],"-i")==0 && optind+1<argc)
		{
		app.ignore=argv[++optind];
		}
	    else if(strcmp(argv[optind],"-E")==0 )
		{
		app.to_ucsc=0;
		}
	    else if(strcmp(argv[optind],"-c")==0 && optind+1<argc)
		{
		char* p2;
		app.chrom_col=(int)strtol(argv[++optind],&p2,10);
		if(*p2!=0 || app.chrom_col<1)
		    {
		    fprintf(stderr,"Bad chromosome column:%s\n",argv[optind]);
		    return EXIT_FAILURE;
		    }
		app.chrom_col--;/* to 0-based */
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
		app.in=stdin;
		normalize(&app);
		}
	else
		{
		while(optind< argc)
			{
			errno=0;
			app.in=fopen(argv[optind],"r");
			if(app.in==NULL)
				{
				fprintf(stderr,"Cannot open \"%s\" : %s\n",argv[optind],strerror(errno));
				return EXIT_FAILURE;
				}
			normalize(&app);
			fclose(app.in);
			++optind;
			}
		}
	return EXIT_SUCCESS;
	}
