/**
 * Author:
 *	Pierre Lindenbaum PhD
 * Contact:
 *	plindenbaum@yahoo.fr
 * WWW:
 *	http://plindenbaum.blogspot.com
 * Motivation:
 *	append a XML file at the end of another one
 */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

using namespace std;

#define DEFAULT_EVENTS(reader,writer)	\
case XML_READER_TYPE_CDATA: \
	{ \
	xmlTextWriterWriteCDATA(writer,xmlTextReaderConstValue(reader)); \
	break; \
	} \
case XML_READER_TYPE_SIGNIFICANT_WHITESPACE: \
case XML_READER_TYPE_TEXT: \
	{ \
	xmlTextWriterWriteString(writer,xmlTextReaderConstValue(reader)); \
	break; \
	} \
case XML_READER_TYPE_COMMENT: \
	{ \
	xmlTextWriterWriteComment(writer,xmlTextReaderConstValue(reader)); \
	break; \
	} \
default:\
	{\
	fprintf(stderr,"Node type (%d) not handled:\n",xmlTextReaderNodeType(reader));\
	exit(EXIT_FAILURE);\
	break;\
	}

struct State
	{
	xmlTextReaderPtr reader;
	xmlTextWriterPtr writer;
	char** argv;
	int argc;
	int optind;
	bool ignore_root;
	};

static void attributes(State* state,xmlTextReaderPtr reader)
	{
	xmlTextWriterPtr writer=state->writer;
	if(	xmlTextReaderHasAttributes(reader) &&
		xmlTextReaderMoveToFirstAttribute(reader)==1)
	    {
	    do
	    	{
	    	xmlTextWriterWriteAttribute(writer,
	    		xmlTextReaderName(reader),
	    		xmlTextReaderValue(reader)
	    		);		    		
	    	} while(xmlTextReaderMoveToNextAttribute(reader)==1);
	    }
	}

static int insertXMLFile(State* state,xmlTextReaderPtr in)
    {
    xmlTextWriterPtr writer=state->writer;
    switch(xmlTextReaderNodeType(in))
    	{
    	case XML_READER_TYPE_ELEMENT:
    		{
    		if(state->ignore_root && xmlTextReaderDepth(in)==0) break;
    		xmlTextWriterStartElement(writer,xmlTextReaderConstName(in));
    		int empty=xmlTextReaderIsEmptyElement(in);
    		attributes(state,in);
    		if(empty)
    			{
    			xmlTextWriterEndElement(writer);
    			}
       		break;
    		}
    	case XML_READER_TYPE_END_ELEMENT:
		{
		if(state->ignore_root && xmlTextReaderDepth(in)==0) break;
		xmlTextWriterEndElement(writer);
		break;
		}
    	DEFAULT_EVENTS(in,writer);
    	}
    return 0;
	}
	
	
	
	

static int insertXMLs(State* state)
	{
	int ret=0;
	for(int i=state->optind;i< state->argc;++i)
		{
		xmlTextReaderPtr in = xmlReaderForFile(state->argv[i], NULL, 0);
		if(in==NULL)
			{
			fprintf(stderr,"Cannot xmlReaderForFile \"%s\".\n",state->argv[i]);
			return -1;
			}
		while((xmlTextReaderRead(in))==1)
			{
			if((ret=insertXMLFile(state,in))!=0)
				{
				break;
				}
			}
		xmlFreeTextReader(in);
		}
	return ret;
	}
/** process XML input node, insert all the XML document do be merged 
 * before the last element */
static int
processInputNode(State* state)
    {
    xmlTextReaderPtr reader=state->reader;
    xmlTextWriterPtr writer=state->writer;
    
    switch(xmlTextReaderNodeType(reader))
    	{
    	case XML_READER_TYPE_ELEMENT:
    		{
    		int empty=xmlTextReaderIsEmptyElement(reader);
		xmlTextWriterStartElement(writer,xmlTextReaderConstName(reader));
		attributes(state,reader);
		if(empty)
			{
			if(xmlTextReaderDepth(reader)==0)
				{
				insertXMLs(state);
				}
			xmlTextWriterEndElement(writer);
			}
       		break;
    		}
    	case XML_READER_TYPE_END_ELEMENT:
		{
		if(xmlTextReaderDepth(reader)==0)
			{
			insertXMLs(state);
			}
		xmlTextWriterEndElement(writer);
		break;
		}
    	DEFAULT_EVENTS(reader,writer);
    	}
    return 0;
    }


int main(int argc,char** argv)
         {
         int ret=0;
         int optind=1;
         State state;
	 LIBXML_TEST_VERSION
	 char* inputFile=NULL;
	 char* outputFile=NULL;
	 
	 state.ignore_root=false;
	 
	/* loop over args */
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        cout << argv[0] << ": Pierre Lindenbaum PHD. 2010." << endl;
                        cout << "Compilation: "<< __DATE__ << " at " << __TIME__ << "." << endl;
                      	cout << "Options:\n";
                      	cout << "  -i <xml-in> (required)\n";
                      	cout << "  -o <xml-out> (default:stdout)\n";
                      	cout << "  -r ignore root of XML docs\n";
                      	cout << "xml1, xml2, ....xmln\n";
                        return(EXIT_SUCCESS);
                        }
                else if(strcmp(argv[optind],"-r")==0)
                        {
                        state.ignore_root=true;
                        } 
                else if(strcmp(argv[optind],"-i")==0 && optind+1<argc)
                        {
                        inputFile=argv[++optind];
                        }
                else if(strcmp(argv[optind],"-o")==0 && optind+1<argc)
                        {
                        outputFile=argv[++optind];
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
        if(inputFile==NULL)
        	{
        	cerr << "input file was not specified\n";
        	return(EXIT_FAILURE);
        	}
        if(outputFile!=NULL && strcmp(inputFile,outputFile)==0)
        	{
        	cerr << "input file == output file\n";
        	return EXIT_FAILURE;
        	}
	state.writer=NULL;
	state.reader=NULL;
	state.argv=argv;
	state.argc=argc;
	state.optind=optind;
	

	
	//start reading input file
	state.reader = xmlReaderForFile(inputFile, NULL, 0);
	if (state.reader== NULL)
		{
		cerr << "unknown create xmlReaderForFile for '"<<inputFile << "'" << endl;
                return(EXIT_FAILURE);
		}
	//create writer
	state.writer= xmlNewTextWriterFilename((outputFile==NULL?"-":outputFile), 0);
	if (state.writer== NULL)
		{
		cerr << "xmlNewTextWriterFilename failed." << endl;
                return(EXIT_FAILURE);
		}
	//start out
	xmlTextWriterStartDocument(state.writer, NULL,"UTF-8", NULL);
	
	while((xmlTextReaderRead(state.reader))==1)
		{
		if((ret=processInputNode(&state))!=0)
			{
			break;
			}
		}
	//release out
	xmlTextWriterEndDocument(state.writer);
	xmlFreeTextWriter(state.writer);
	xmlFreeTextReader(state.reader);
	
	xmlCleanupParser();
	xmlMemoryDump();
	
	return EXIT_SUCCESS;
        }


