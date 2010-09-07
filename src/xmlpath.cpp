
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
#include <libxml/parser.h>
#include <libxml/tree.h>

using namespace std;

/** process the DOM tree recursively, echo path if it matches */
static void recurs(std::vector<xmlNodePtr>& path,xmlNodePtr node,const char* elementName)
	{
	xmlNodePtr child=NULL;
	if(node==NULL) return;
	path.push_back(node);
	if(elementName==NULL || strcmp( (const char*) node->name, elementName)==0)
		{
		for(std::vector<xmlNodePtr>::size_type i=0;
			i< path.size();++i)
			{
			cout << "/";
			cout << path.at(i)->name;
			} 
		cout << endl;
		}

	for (child = node->children;child!=NULL; child = child->next)
		{
		if(child->type != XML_ELEMENT_NODE) continue;
		recurs(path,child,elementName);
		}

	path.pop_back();
	}




int main(int argc,char** argv)
         {
         char* elementName=NULL;
         int optind=1;
	 LIBXML_TEST_VERSION
	/* loop over args */
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        cerr << argv[0] << ": Pierre Lindenbaum PHD. 2010." << endl;
                        cerr << "Compilation: "<< __DATE__ << " at " << __TIME__ << "." << endl;
                        cerr << "Option:\n";
                        cerr << " -t <tag> matching tag (or match everything if not defined." << endl;
                        return(EXIT_FAILURE);
                        }
                else if(strcmp(argv[optind],"-t")==0)
                        {
			elementName=argv[++optind];
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
	
        if(optind==argc)
                {
                cerr << "Illegal number of arguments." << endl;
		return EXIT_FAILURE;
                }
         while(optind <argc)
         	{
		std::vector<xmlNodePtr> path;
		char* fname=argv[optind++];
		xmlDocPtr dom=xmlParseFile(fname);
		if(dom==NULL)
			{
			cerr << "Cannot read XML file " << fname << endl;
			exit( EXIT_FAILURE);
			}
		xmlNodePtr root_element = xmlDocGetRootElement(dom);
		if(root_element==NULL)
			{
			cerr << "Cannot get root for XML file " << fname << endl;
			exit( EXIT_FAILURE);
			}
		recurs(path,root_element,elementName);
		xmlFreeDoc(dom);
		}
	return EXIT_SUCCESS;
        }


