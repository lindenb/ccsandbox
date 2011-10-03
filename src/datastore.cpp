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
 *	key value datastore using 3 engines:
 *	* leveldb
 *	* sqlite3
 *	* berkeleydb
 * Compilation:
 *	g++ -O3 -Wall -o sqlitedatastore datastore.cpp  -lsqlite3
 *
 *	g++ -O3 -Wall -o leveldatastore -L ${LEVELDDBDIR} -I ${LEVELDDBDIR}/include -DLEVELDB_VERSION  datastore.cpp   -lz -lleveldb -lpthread
 *
 *	export LD_LIBRARY_PATH=${BDBPATH}/lib
 *	g++ -Wall -O3 -o bdbdatastore -L ${BDBPATH}/lib -I ${BDBPATH}/include -DBERKELEYDB_VERSION  datastore.cpp -ldb
 *
 */
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <vector>
#include <fstream>
#include <list>
#include <cstring>
#include <sstream>
#include <memory>
#define DB_NAME "KeyValueDatabase"

#ifdef LEVELDB_VERSION
#include "leveldb/db.h"
#elif BERKELEYDB_VERSION
#include <db.h>
#else
#define SQLLITE_VERSION
#include <sqlite3.h>
#endif

using namespace std;

#define WHERE(a) std::cerr << __FILE__ << ":" << __LINE__ << ":" << a << std::endl
#define strequals(a,b) (std::strcmp(a,b)==0)
enum ProgramType
    {
    DATASTORE_GET,
    DATASTORE_DUMP,
    DATASTORE_PUT,
    DATASTORE_RM
    };

class DataStore
    {
    public:
        DataStore();
        ~DataStore();
        int open();
        void close();
        int put(const char* key,size_t lenkey,const char* data,size_t lendata);
        int get(const char* key,size_t len);
        int scanfile(std::istream&);
        int rm(const char* key,size_t len);
        int dump();
        bool isReadOnly();
        static int main(int argc,char** argv);
    private:
        char* db_home;
        char delim;
        int program;
        char* lower_key;
        char* upper_key;

	#ifdef LEVELDB_VERSION
	    leveldb::DB* db;
	#elif BERKELEYDB_VERSION
	    DB *dbp;
	    DB_TXN *txn;
        #else
	    sqlite3_stmt* prepare_dump();
            sqlite3* connection;
            sqlite3_stmt* stmt_get;
            sqlite3_stmt* stmt_delete;
            sqlite3_stmt* stmt_put;
        #endif
    };


DataStore::DataStore():db_home(NULL),
    delim('\t'),
    program(DATASTORE_GET),
    lower_key(NULL),
    upper_key(NULL)
#ifdef LEVELDB_VERSION
    ,db(NULL)
#elif BERKELEYDB_VERSION
    ,dbp(NULL),txn(NULL)
#else
    ,connection(NULL),
    stmt_get(NULL),
    stmt_delete(NULL),
    stmt_put(NULL)
#endif
    {

    }

DataStore::~DataStore()
    {
    close();
    }

bool DataStore::isReadOnly()
    {
    return program==DATASTORE_GET || program==DATASTORE_DUMP;
    }

