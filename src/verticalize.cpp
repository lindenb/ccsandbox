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
 *	verticalize a table
 * Compilation:
 *	 g++ -o verticalize -Wall -O3 verticalize.cpp -lz
 */
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <cerrno>
#include <string>
#include <cstring>
#include <stdexcept>
#include <climits>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <iostream>
#include <zlib.h>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <stdint.h>

using namespace std;


class Verticalize
    {
    public:

	char delim;
	bool first_line_is_header;


	Verticalize()
	    {
	    delim='\t';
	    first_line_is_header=true;
	    }
	~Verticalize()
	    {

	    }

	bool readline(gzFile in,std::string& line)
	    {
	    line.clear();
	    int c;
	    if(gzeof(in)) return false;
	    while((c=gzgetc(in))!=EOF && c!='\n')
		    {
		    line+=(char)c;
		    }
	    return true;
	    }
	void split(const string& line,vector<string>& tokens)
	    {
	    size_t prev=0;
	    size_t i=0;
	    tokens.clear();
	    while(i<=line.size())
		{
		if(i==line.size() || line[i]==delim)
		    {
		    tokens.push_back(line.substr(prev,i-prev));
		    if(i==line.size()) break;
		    prev=i+1;
		    }
		++i;
		}
	    }


	void run(gzFile in)
	    {
	    size_t nLine=0UL;
	    vector<string> header;
	    vector<string> tokens;
	    string line;
	    size_t len_word=0UL;
	    if(first_line_is_header)
		{
		if(!readline(in,line))
		    {
		    cerr << "Error cannot read first line.\n";
		    return;
		    }
		++nLine;
		split(line,header);
		for(size_t i=0;i< header.size();++i) len_word=max(len_word,header[i].size());
		}

	    while(readline(in,line))
		{
		++nLine;
		cout << ">>>"<< delim << (nLine)<< endl;
		split(line,tokens);
		if(first_line_is_header)
		    {
		    for(size_t i=0;i< header.size();++i)
			{
			cout << "$"<<(i+1)<< delim << header[i];
			for(size_t j=header[i].size();j< len_word;++j)
			    {
			    cout << " ";
			    }
			cout <<delim;
			if(i<tokens.size())
			    {
			    cout << tokens[i];
			    }
			else
			    {
			    cout << "???";
			    }
			cout <<endl;
			}
		    for(size_t i=header.size();i< tokens.size();++i)
			{
			cout << "$"<<(i+1)<<delim << "???";
			for(size_t j=3;j< len_word;++j)
			    {
			    cout << " ";
			    }
			cout << delim << tokens[i] << endl;
			}
		    }
		else
		    {
		    for(size_t i=header.size();i< tokens.size();++i)
			{
			cout << "$"<<(i+1)<<delim << tokens[i] << endl;
			}
		    }
		cout << "<<<"<< delim << (nLine)<< "\n\n";
		}
	    }
    };


int main(int argc,char** argv)
    {
    Verticalize app;
    int optind=1;
    while(optind < argc)
   		{
   		if(std::strcmp(argv[optind],"-h")==0)
   			{
   			cerr << argv[0] << "Pierre Lindenbaum PHD. 2011.\n";
   			cerr << "Compilation: "<<__DATE__<<"  at "<< __TIME__<<".\n";
   			cerr << "Options:\n";
   			cerr << "  -d or --delim (char) delimiter default:tab\n";
   			cerr << "  -n first line is NOT the header.\n";
   			cerr << "(stdin|file|file.gz)\n";
   			exit(EXIT_FAILURE);
   			}

   		else if(std::strcmp(argv[optind],"-n")==0)
   			{
   			app.first_line_is_header =false;
   			}
   		else if((std::strcmp(argv[optind],"-d")==0 ||
   			 std::strcmp(argv[optind],"--delim")==0)
   			&& optind+1< argc)
			{
			char* p=argv[++optind];
			if(strlen(p)!=1)
			    {
			    cerr << "Bad delimiter \""<< p << "\"\n";
			    exit(EXIT_FAILURE);
			    }
			app.delim=p[0];
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
	    gzFile in=gzdopen(fileno(stdin),"r");
	    if(in==NULL)
		{
		cerr << "Cannot open stdin" << endl;
		return EXIT_FAILURE;
		}
	    app.run(in);
	    }
    else
	    {
	    while(optind< argc)
		{
		char* filename=argv[optind++];
		gzFile in=gzopen(filename,"r");
		if(in==NULL)
		    {
		    cerr << "Cannot open "<< filename << " " << strerror(errno) << endl;
		    return EXIT_FAILURE;
		    }
		app.run(in);
		gzclose(in);
		}
	    }
    return EXIT_SUCCESS;
    }
