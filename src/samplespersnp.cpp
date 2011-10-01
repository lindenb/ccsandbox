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
 *	group VCF data by sample/SNP
 * Compilation:
 *	 g++ -o samplepersnp -Wall -O3 samplepersnp.cpp -lz
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

#define THROW(a) do{ostringstream _os;\
	_os << __FILE__ << ":"<< __LINE__ << ":"<< a << endl;\
	throw runtime_error(_os.str());\
	}while(0)

typedef int column_t;

#define CHECK_COL_INDEX(idx,tokens) do { if(((size_t)idx)>=tokens.size()) \
	{\
	THROW("Column index " <<  #idx << " out of range ("<< idx << ")");\
	} } while(0)

#define CHECK_COL_INDEXES(list) \
	    CHECK_COL_INDEX(chromcol,list);\
	    CHECK_COL_INDEX(poscol,list);\
	    if(use_ref_alt) CHECK_COL_INDEX(refcol,list);\
	    if(use_ref_alt) CHECK_COL_INDEX(altcol,list);\
	    CHECK_COL_INDEX(samplecol,list);

class SamplePerSnp
    {
    public:
	char delim;
	set<string> samples;
	column_t chromcol;
	column_t poscol;
	column_t genecol;
	column_t refcol;
	column_t altcol;
	column_t samplecol;

	bool use_ref_alt;
	vector<string> buffer;






	SamplePerSnp()
	    {
	    delim='\t';
	    chromcol=0;
	    poscol=1;
	    refcol=2;
	    altcol=3;
	    samplecol=-1;
	    use_ref_alt=true;
	    }
	~SamplePerSnp()
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

	bool same(const string& line1,const string& line2)
	    {
	    return strcasecmp(line1.c_str(),line2.c_str())==0;
	    }
	bool equals(const string& line1,const string& line2)
	    {
	    vector<string> tokens1;
	    split(line1,tokens1);
	    CHECK_COL_INDEXES(tokens1);
	    vector<string> tokens2;
	    split(line2,tokens2);
	    CHECK_COL_INDEXES(tokens2);
	    if(!same(tokens1[chromcol],tokens2[chromcol])) return false;
	    if(!same(tokens1[poscol],tokens2[poscol])) return false;
	    if(use_ref_alt)
		{
		if(!same(tokens1[refcol],tokens2[refcol]))
		    {
		    cerr << "?not same ref beween\n\t"
			<< line1 << "\n\t"<< line2 << endl;
		    return false;
		    }
		if(!same(tokens1[altcol],tokens2[altcol]))
		    {
		    return false;
		    }
		}
	    return true;
	    }

	size_t countSamples()
	    {
	    set<string> samples;
	    for(size_t i=0;i< buffer.size();++i)
		{
		vector<string> tokens;
		split(buffer[i],tokens);
		CHECK_COL_INDEX(samplecol,tokens);
		if(tokens[samplecol].empty())
		    {
		    cerr << "Warning empty sample name in "<< buffer[i]<< endl;
		    }
		samples.insert(tokens[samplecol]);
		}
	    return samples.size();
	    }


	void run(gzFile in)
	    {
	     string line;



	    while(readline(in,line))
		{
		if(line.empty()) continue;


		if(buffer.empty() || equals(buffer.front(),line))
		    {
		    buffer.push_back(line);
		    }
		else
		    {
		    for(size_t i=0;i< buffer.size();++i)
			{
			cout << buffer[i] << delim <<countSamples()<< endl;
			}
		    buffer.clear();
		    buffer.push_back(line);
		    }
		}
	    for(size_t i=0;i< buffer.size();++i)
		{
		cout << buffer[i] << delim << countSamples() << endl;
		}
	    }
    };

#define SETINDEX(option,col) else if(std::strcmp(argv[optind],option)==0 && optind+1<argc) \
	{\
	char* p2;\
	int idx=strtol(argv[++optind],&p2,10);\
      	if(idx<1) THROW("Bad " option " index in "<< argv[optind]);\
	app.col=idx-1;\
	}
#define SHOW_OPT(col) \
	cerr << "  --"<< cols<< " (column index)\n";

int main(int argc,char** argv)
    {
    SamplePerSnp app;
    int optind=1;
    while(optind < argc)
   		{
   		if(std::strcmp(argv[optind],"-h")==0)
   			{
   			cerr << argv[0] << "Pierre Lindenbaum PHD. 2011.\n";
   			cerr << "Compilation: "<<__DATE__<<"  at "<< __TIME__<<".\n";
   			cerr << "Options:\n";
   			cerr << "  --delim (char) delimiter default:tab\n";
   			cerr << "  --norefalt : don't look at REF and ALT\n";
   			cerr << "  --sample SAMPLE column index\n";
   			cerr << "  --chrom CHROM column index: default "<< (app.chromcol+1) << "\n";
   			cerr << "  --pos POS position column index: default "<< (app.poscol+1) << "\n";
   			cerr << "  --ref REF reference allele column index: default "<< (app.refcol+1) << "\n";
   			cerr << "  --alt ALT alternate allele column index: default "<< (app.altcol+1) << "\n";
   			cerr << "(stdin|vcf|vcf.gz)\n";
   			exit(EXIT_FAILURE);
   			}
   		SETINDEX("--sample",samplecol)
   		SETINDEX("--chrom",chromcol)
   		SETINDEX("--pos",poscol)
   		SETINDEX("--ref",refcol)
   		SETINDEX("--alt",altcol)
   		else if(std::strcmp(argv[optind],"--norefalt")==0)
   			{
   			app.use_ref_alt=false;
   			}
   		else if(std::strcmp(argv[optind],"--delim")==0 && optind+1< argc)
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
    if(app.samplecol==-1)
    	{
    	cerr << "Undefined sample column."<< endl;
    	return (EXIT_FAILURE);
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