int DataStore::open()
    {

    if(db_home==NULL)
        {
        cerr << "DB_HOME undefined.\n";
        return EXIT_FAILURE;
        }
#ifdef LEVELDB_VERSION
    leveldb::Options options;
    options.create_if_missing = !isReadOnly();
    leveldb::Status status = leveldb::DB::Open(options,db_home, &db);
    if(!status.ok())
	{
	db=NULL;
	cerr << "Cannot open leveldb file "<< db_home <<".\n";
	return EXIT_FAILURE;
	}
#elif BERKELEYDB_VERSION
    int ret = db_create(&dbp, NULL, 0);
    if (ret != 0)
	{
	cerr << "Cannot create db"<< endl;
	return EXIT_FAILURE;
	}
    int flags=  (isReadOnly()?DB_RDONLY:DB_CREATE);
    ret = dbp->open(dbp,        /* DB structure pointer */
                    NULL,       /* Transaction pointer */
                    db_home, /* On-disk file that holds the database. */
                    DB_NAME,       /* Optional logical database name */
                    (isReadOnly()?DB_UNKNOWN:DB_BTREE),   /* Database access method */
                    flags,      /* Open flags */
                    0);         /* File mode (using defaults) */
    if (ret != 0)
	{
	cerr << "Cannot open berkeleydb file "<< db_home <<".\n";
	return EXIT_FAILURE;
	}
#else
    char *error=NULL;
    if(::sqlite3_open(db_home,&connection)!=SQLITE_OK)
        {
        cerr << "Cannot open sqlite file "<< db_home <<".\n";
        connection=NULL;
        return EXIT_FAILURE;
        }


    if(!isReadOnly())
        {
	if(::sqlite3_exec(connection,
            "create table if not exists " DB_NAME"(xKey TEXT NOT NULL PRIMARY KEY ASC,xData TEXT NOT NULL)",
            NULL,NULL,&error )!=SQLITE_OK)
            {
            cerr << "Cannot create table "<< error << endl;
            return EXIT_FAILURE;
            }

        if(sqlite3_prepare(connection,
                "insert into  "DB_NAME"(xKey,xData) values(?,?)",
                -1,&stmt_put,NULL )!=SQLITE_OK)
            {
            cerr <<"Cannot compile insert statement.\n"<< endl;
            return EXIT_FAILURE;
            }
        if(sqlite3_prepare(connection,
            "delete from "DB_NAME" where xKey=?",
            -1,&stmt_delete,NULL )!=SQLITE_OK)
            {
            cerr <<"Cannot compile delete statement.\n"<< endl;
            return EXIT_FAILURE;
            }
        }
    if(sqlite3_prepare(connection,
        "select xData from "DB_NAME" where xKey=?",
        -1,&stmt_get,NULL )!=SQLITE_OK)
        {
        cerr <<"Cannot compile select statement.\n"<< endl;
        return EXIT_FAILURE;
        }
#endif
    return EXIT_SUCCESS;
    }

#define DEL_STMT(stmt) if(stmt!=NULL) { ::sqlite3_finalize(stmt); stmt=NULL; }
void DataStore::close()
    {
#ifdef LEVELDB_VERSION
    if(db!=NULL)
	{
	delete db;
	db=NULL;
	}
#elif BERKELEYDB_VERSION
    if (dbp != NULL)
	{
        dbp->close(dbp, 0);
	dbp=NULL;
	}
#else
	DEL_STMT(stmt_put)
        DEL_STMT(stmt_delete)
        DEL_STMT(stmt_get)
        if(connection!=NULL)
            {
            sqlite3_close(connection);
            connection=NULL;
            }
#endif
    }
#undef DEL_STMT

int DataStore::rm(const char* s,size_t len)
    {
#ifdef LEVELDB_VERSION
    leveldb::Slice key1(s,len);
    leveldb::WriteOptions options;
    leveldb::Status status = db->Delete(options, key1);
    if(!status.ok())
	{
	cerr << "Cannot remove "<< status.ToString() << endl;
	return EXIT_FAILURE;
	}
#elif BERKELEYDB_VERSION
    DBT key1;
    memset(&key1, 0, sizeof(DBT));
    key1.data = (char*)s;
    key1.size = (int)len;
    int ret=dbp->del(dbp, txn, &key1, 0);
    if(ret!=0)
	{
	cerr << "del failed "<< db_strerror(ret)<< endl;
	}
#else
    if(::sqlite3_bind_text(
            stmt_delete,1,
            s,
            len,
            NULL)!=SQLITE_OK)
            {
            cerr << "Cannot bind key[1]\n";
            return EXIT_FAILURE;
            }
    while(sqlite3_step(stmt_delete) != SQLITE_DONE)
        {

        }
#endif
    return EXIT_SUCCESS;
    }

#ifdef LEVELDB_VERSION
    //specific to level db
#elif BERKELEYDB_VERSION
    //nothing
