/*
 * opensearch.cpp
 *
 * Date:
 *	Aug 10, 2011
 * Author:
 *	Pierre Lindenbaum PhD
 * Motivation:
 *	opensearch for mediawiki/wikipedia API
 * WWW:
 *	http://plindenbaum.blogspot.com
 * Contact:
 *	plindenbaum@yahoo.fr
 * Compilation:
 *	g++ -o opensearch -Wall `curl-config --cflags  --libs ` `xml2-config --cflags  --libs` opensearch.cpp
 * Example:
 *	$ bin/opensearch Charles D
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

using namespace std;

class XMLHandler
	{
	public:
		CURL *curl_handle;
		bool print_desc;
		bool print_color;
		string* content;
		string text;
		string desc;
		int columns;
		XMLHandler():print_desc(true),print_color(true),content(NULL),columns(80)
			{
			}
	};





static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
  {
  string *mem = (string*)data;
  mem->append((char*)ptr,nmemb);
  return size * nmemb;
  }

/** called when an TAG is opened */
static void startElement(void * ctx,
	const xmlChar * localname,
	const xmlChar * prefix,
	const xmlChar * URI,
	int nb_namespaces,
	const xmlChar ** namespaces,
	int nb_attributes,
	int nb_defaulted,
	const xmlChar ** attributes)
    {
    XMLHandler* state=(XMLHandler*)ctx;
    if(strcmp((const char*)localname,"Text")==0 ||
       (state->print_desc && strcmp((const char*)localname,"Description")==0))
	{
	state->content=new string;
	}
    }

static char underscore(char c)
    {
    return c==' '?'_':c;
    }

/** called when an TAG is closed */
static void endElement(void * ctx,
	const xmlChar * localname,
	const xmlChar * prefix,
	const xmlChar * URI)
	{
	XMLHandler* state=(XMLHandler*)ctx;
	if(strcmp((const char*)localname,"Text")==0)
	    {
	    state->text.assign(*(state->content));
	    }
	else if(state->print_desc && strcmp((const char*)localname,"Description")==0)
	    {
	    state->desc.assign(*(state->content));
	    }
	else if(strcmp((const char*)localname,"Item")==0)
	    {
	    transform(state->text.begin(),
		       state->text.end(),
		       state->text.begin(),
		       underscore
			);
	    char* escaped=curl_easy_escape(state->curl_handle ,state->text.c_str(),state->text.size());
	    if(escaped!=NULL)
		{
		string uri(escaped);
		string::size_type i=0;
		while((i=uri.find("%5F",i))!=string::npos)
		    {
		    uri.replace(i,3,"_");
		    }

		bool istt=(state->print_color && isatty(fileno(stdout)));
		if(istt) fputs("\x1b[1m",stdout);
		fputs(uri.c_str(),stdout);
		if(istt) fputs("\x1b[22m",stdout);
		if(state->print_desc)
		    {
		    i=0;
		    while((i=state->desc.find("\n",i))!=string::npos)
			{
			state->desc.replace(i,1," ");
			}
		    if((int)state->desc.size()+3+(int)uri.length() > state->columns)
			{
			int n= max(0,(int)(state->columns-(6+uri.size())));
			state->desc.erase(state->desc.begin()+n,state->desc.end());
			state->desc.append("...");
			}

		    fputs("   ",stdout);
		    if(istt) fputs("\x1b[3m",stdout);
		    fputs(state->desc.c_str(),stdout);
		    if(istt) fputs("\x1b[23m",stdout);
		    }
		fputc('\n',stdout);
		curl_free(escaped);
		}
	    else
		{
		fprintf(stderr,"Cannot escape string\n");
		}
	    state->text.clear();
	    state->desc.clear();
	    }
	if(state->content!=NULL)
	    {
	    delete state->content;
	    state->content=NULL;
	    }
	}


static void handleCharacters(void * ctx, const xmlChar * ch, int len)
	{
	XMLHandler* state=(XMLHandler*)ctx;
	if(state->content==NULL) return;
	state->content->append((char*)ch,len);
	}



int main(int argc,char**  argv)
    {
    int optind=1;
    CURL *curl_handle;
    string xmlStr;
    string base("http://en.wikipedia.org/w/api.php");
    string count("20");
    string uri;
    XMLHandler state;


    LIBXML_TEST_VERSION;

    char* cols=getenv("COLUMNS");
    if(cols!=NULL)
    	{
    	state.columns=max(atoi(cols),80);
    	}


    /* loop over args */
    while(optind < argc)
	{
	if(strcmp(argv[optind],"-h")==0)
	    {
	    cout << argv[0] << ": Pierre Lindenbaum PHD. 2011." << endl;
	    cout << "Compilation: "<< __DATE__ << " at " << __TIME__ << "." << endl;
	    cout  << "options:" << endl;
	    cout  << " -base <uri> default:"<< base << endl;
	    cout  << " -n <int> . Max number of records returned. default:"<< count << endl;
	    cout  << " -C <int> . columns. default:"<< state.columns << endl;
	    cout  << " -d Don't print description\n";
	    cout  << " -c Don't print colors/style\n";
	    return EXIT_SUCCESS;
	    }
	else if(strcmp(argv[optind],"-base")==0 && optind+1<argc)
	    {
	    base.assign(argv[++optind]);
	    }
	else if(strcmp(argv[optind],"-C")==0 && optind+1<argc)
	    {
	    state.columns=max(atoi(argv[++optind]),80);
	    }
	else if(strcmp(argv[optind],"-d")==0)
	    {
	    state.print_desc=false;
	    }
	else if(strcmp(argv[optind],"-c")==0)
	    {
	    state.print_color=false;
	    }
	else if(strcmp(argv[optind],"-n")==0 &&
		optind+1<argc &&
		atoi(argv[optind+1])>0)
	    {
	    count.assign(argv[++optind]);
	    }
	else if(strcmp(argv[optind],"--")==0)
	    {
	    ++optind;
	    break;
	    }
	else if(argv[optind][0]=='-')
	    {
	    cerr << "unknown option '"<<argv[optind] << "'" << endl;
	    return(EXIT_FAILURE);
	    }
	else
	    {
	    break;
	    }
	++optind;
	}


    if(optind==argc) return EXIT_SUCCESS;

    curl_global_init(CURL_GLOBAL_ALL);


    /* init the curl session */
    curl_handle = curl_easy_init();

    uri.append(base);
    uri.append("?action=opensearch&format=xml&limit=");
    uri.append(count);
    uri.append("&search=");
    for(int i=optind; i< argc;++i)
	{
	if(i!=optind) uri.append("+");
	char* escaped=curl_easy_escape(curl_handle ,argv[i],strlen(argv[i]));
	if(escaped==NULL)
	    {
	   cerr << "cannot escape '"<<argv[i] << "'" << endl;
	   return(EXIT_FAILURE);
	    }
	uri.append(escaped);
	curl_free(escaped);
	}

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL,uri.c_str());

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&xmlStr);

    /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");


    /* get it! */
    curl_easy_perform(curl_handle);




    state.curl_handle=curl_handle;
    
    
   
   	 
    
    xmlSAXHandler handler;
    memset(&handler,0,sizeof(xmlSAXHandler));
    handler.startElementNs= startElement;
    handler.endElementNs=endElement;
    handler.characters=handleCharacters;
    handler.initialized = XML_SAX2_MAGIC;

    xmlSAXUserParseMemory(&handler,&state,xmlStr.c_str(),xmlStr.size());

    xmlCleanupParser();
    xmlMemoryDump();


    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);




    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();
    return EXIT_SUCCESS;
    }
