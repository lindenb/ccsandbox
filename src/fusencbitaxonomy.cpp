/*
 * Author:
 *      Pierre Lindenbaum PhD
 * WWW
 *      http://plindenbaum.blogspot.com
 * Date
 * 	2011-08-15
 * Motivation:
 *       use the FUSE API to browse the NCBI taxonomy as a filesystem
 * Compilation:
 * 	g++ -D_FILE_OFFSET_BITS=64 -I $(FUSE)/include -Wall -o fusetaxonomy  fusencbitaxonomy.cpp -L  $(FUSE)/lib -lfuse
 * Reference:
 *	http://plindenbaum.blogspot.com/2011/08/fuse-based-filesyetem-reproducing-ncbi.html
 * 	http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Hello_World
 */
/* required, see top of fuse/fuse.h */
#define FUSE_USE_VERSION 26 
#include <fuse/fuse.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cassert>
#include <sstream>
#include <fcntl.h>

using namespace std;

FILE *safeFOpen (const char *filename,const char *modes)
    {
    FILE* f=std::fopen(filename,modes);
    if(f==NULL)
        {
        cerr << "[I/O] Cannot open " << filename << " " << strerror(errno)<< ".\n";
        std::exit(EXIT_FAILURE);
        }
    return f;
    }


void *safeRealloc (void *ptr, size_t size)
    {
    ptr=std::realloc(ptr,size);
    if(ptr==NULL)
        {
        cerr << "[Out of memory] Cannot re-allocate " << size << " bytes.\n";
        std::exit(EXIT_FAILURE);
        }
    return ptr;
    }

void *safeMalloc (std::size_t size)
    {
    void* p=std::malloc(size);
    if(p==NULL)
        {
        cerr << "[Out of memory] Cannot allocate " << size << " bytes.\n";
        std::exit(EXIT_FAILURE);
        }
    return p;
    }
char* safeStrndup(const char* s,size_t len)
    {
    char *p=(char*)safeMalloc((len+1)*sizeof(char));
    memcpy((void*)p,s,len*sizeof(char));
    p[len]=0;
    return p;
    }

char* safeStrdup(const char* s)
    {
    return safeStrndup(s,strlen(s));
    }


/** Line reader Utility */
class LineReader
    {
    private:
	FILE* in;
	char* buffer;
	std::size_t buffer_capacity;
    public:
	LineReader(FILE* in):in(in),buffer(NULL),buffer_capacity(BUFSIZ)
	    {
	    buffer=(char*)safeMalloc(buffer_capacity);
	    }
	virtual ~LineReader()
	    {
	    free(buffer);
	    }
	char* readLine(std::size_t* userlen)
	    {
	    if(feof(in))
		{
		if(userlen!=NULL) *userlen=0UL;
		return NULL;
		}
	    int c;
	    size_t len=0;
	    while((c=fgetc(in))!=EOF && c!='\n')
		{
		if(len+2>=buffer_capacity)
		    {
		    buffer_capacity+=BUFSIZ;
		    buffer=(char*)safeRealloc(buffer,buffer_capacity*sizeof(char));
		    }
		buffer[len++]=c;
		}
	    buffer[len]='\0';
	    if(userlen!=NULL) *userlen=len;
	    return buffer;
	    }
	
    };

/**
 * A node in the NCBI taxonomy 
 */
class Taxon
    {
    public:
	 /* node id */
	int id;
	/* parent id */
	int parent_id;
	/* number of children */
	int count_children;
	/* name for this node */
	char* name;
	/* children */
	Taxon** children;
	Taxon():id(-1),parent_id(-1),count_children(0),name(NULL),children(NULL)
	    {
	    
	    }
	~Taxon()
	    {
	    free(name);
	    free(children);
	    }
	/* compare by id */
	static int comparator(const void* p1,const void* p2)
	    {
	    Taxon** tx1=(Taxon**)p1;
	    Taxon** tx2=(Taxon**)p2;
	   
	    return (*tx1)->id - (*tx2)->id;
	    }
	/* compare by name*/
	static int compareByName(const void* p1,const void* p2)
	    {
	    Taxon** tx1=(Taxon**)p1;
	    Taxon** tx2=(Taxon**)p2;
	   
	    return strcmp((*tx1)->name, (*tx2)->name);
	    }
	/* get i-th children */
	const Taxon* getChildrenAt(int i) const
	    {
	    assert(i>=0 && i< count_children);
	    return children[i];
	    }
	/* find children by its name*/
	const Taxon* findChildByName(const char* name) const
	    {
	    for(int i=0;i< count_children;++i)
		{
		const Taxon* c = getChildrenAt(i);
		if(strcmp(c->name,name)==0) return c;
		}
	    return NULL;
	    }
	
	/* find child from a unix path */
	const Taxon* findChildByPath(const char* path) const
	    {
	    char* p=(char*)strchr(path,'/');
	    if(p==NULL)
		{
		const Taxon* child=findChildByName(path);
		return child;
		}
	    char* s=safeStrndup(path,p-path);
	    const Taxon* child=findChildByName(s);
	    free(s);
	    if(child==NULL) return NULL;
	    return child->findChildByPath(&p[1]);
	    }
	
	/* stupid representation of a node as  a XML file */
	string xml() const
	    {
	    ostringstream os;
	    os << "<?xml version=\"1.0\"?>\n<Taxon-Id>"<< id << "</Taxon-Id>\n";
	    return os.str();
	    }
    };