#else
sqlite3_stmt* DataStore::prepare_dump()
    {
    sqlite3_stmt* stmt_dump=NULL;
    std::ostringstream os;
    os <<  "select xKey,xData from "DB_NAME;
    if(lower_key!=NULL)
        {
        os << " WHERE xKey>=? ";
        }
    if(upper_key!=NULL)
        {
        if(lower_key==NULL)
            {
            os << " WHERE ";
            }
        else
            {
            os << " AND ";
            }
        os << " xKey<=?";
        }
    string sql(os.str());

    if(::sqlite3_prepare(connection,
            sql.c_str(),
            -1,&stmt_dump,NULL )!=SQLITE_OK)
            {
            cerr <<"Cannot compile dump statement.\n"<< endl;
            return NULL;
            }
    if(lower_key!=NULL)
        {
        if(::sqlite3_bind_text(
            stmt_dump,1,
            lower_key,
            strlen(lower_key),
            NULL)!=SQLITE_OK)
            {
            ::sqlite3_finalize(stmt_dump);
            cerr << "Cannot bind lower key\n";
            return NULL;
            }
        }
    if(upper_key!=NULL)
        {
        if(::sqlite3_bind_text(
                stmt_dump,(lower_key==NULL?1:2),
            upper_key,
            strlen(upper_key),
            NULL)!=SQLITE_OK)
            {
            ::sqlite3_finalize(stmt_dump);
            cerr << "Cannot bind upper key\n";
            return NULL;
            }
        }
    return stmt_dump;
    }
#endif

int DataStore::dump()
    {
#ifdef LEVELDB_VERSION
    auto_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));
    if(lower_key!=NULL)
	{
	it->Seek(lower_key);
	}
    else
	{
	it->SeekToFirst();
	}

    while(it->Valid())
	{
	if(upper_key!=NULL)
	    {
	    if(it->key().ToString().compare(upper_key)>0) break;
	    }
	cout << it->key().ToString() ;
	cout << delim;
	cout << it->value().ToString() ;
	cout << endl;
	it->Next();
	}

    it.reset();
#elif BERKELEYDB_VERSION
    DBC *cursorp=NULL;
    DBT key, data;
    int ret;
    string upperstring;
    if(upper_key!=NULL)
	{
	upperstring.assign(upper_key);
	}
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    bool first=true;
    if((ret=dbp->cursor(dbp, txn, &cursorp, 0))!=0)
	{
	cerr << "Cannot init cursor "<< db_strerror(ret) << endl;
	}
    for(;;)
	{
	if(first && lower_key!=NULL)
	    {
	    key.data=lower_key;
	    key.size=strlen(lower_key);
	    ret=cursorp->get(cursorp, &key, &data, DB_SET_RANGE);
	    }
	else
	    {
	    ret=cursorp->get(cursorp, &key, &data, DB_NEXT);
	    }
	if(ret!=0) break;
	first=false;
	if(upper_key!=NULL)
	    {
	    if(upperstring.compare(0,upperstring.size(),(char*)key.data,key.size)<0) break;
	    }
	cout.write((char*)key.data,key.size);
	cout << delim;
	cout.write((char*)data.data,data.size);
	cout << endl;
	}
    cursorp->close(cursorp);
#else
    sqlite3_stmt* stmt_dump=prepare_dump();
    if(stmt_dump==NULL) return EXIT_FAILURE;
    while(::sqlite3_step(stmt_dump) == SQLITE_ROW)
        {
        cout << (const char*)sqlite3_column_text(stmt_dump,0);
        cout << delim;
        cout << (const char*)sqlite3_column_text(stmt_dump,1);
        cout << endl;
        }
    ::sqlite3_finalize(stmt_dump);
#endif
    return EXIT_SUCCESS;
    }


int DataStore::put(const char* key,size_t lenkey,const char* data,size_t lendata)
    {
#ifdef LEVELDB_VERSION
    leveldb::Slice key1(key,lenkey);
    leveldb::Slice value(data,lendata);
    leveldb::WriteOptions opt;
    leveldb::Status status = db->Put(opt, key1, value);
    if(!status.ok())
	{
	cerr << "Could not insert:"  << status.ToString() << endl;
	return EXIT_FAILURE;
	}
#elif BERKELEYDB_VERSION
    DBT key1, data1;
    memset(&key1, 0, sizeof(DBT));
    memset(&data1, 0, sizeof(DBT));
    key1.data = (char*)key;
    key1.size = lenkey;
    data1.data = (char*)data;
    data1.size = lendata;
    int ret = dbp->put(dbp, txn, &key1, &data1, 0);
    if(ret!=0)
	{
	cerr << "Could not insert:"  << db_strerror(ret) << endl;
	return EXIT_FAILURE;
	}
#else
    for(int i=1;i<=2;++i)
        {
        if(::sqlite3_bind_text(
            stmt_put,i,
            (i==1?key:data),
            (i==1?lenkey:lendata),
            NULL)!=SQLITE_OK)
            {
            cerr << "Cannot bind key["<< i << "]\n";
            return EXIT_FAILURE;
            }
        }
    if (::sqlite3_step(stmt_put) != SQLITE_DONE)
        {
        cerr << "Could not step (execute) stmt" << endl;
        return EXIT_FAILURE;
        }
#endif
    return EXIT_SUCCESS;
    }

