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
 *	cut columns by finding the indexes in the header
 * Compilation:
 *	 g++ -o colgrep -Wall -O3 colgrep
 */
#include <set>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cassert>
using namespace std;

struct ToUpper {
   void operator()(char& c) { c = toupper(c); }
};


class ColGrep
    {
    public:
	char delim;
	set<string> columnnames;
	vector<bool> columnindex;
	bool inverse;
	bool printIndexesAndExit;
	bool ignorecase;

	ColGrep():delim('\t')
	    {
	    inverse=false;
	    printIndexesAndExit=false;
	    ignorecase=false;
	    }

	void split(const string& line,vector<string>& tokens)
	    {
	    size_t prev=0;
	    size_t i=0;
	    tokens.clear();
	    assert(tokens.empty());
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

	void run(FILE* in)
	    {
	    size_t nLine=0;
	    string line;
	    vector<string> tokens;
	    while(!feof(in))
		{
		int c;
		line.clear();
		while((c=fgetc(in))!=EOF && c!='\n')
		    {
		    line+=c;
		    }
		++nLine;


		split(line,tokens);

		if(nLine==1)
		    {
		    for(size_t i=0;i< tokens.size();++i)
			{
			string head=tokens[i];
			if(ignorecase)
			    {
			    std::for_each(head.begin(),head.end(),ToUpper());
			    }
			set<string>::iterator r=columnnames.find(head);
			if(r==columnnames.end()) continue;
			columnnames.erase(r);
			columnindex.resize(i+1,false);
			columnindex[i]=true;
			}
		    for(set<string>::iterator i=columnnames.begin();
			i!=columnnames.end();
			++i)
		    	{
			cerr << "Warning: could not find column \""<< (*i)<< "\" in "
				<< line << endl;
		    	}
		    if(printIndexesAndExit)
			{
			bool first=true;
			for(size_t i=0;i< columnindex.size();++i)
			    {
			    if(!columnindex[i]) continue;
			    if(!first) cout <<",";
			    first=false;
			    cout << (i+1);
			    }
			cout << endl;
			return;
			}
		    }

	    bool first=true;
	    for(size_t i=0;i< tokens.size();++i)
		{
		if((i<columnindex.size() && columnindex[i])==!inverse)
		    {
		    if(!first) cout << delim;
		    first=false;
		    cout <<tokens[i];
		    }
		}
	    cout << endl;
	    }
	}
    };

int main(int argc,char** argv)
         {
	 ColGrep app;
         int optind=1;
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
			cerr << argv[0] << "Pierre Lindenbaum PHD. 2011.\n";
			cerr << "Compilation: "<<__DATE__<<"  at "<< __TIME__<<".\n";
			cerr << "Options:\n";
			cerr << "  -v inverse selection\n";
			cerr << "  -d (char) delimiter (default is TAB)\n";
			cerr << "  -p print indexes and exit\n";
			cerr << "  -c <string> add column in selection (can be called multiple time)\n";
			cerr << "  -i ignore case\n";
			cerr << "(stdin|file)\n";
			return (EXIT_FAILURE);
                        }
                else if(strcmp(argv[optind],"-v")==0 )
                        {
                        app.inverse=true;
                        }
                else if(strcmp(argv[optind],"-i")==0 )
		       {
		       app.ignorecase=true;
		       }
                else if(strcmp(argv[optind],"-p")==0)
		       {
		       app.printIndexesAndExit=true;
		       }
                else if(strcmp(argv[optind],"-c")==0 && optind+1<argc)
		       {
		       app.columnnames.insert(argv[++optind]);
		       }
                else if(std::strcmp(argv[optind],"-d")==0 && optind+1< argc)
			{
			char* p=argv[++optind];
			if(strlen(p)!=1)
			    {
			    cerr << "Bad delimiter \""<< p << "\"\n";
			    exit(EXIT_FAILURE);
			    }
			app.delim=p[0];
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
       if(app.ignorecase)
	   {
	   set<string> colsi;
	   for(set<string>::iterator r=app.columnnames.begin();r!=app.columnnames.end();++r)
	       {
	       string s(*r);
	       std::for_each(s.begin(),s.end(),ToUpper());
	       colsi.insert(s);
	       }
	   app.columnnames.clear();
	   app.columnnames.insert(colsi.begin(),colsi.end());
	   }
        if(optind==argc)
                {
                app.run(stdin);
                }
        else if(optind+1==argc)
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

		app.run(in);
		fclose(in);
                }
        else
            {
            cerr << "Illegal number of arguments\n";
            return EXIT_FAILURE;
            }
        return EXIT_SUCCESS;
        }