/** the NCBI taxonomy */
class FuseTaxonomy
	{
	private:
	     /* number of nodes */
	    size_t nTaxons;
	    /** taxons ordered by ids */
	    Taxon** taxons;
	    /** taxons ordered by names */
	    Taxon** names;
	    
	public:
		/* global instance */
		static FuseTaxonomy* INSTANCE;
		FuseTaxonomy():nTaxons(0),taxons(NULL),names(NULL)
		    {
		    
		    }
		~FuseTaxonomy()
		    {
		    for(size_t i=0;i< nTaxons;++i)
			{
			delete taxons[i];
			}
		    free(taxons);
		    free(names);
		    }
		/* find taxon by ID */
		const Taxon* findTaxonById(int id)const
		    {
		    Taxon key;
		    key.id=id;
		    Taxon* keyPtr=&key;
		    Taxon** v=(Taxon**)bsearch(
			    &keyPtr,taxons,
			    nTaxons,
			    sizeof(Taxon*),
			    Taxon::comparator
			    );
		    return v==NULL?NULL:*v;
		    }
		/* find taxon by name */
		const Taxon* findTaxonByName(const char* name) const
			{
			Taxon key;
			key.name=(char*)name;//simple reference, no copy
			Taxon* keyPtr=&key;
			Taxon** v=(Taxon**)bsearch(
				&keyPtr,names,
				nTaxons,
				sizeof(Taxon*),
				Taxon::compareByName
				);
			key.name=NULL;//prevent destructor to crash
			return v==NULL?NULL:*v;
			}
		/* root of taxonomy */
		const Taxon* getRoot() const
		    {
		    const Taxon* root=findTaxonById(1);
		    assert(root!=NULL);
		    return root;
		    }
		
		/* read file nodes.dmp */
		void readNodes(const char* nodes)
			{
			FILE* in=safeFOpen(nodes,"r");
			LineReader reader(in);
			char* line=NULL;
			while((line=reader.readLine(NULL))!=NULL)
			    {
			    char* pipe1=strchr(line,'|');
			    if(pipe1==NULL) continue;
			    *pipe1=0;
			    ++pipe1;
			    char* pipe2= strchr(pipe1,'|');
			    if(pipe2==NULL) continue;
			    *pipe2=0;
			    Taxon* taxon=new Taxon;
			    taxon->id= atoi(line);
			    
			    taxon->parent_id=atoi(pipe1);
			    taxons=(Taxon**)safeRealloc(taxons,sizeof(Taxon*)*(nTaxons+1));
			    taxons[nTaxons]=taxon;
			    nTaxons++;
			    }
			fclose(in);
			qsort ((void*)taxons,nTaxons,sizeof(Taxon*), Taxon::comparator);
			for(size_t i=0;i< nTaxons;++i)
			    {
			    Taxon* taxon=taxons[i];
			   
			   
			    Taxon* parent=(Taxon*)findTaxonById(taxon->parent_id);
			    if(parent==NULL || parent->id==taxon->id)
				{
				continue;
				}
			    parent->children=(Taxon**)safeRealloc(parent->children,sizeof(Taxon*)*(parent->count_children+1));
			    parent->children[parent->count_children]=taxon;
			    parent->count_children++;
			    }
			}
		
		/* read file names.dmp */
		void readNames(const char* namesfile)
			{
			FILE* in=safeFOpen(namesfile,"r");
			LineReader reader(in);
			size_t len;
			char* line=NULL;
			while((line=reader.readLine(&len))!=NULL)
			    {
			    char* pipe1=strchr(line,'|');
			    if(pipe1==NULL) continue;
			    *pipe1=0;
			    ++pipe1;
			    while(isspace(*pipe1)) ++pipe1;
	
			    char* pipe2= strchr(pipe1,'|');
			    if(pipe2==NULL) continue;
			    *pipe2=0;
			    char* p3=pipe2-1;
			    while(pipe1< p3 && isspace(*p3)) --p3;
			    *(p3+1)=0;
			  
			    
			    int id=atoi(line);
			    Taxon* taxon=(Taxon*)findTaxonById(id);
			    if(taxon==NULL) continue;
			    if(taxon->name!=NULL)
				{
				if(strstr(pipe2+1,"scientific name")==NULL) continue;
				free(taxon->name);
				}
			   
			    taxon->name= safeStrdup(pipe1);
			    char* p4=taxon->name;
			    while(*p4!=0)
				{
				if(!isalnum(*p4))
				    {
				    *p4='_';
				    }
				++p4;
				}
			    }
			fclose(in);
			names=(Taxon**)safeMalloc(sizeof(Taxon*)*(nTaxons));
			memcpy((void*)names,taxons,sizeof(Taxon*)*(nTaxons));
			qsort ((void*)names,nTaxons,sizeof(Taxon*), Taxon::compareByName);
			}
		
		
	
		/* find taxon node from unix path */
		const Taxon* findByPath(const char* path) const
		    {

		    if(path==NULL )
			{
			return NULL;
			}
		    if(path[0]!='/')
			{
			return NULL;
			}
		    char* path1=(char*)&path[1];
		    if(strcmp(path1,getRoot()->name)==0)
			{
			return getRoot();
			}
		    char* slash=(char*)strchr(path1,'/');
		    if(slash==NULL)
			{
			return NULL;
			}
		    if(strncmp(getRoot()->name,path1,strlen(getRoot()->name))!=0)
			{
			return NULL;
			}
		    
		    return getRoot()->findChildByPath(&slash[1]);
		    }
/**  FUSE CALLBACK:  This function returns metadata concerning a file specified by path in a special stat structure. */
static int getattr(const char *path, struct stat *stbuf)
    {
    int res = 0;
    FuseTaxonomy* taxonomy=FuseTaxonomy::INSTANCE;
    //Reset memory for the stat structure
    memset(stbuf, 0, sizeof(struct stat));

    if(path[0]!='/')
	{
	return ENOENT;
	}
    if(path[1]=='.')
   	{
   	return ENOENT;
   	}
    const Taxon* taxon= taxonomy->findByPath(path);
    
    if(strcmp("/",path)==0)
	{
	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 1 ;
	}
    else if(taxon!=NULL && taxon->count_children!=0)
   	{
   	stbuf->st_mode = S_IFDIR | 0755;
   	stbuf->st_nlink = taxon->count_children;
   	}
    else if(taxon!=NULL )
	{
	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;
	stbuf->st_size = taxon->xml().size();
	}
    else
	{
	res = -ENOENT;
	}

    return res;
    }
/* FUSE CALLBACK:  used to read directory contents */
static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
    {
    (void) offset;
    (void) fi;
    FuseTaxonomy* taxonomy=FuseTaxonomy::INSTANCE;
    
    if(strcmp(path,"/")==0)
	{

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, taxonomy->getRoot()->name, NULL, 0);
	}
    else
	{
	const Taxon* taxon= taxonomy->findByPath(path);
	if(taxon==NULL)
	    {
	   return -ENOENT;
	    }
	
	if(taxon->count_children>0)
	    {
	    filler(buf, ".", NULL, 0);
	    filler(buf, "..", NULL, 0);
	    }
	
	for(int i=0;i< taxon->count_children;++i)
	    {
	    const Taxon* c=taxon->getChildrenAt(i);
	    filler(buf, c->name, NULL, 0);
	    }
	}
    return 0;
    }