int DataStore::get(const char* key,size_t lenkey)
    {
#ifdef LEVELDB_VERSION
    leveldb::Slice key1(key,lenkey);
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), key1, &value);
    if(status.ok())
	{
	cout.write(key,lenkey);
	cout << delim;
	cout << value;
	cout << endl;
	}
#elif BERKELEYDB_VERSION
    DBT key1, data;
    memset(&key1, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key1.data =(char*) key;
    key1.size = lenkey;
    int ret=dbp->get(dbp, txn, &key1, &data, 0);
    if(ret==0)
	{
	cout.write((char*)key,lenkey);
	cout << delim;
	cout.write((char*)data.data,data.size);
	cout << endl;
	}
#else
    if(::sqlite3_bind_text(
        stmt_get,1,
        key,
        lenkey,
        NULL)!=SQLITE_OK)
        {
        cerr << "Cannot bind key[1]\n";
        return EXIT_FAILURE;
        }
    while(sqlite3_step(stmt_get) == SQLITE_ROW)
        {
        cout.write(key,lenkey);
        cout << delim;
        cout << (const char*)sqlite3_column_text(stmt_get,0);
        cout << endl;
        }
#endif
    return EXIT_SUCCESS;
    }


int DataStore::scanfile(std::istream& in)
    {
    int ret=0;
    string line;
    while(getline(in,line,'\n'))
        {
        if(line.empty()) continue;
        switch(program)
            {
            case DATASTORE_GET:
                {
                if((ret=get(line.c_str(),line.size()))!=EXIT_SUCCESS)
                    {
                    return ret;
                    }
                break;
                }
            case DATASTORE_RM:
                {
                if((ret=rm(line.c_str(),line.size()))!=EXIT_SUCCESS)
                    {
                    return ret;
                    }
                break;
                }
            case DATASTORE_PUT:
                {
                string::size_type n=line.find(delim,0);
                if(n==string::npos)
                    {
                    cerr << "Cannot find delimiter in "<< line << endl;
                    return EXIT_FAILURE;
                    }
                string key=line.substr(0,n);
                string value=line.substr(n+1);
                if((ret=put(
                    key.c_str(),key.size(),
                    value.c_str(),value.size()
                    ))!=EXIT_SUCCESS)
                    {
                    return ret;
                    }
                break;
                }
            default:
                {
                cerr << "Runtime error. Not handled."<< endl;
                return EXIT_FAILURE;
                }
            }

        }
    return ret;
    }
#ifdef TODO
int DataStore::join(DataStore* left,DataStore* right)
    {
    sqlite3_stmt* stmtL=left->prepare_dump();

    if(stmtL==NULL) return EXIT_FAILURE;
    sqlite3_stmt* stmtR=right->prepare_dump();
    bool eofR=false;
    if(stmtR==NULL)
        {
        ::sqlite3_finalize(stmtL);
        return EXIT_FAILURE;
        }
    list<string> bufferR;
    while(sqlite3_step(stmtL) == SQLITE_ROW)
            {
            string sL((const char*)sqlite3_column_text(stmtL,0));


            while(buffer.empty() && !eofR)
                {
                if(sqlite3_step(stmtR) != SQLITE_ROW)
                    {
                    eofR=true;
                    break;
                    }
                string sR((const char*)sqlite3_column_text(stmtR,0));
                buffer.push_back(sR);
                }

            if(buffer.empty())
                {
                cout << sL << delim << delim <<endl;
                }

            while(!buffer.empty())
                {
                list<string>::iterator r=buffer.begin();
                string sR=(*r);
                if(sR<sL)
                    {
                    cout << delim << delim << sR <<endl;
                    buffer.erase(r);
                    if(sqlite3_step(stmtR)== SQLITE_ROW)
                        {
                        sR.assign((const char*)sqlite3_column_text(stmtR,0));
                        buffer.push_back(sR);
                        }
                    continue;
                    }
                else if(sR==sL)
                    {
                    cout << delim << sR << delim <<endl;
                    buffer.erase(r);
                    break;
                    }
                else
                    {
                    cout << sL << delim << delim <<endl;
                    break;
                    }
                }

            }
    while(sqlite3_step(stmtR) == SQLITE_ROW)
        {
        cout << delim << delim << (const char*)sqlite3_column_text(stmtR,0) <<endl;
        }
    ::sqlite3_finalize(stmtR);
    ::sqlite3_finalize(stmtL);
    return EXIT_SUCCESS;
    }
