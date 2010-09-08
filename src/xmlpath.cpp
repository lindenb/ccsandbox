
/**
 * Author:
 *	Pierre Lindenbaum PhD
 * Contact:
 *	plindenbaum@yahoo.fr
 * WWW:
 *	http://plindenbaum.blogspot.com
 * Motivation:
 *	small tool to help me to find a correct XPATH expression
 */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <vector>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

using namespace std;
class Tag
	{
	public:
		string qName;
		bool hasChildren;
		
		Tag(string qName):qName(qName),hasChildren(false)
			{
			}
	};

class XMLHandler
	{
	public:
		vector<Tag*> stack;
		string* content;
		XMLHandler():content(NULL)
			{
			}
	};

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
	if(state->content!=NULL)
		{
		delete state->content;
		state->content=NULL;
		}
	string qName;
	if(prefix!=NULL && strlen((char*)prefix)>0)
		{
		qName.append((char*)prefix).append(":");
		}
	qName.append((char*)localname);
	Tag* tag=new Tag(qName);
	if(!state->stack.empty())
		{
		state->stack.back()->hasChildren=true;
		}
	state->stack.push_back(tag);
	
	for(int i=0;i< nb_attributes;++i)
		{
		
		int len=(char*) attributes[i*5+4]-(char*) attributes[i*5+3];
		string att((char*) attributes[i*5+3],len);
		for(vector<string>::size_type n=0;
			n< state->stack.size();
			++n)
			{
			cout << "/"<< state->stack.at(n)->qName;
			}
		cout << "/@" << ((char*) attributes[i*5]) << "\t" << att << endl;
		}
	}


/** called when an TAG is closed */
static void endElement(void * ctx,
	const xmlChar * localname,
	const xmlChar * prefix,
	const xmlChar * URI)
	{
	XMLHandler* state=(XMLHandler*)ctx;
	for(vector<string>::size_type n=0;
			n< state->stack.size();
			++n)
			{
			cout << "/"<< state->stack.at(n)->qName;
			}
	if(state->content!=NULL && !state->stack.back()->hasChildren)
		{
		cout << "\t" <<  *(state->content);
		}
	cout << endl;
	delete state->stack.back();
	state->stack.pop_back();
	if(state->content!=NULL)
		{
		delete state->content;
		state->content=NULL;
		}
	}

static void handleCharacters(void * ctx, const xmlChar * ch, int len)
	{
	XMLHandler* state=(XMLHandler*)ctx;
	if(state->stack.empty()) return;
	if(state->stack.back()->hasChildren) return;
	if(state->content==NULL) state->content=new string;	
	state->content->append((char*)ch,len);
	}




int main(int argc,char** argv)
         {
         int optind=1;
	 LIBXML_TEST_VERSION
	/* loop over args */
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        cerr << argv[0] << ": Pierre Lindenbaum PHD. 2010." << endl;
                        cerr << "Compilation: "<< __DATE__ << " at " << __TIME__ << "." << endl;
                      	
                        return(EXIT_FAILURE);
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
	
	XMLHandler state;
	
	xmlSAXHandler handler;
	memset(&handler,0,sizeof(xmlSAXHandler));
	handler.startElementNs= startElement;
	handler.endElementNs=endElement;
	handler.characters=handleCharacters;
	handler.initialized = XML_SAX2_MAGIC;
        if(optind==argc)
                {
                int res=xmlSAXUserParseFile(&handler,&state,"-");
		if(res!=0)
			{
			std::cerr << "Error "<< res << " : stdin" << std::endl;
			return EXIT_FAILURE;
			}
                }
        else
        	{
		while(optind <argc)
			{	
			char* fname=argv[optind++];
			int res=xmlSAXUserParseFile(&handler,&state,fname);
	
			if(res!=0)
				{
				std::cerr << "Error "<< res << " : " << fname<< std::endl;
				return EXIT_FAILURE;
				}
			}
		}
	xmlCleanupParser();
	xmlMemoryDump();
	return EXIT_SUCCESS;
        }


