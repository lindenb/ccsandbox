/*
 * Date:
 *	Aug 10, 2011
 * Author:
 *	Pierre Lindenbaum PhD
 * Motivation:
 *	escapes a HTTP URI
 * WWW:
 *	http://plindenbaum.blogspot.com
 * Contact:
 *	plindenbaum@yahoo.fr
 * Compilation:
 *	gcc -o escapeuri -Wall `curl-config --cflags  --libs ` escapeuri.c
 * Example:
 *	$ echo "Hello world" | escapeuri
 */#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static void escape(CURL *curl_handle,const char* s,size_t len)
    {
    char* escaped=curl_easy_escape(curl_handle ,s,len);
     if(escaped==NULL)
   		{
		fprintf(stderr,"Cannot escape %50s...\n",s);
   		return;
   		}
    fputs(escaped,stdout);
    fputc('\n',stdout);
    curl_free(escaped);
    }

int main(int argc,char**  argv)
    {
    int optind=1;
    CURL *curl_handle;

    /* loop over args */
    while(optind < argc)
    	{
    	if(strcmp(argv[optind],"-h")==0)
    	    {
    	    printf("%s: Pierre Lindenbaum PHD. 2011.\n",argv[0]);
    	    printf("Compilation: %s at %s\n",__DATE__,__TIME__);
    	    return EXIT_SUCCESS;
    	    }
    	else if(strcmp(argv[optind],"--")==0)
    	    {
    	    ++optind;
    	    break;
    	    }
    	else if(argv[optind][0]=='-')
    	    {
    	    fprintf(stderr,"unknown option '%s'\n",argv[optind]);
    	    return(EXIT_FAILURE);
    	    }
    	else
    	    {
    	    break;
    	    }
    	++optind;
    	}
    curl_global_init(CURL_GLOBAL_ALL);
    /* init the curl session */
    curl_handle = curl_easy_init();
    if(optind==argc)
	{
	size_t buffer_len=BUFSIZ;

        char* p=(char*)malloc(buffer_len);
        if(p==NULL)
            {
	    fputs("Out of memory.\n",stderr);
	    return EXIT_FAILURE;
            }
        while(!feof(stdin))
            {
	    int c;
	    size_t len=0;
	    while((c=fgetc(stdin))!=EOF && c!='\n')
		{
		if(len+2>=buffer_len)
		    {
		    buffer_len+=BUFSIZ;
		    p=(char*)realloc(p,buffer_len*sizeof(char));
		    if(p==NULL)
			{
			fputs("Out of memory.\n",stderr);
			return EXIT_FAILURE;
			}
		    }
		p[len++]=c;
		}
	    p[len]='\0';
	    escape(curl_handle,p,len);
            } 
	}
    else
	{
	while(optind<argc)
	    {
	    escape(curl_handle,argv[optind],strlen(argv[optind]));
	    optind++;
	    }
	}
      /* cleanup curl stuff */
      curl_easy_cleanup(curl_handle);




       /* we're done with libcurl, so clean it up */
       curl_global_cleanup();
       return EXIT_SUCCESS;
       }