#endif

int DataStore::main(int argc,char** argv)
    {
    int optind=1;
    std::vector<string> filenames;
    DataStore ds;
    char* progname=argv[0];
    while(optind< argc)
        {
        if(strcmp(argv[optind],"-h")==0)
            {
            //usage();
            return 0;
            }
        else if(strcmp(argv[optind],"-d")==0 && optind+1< argc)
            {
            ds.db_home=argv[++optind];
            }
        else if(strcmp(argv[optind],"-t")==0 && optind+1< argc)
            {
            ds.delim=argv[++optind][0];
            }
        else if(strcmp(argv[optind],"-f")==0 && optind+1< argc)
            {
            filenames.push_back(argv[++optind]);
            }
        else if(argv[optind][0]=='-')
            {
            cerr << "unknown option \""<< argv[optind]<< "\"" <<endl;
            return EXIT_FAILURE;
            }
        else if(strcmp(argv[optind],"--")==0 && optind+1< argc)
            {
            ++optind;
            break;
            }
        else
            {
            break;
            }
        ++optind;
        }

    if(strequals(progname,"sel") || strequals(progname,"get") || strequals(progname,"select"))
        {
        ds.program=DATASTORE_GET;
        }
    else if(strequals(progname,"put") || strequals(progname,"insert") )
        {
        ds.program=DATASTORE_PUT;
        }
    else if(strequals(progname,"del") || strequals(progname,"delete") || strequals(progname,"rm") || strequals(progname,"remove"))
        {
        ds.program=DATASTORE_RM;
        }
    else if(strequals(progname,"dump"))
        {
        ds.program=DATASTORE_DUMP;
        }
    else
        {
        cerr << "Undefined program.\n";
        return EXIT_FAILURE;
        }

    if(ds.db_home==NULL)
        {
        cerr << "db-home missing\n";
        return EXIT_FAILURE;
        }
    if(ds.open()!=EXIT_SUCCESS)
        {
        return EXIT_FAILURE;
        }
    if(ds.program==DATASTORE_DUMP)
        {
        if(optind!=argc)
            {
            cerr << "Illegal number of arguments.\n";
            return EXIT_FAILURE;
            }
        return ds.dump();
        }
    else if(optind==argc && !filenames.empty())
        {
        for(size_t i=0;i< filenames.size();++i)
            {
            ifstream in(filenames[i].c_str(),ios::in);
            ds.scanfile(in);
            in.close();
            }
        }
    else if(optind==argc)
        {
        ds.scanfile(cin);
        }
    else
        {
        switch(ds.program)
            {
            case DATASTORE_GET:
                {
                while(optind<argc)
                    {
                    ds.get(argv[optind],strlen(argv[optind]));
                    ++optind;
                    }
                break;
                }
            case DATASTORE_RM:
                {
                while(optind<argc)
                    {
                    ds.rm(argv[optind],strlen(argv[optind]));
                    ++optind;
                    }
                break;
                }
            case DATASTORE_PUT:
                {
                while(optind+1<argc)
                    {
                    ds.put(
                            argv[optind],strlen(argv[optind]),
                            argv[optind+1],strlen(argv[optind+1])
                            );
                    optind+=2;
                    }
                break;
                }
            default: cerr << "Not handled.\n"; return EXIT_FAILURE;break;
            }
        }
    return EXIT_SUCCESS;
    }



int main(int argc,char** argv)
    {
    DataStore ds;
    if(argc==1)
        {
        return 0;
        }
    return DataStore::main(argc-1,&argv[1]);
    }
