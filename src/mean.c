#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <float.h>
static int process(const char* filename,FILE* in)
	{
	long double total=0;
	long long count=0L;
	long double min= LDBL_MAX;
	long double max=-LDBL_MAX;
	while(!feof(in))
		{
		long double v;
		int status;
		if((status=fscanf(in,"%Lf\n",&v))!=1)
			{
			int c=0;
			clearerr(in);
			c=fgetc(in);
			if(c==EOF) break;
			fprintf(stderr,"Error skipping character \"%c\".\n",c);
			continue;
			}
		count++;
		total+=v;
		if(v<min) min=v;
		if(max<v) max=v;
		}
	if(filename!=0) printf("%s\t",filename);
	if(count!=0L)
		{
		printf("%Ld\t%Lf\t%Lf\t%Lf\t%Lf",count,total/count,total,min,max);
		}
	fputs("\n",stdout);
	return 0;
	}
int main(int argc,char** argv)
	{
	if(argc==1)
		{
		process(NULL,stdin);
		}
	else
		{
		int optind=1;
		while(optind<argc)
			{
			char* filename=argv[optind++];
			FILE* in=fopen(filename,"r");
			if(in==NULL)
				{
				fprintf(stderr,"Cannot open \"%s\" %s\n",
					filename,
					strerror(errno)
					);
				exit(EXIT_FAILURE);
				}
			process(filename,in);
			fclose(in);
			}
		}
	return 0;
	}