/* FUSE CALLBACK: checks whatever user is permitted to open the /hello file with flags given in the fuse_file_info structure.  */
static int open(const char *path, struct fuse_file_info *fi)
    {

    if(strcmp(path,"/")==0) return -ENOENT;
    FuseTaxonomy* taxonomy=FuseTaxonomy::INSTANCE;
    const Taxon* taxon= taxonomy->findByPath(path);
    if(taxon==NULL) return -ENOENT;
    //if the user wants to open the file for anything else than reading only, we tell him that he does not have sufficient permissions.
    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
    }
/* FUSE CALLBACK:  used to feed the user with data from the file. */
static int read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
	{
	FuseTaxonomy* taxonomy=FuseTaxonomy::INSTANCE;
	 const Taxon* taxon= taxonomy->findByPath(path);
	if(taxon==NULL) return -ENOENT;
	string xml = taxon -> xml();
	
	size_t len = 0;
	for(size_t i=offset;i< xml.size() && i< size;++i)
	    {
	    buf[i]=xml.at(i);
	    ++len;
	    }
	return len;
	}
};

FuseTaxonomy* FuseTaxonomy::INSTANCE=NULL;

int main(int argc, char *argv[])
	{
	if(argc!=4)
		{
		fprintf(stderr,"%s: Pierre Lindenbaum PhD. 2011. Compilation:%s \n",argv[0],__DATE__);
		fprintf(stderr,"Usage:%s nodes.dmp names.dmp fuse.directory\n",argv[0]);
		return EXIT_FAILURE;
		}
	void* userData=NULL;
	FuseTaxonomy::INSTANCE=new FuseTaxonomy;
	FuseTaxonomy::INSTANCE->readNodes(argv[1]);
	FuseTaxonomy::INSTANCE->readNames(argv[2]);
	assert(FuseTaxonomy::INSTANCE->getRoot()!=NULL);
	
	struct fuse_operations operations;
	memset((void*)&operations,0,sizeof( struct fuse_operations));
	operations.getattr	= FuseTaxonomy::getattr;
	operations.readdir	= FuseTaxonomy::readdir;
	operations.open	= FuseTaxonomy::open;
	operations.read	= FuseTaxonomy::read;
	
	return fuse_main(argc-2, &argv[2], &operations,userData);
	}

