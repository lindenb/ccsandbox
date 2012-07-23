#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc,char ** argv)
	{
	FILE* in;
	char c;
	char array[2];
	if(argc!=2) 
		{
		fprintf(stderr,"Usage %s file\n",argv[0]);
		return -1;
		}
	if((in=fopen(argv[1],"r+"))==NULL)
		{
		fprintf(stderr,"Cannot open \"%s\" %s\n",argv[1],strerror(errno));
		return EXIT_FAILURE;
		}
	if(fread((void*)array,sizeof(char),2,in)!=2)
		{
		fprintf(stderr,"Cannot read 2 bytes\n");
		return EXIT_FAILURE;
		}
	fseek(in,SEEK_SET,0L);
	c=array[0]; array[0]=array[1]; array[1]=c;
	fwrite(array,sizeof(char),2,in);
	fclose(in);
	return EXIT_SUCCESS;
	}
