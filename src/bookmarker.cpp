#include <sqlite3.h>
#include "lindenb/net/cgi.h"
#include "lindenb/xml/escape.h"

/**
Compilation:
g++ -o /srv/www/cgi-bin/a.cgi  -DIMAGEDB_FILE=/data/db.sqlite -I ../.. -I ${SQLITE3} -L ${SQLITE3LIBS} bookmarker.cpp -lsqlite3 
*/
class ImageDB
	{
	public:
		 sqlite3* connection;
		 ImageDB():connection(NULL)
		 	{
		 	}
		 ~ImageDB()
		 	{
		 	close();
		 	}
		 
		 void close()
		 	{
		 	if(connection==NULL) return;
		 	::sqlite3_close(connection);
		 	connection=NULL;
		 	}
		 	
		 void open()
		 	{
		 	char *error=NULL;
		 	close();
		 	if(::sqlite3_open(IMAGEDB_FILE,&connection)!=SQLITE_OK)
		 		{
		 		throw std::runtime_error("Cannot open file "IMAGEDB_FILE);
		 		}
		 	if(::sqlite3_exec(connection,
				"create table if not exists images("
				"id INTEGER PRIMARY KEY,"
				"title TEXT,"
				"url TEXT UNIQUE,"
				"img TEXT,"
				"width INTEGER ,"
				"height INTEGER "
				")", NULL,NULL,&error
				)!=SQLITE_OK)
				{
				close();
				std::ostringstream os;
				os << "Cannot create table : " <<":" << error;
				throw std::runtime_error(os.str());
				}
		 	}
		
		
	};

/** callback called by the 'selecy' query */
static int callback_select(
	void* notUsed,
	int argc,/* number of args */
	char** argv,/* arguments as string */
	char** columns /* labels for the columns */
	)
	{
	if(argv[1]==NULL) return 0;
	int width=atoi(argv[3]);
	
	std::cout
		<< "<a href='"<< lindenb::xml::escape(argv[0]==NULL?"":argv[0]) << "'>"
		<< "<img src='"<< lindenb::xml::escape(argv[1]==NULL?"":argv[1]) << "' width='200'/>"
		<< "</a> ";
	return 0;
	}


int main(int argc,char** argv)
	{
	lindenb::net::CGI cgi;
	try
		{
		cgi.parse();
		}
	catch(...)
		{
		std::cout << "Content-Type: text/html\n\n";
		std::cout << "CGI::Error\n";
		return 0;
		}
	const char* action=cgi.getParameter("action");
	if(action==NULL)
		{
		std::cout << "Content-Type: text/html\n\n";
		std::cout << "<html><body>";
		std::cout << "</body></html>";
		}
	else if(strcmp(action,"script")==0)
		{
		std::cout << "Cache-Control: no-cache\n"
			<< "Content-Type: text/plain\n\n";
		
		std::cout << 
		"var BOOKMARKER={\n" <<
		"selecting:false,\n" <<
		"cliked:function(event)\n" <<
		"\t{\n" <<
		"\tif(!BOOKMARKER.selecting) return true;\n" <<
		"\tevent.stopPropagation();\n" << 
		"\tevent.preventDefault();\n" << 
		"\tBOOKMARKER.selecting=false;\n" <<
		"\tvar img=event.target;\n" <<
		"\tif(img==null || \"img\"!=img.localName.toLowerCase() ) return false;\n" <<
		"        var src= img.src;\n" <<
		"        if(src==null || src==\"\")\n" <<
		"                {\n" <<
		"                return true;\n" <<
		"                }\n" <<
		"\tdocument.location=(\"http://" << getenv("SERVER_NAME") << ":" <<
		getenv("SERVER_PORT")  << 
		getenv("SCRIPT_NAME") << 
		"?\"+\n" <<
		"\t\t\"action=post1\"+\n" <<
		"\t\t\"&url=\"+escape(document.location.href)+\n" <<
		"\t\t\"&title=\"+escape(document.title)+\n" <<
		"\t\t\"&width=\"+escape(img.width)+\n" <<
		"\t\t\"&height=\"+escape(img.height)+\n" <<
		"\t\t\"&img=\"+escape(src)\n" <<
		"\t\t);\n" <<
		"\treturn false;\n" << 
		"\t},\n" <<
		"startup:function(event)\n" <<
		"\t{\n" <<
		"\tBOOKMARKER.selecting=true;\n" <<
		"\t},\n" <<
		"init:function()\n" <<
		"\t{\n" <<
		"\tvar bodies=document.getElementsByTagName(\"body\");\n" <<
		"\tif(bodies.length==0) return;\n" <<
		"\tvar div=document.createElement(\"div\");\n" <<
		"\tdiv.style.margin=\"3px\";\n" <<
		"\tdiv.style.padding=\"3px\";\n" <<
		"\tdiv.style.right=\"5px\";\n" <<
		"\tdiv.style.top=\"1px\";\n" <<
		"\tdiv.style.border=\"solid 1px black\";\n" <<
		"\tdiv.style.position=\"fixed\";\n" <<
		"\tdiv.style.border=\"solid 1px black\";\n" <<
		"\tdiv.style.background=\"white\";\n" <<
		"\tvar button=document.createElement(\"button\");\n" <<
		"\tbutton.setAttribute(\"onclick\",\"javascript:BOOKMARKER.startup()\");\n" <<
		"\tbutton.appendChild(document.createTextNode(\"push\"));\n" <<
		"\tdiv.appendChild(button);\n" <<
		"\tbodies[0].appendChild(div);\n" <<
		"\twindow.addEventListener(\"mousedown\",BOOKMARKER.cliked,true);\n" <<
		"\treturn false;\n" <<
		"\t}\n" <<
		"}\n" <<
		//"window.addEventListener(\"load\",BOOKMARKER.init,false);\n"
		"BOOKMARKER.init();"
		;
		}
	else if(strcmp(action,"post1")==0 &&
		cgi.hasParameter("img") &&
		cgi.hasParameter("url"))
		{
		const char* width= cgi.getParameter("width");
		const char* height= cgi.getParameter("height");
		const char* title= cgi.getParameter("title");
		std::cout << "Content-Type: text/html\n\n";
		std::cout << "<html><body>" <<
			"<form action='" << getenv("SCRIPT_NAME") <<"' method='GET'>" <<
			"<input type='submit'/><br/>"<<
			"<input type='hidden' name='action' value='post2'/>" <<
			"<input type='hidden' name='width' value='"<< lindenb::xml::escape(width==NULL?"-1":width) <<"'/>" <<
			"<input type='hidden' name='height' value='"<<  lindenb::xml::escape(height==NULL?"-1":height) <<"'/>" <<
			"<input type='hidden' name='url' value='"<< lindenb::xml::escape(cgi.getParameter("url")) << "'/>" <<
			"<input type='hidden' name='img' value='"<< lindenb::xml::escape(cgi.getParameter("img")) << "'/>" <<
			"<img src='"<< lindenb::xml::escape(cgi.getParameter("img"))<< "'><br/>" <<
			"<label for='title'>Title:</label><input  id='title' name='title' value='"<< lindenb::xml::escape(cgi.getParameter("title"))<< "'/><br/>" <<
			"</form></body></html>";
		}
	else if(strcmp(action,"post2")==0 &&
		cgi.hasParameter("img") &&
		cgi.hasParameter("url"))
		{
		try
			{
			ImageDB db;
			db.open();
			const char* width= cgi.getParameter("width");
			const char* height= cgi.getParameter("height");
			const char* title= cgi.getParameter("title");
			
			
			char *query = sqlite3_mprintf(
				"insert into images(title,url,img,width,height) values(%Q,%Q,%Q,%d,%d)",
				 (title==NULL?"":title),
				 cgi.getParameter("url"),
				 cgi.getParameter("img"),
				 (width==NULL || atoi(width)<=0?-1:atoi(width)),
				 (height==NULL || atoi(height)<=0?-1:atoi(height))
				 );

			int ret=::sqlite3_exec(db.connection, query, 0, 0, 0);
			::sqlite3_free(query);
			
			db.close();
			std::cout << "Content-Type: text/html\n\n";
			std::cout << "<html><head><body><a href='" <<
			 lindenb::xml::escape(cgi.getParameter("url")) << 
			 "'>continue</a></body></head></html>"
			 ;
			}
		catch(std::exception& err)
			{
			std::cout << "Content-Type: text/plain\n\nError:"<< err.what();
			}
		catch(...)
			{
			std::cout << "Content-Type: text/plain\n\nError";
			}
		
		
		}
	else if(strcmp(action,"bookmarklet")==0 &&
		cgi.hasParameter("img") &&
		cgi.hasParameter("url"))
		{
		std::cout << "Content-Type: text/html\n\n";
		std::cout << "<html><body><a href=\"javascript:"<<
			"void((function(){"<<
			"var%20e=document.createElement('script');"<<
			"e.setAttribute('type','text/javascript');"<<
			"e.setAttribute('src','http://"<<
			getenv("SERVER_NAME") << ":" << getenv("SERVER_PORT") <<
			getenv("SCRIPT_NAME") << "?action=script')"<<
			";document.body.appendChild(e)})())"
			<< "\">drag me</a>";
		std::cout << "</body></html>";
		}
	else if(strcmp(action,"list")==0)
		{
		char* errormsg;
		const char* page=cgi.getParameter("page");
		std::cout << "Content-Type: text/html\n\n";
		std::cout << "<html><body>";
		ImageDB db;
		db.open();
		int i=0;
		std::ostringstream os;
		os << "select url,img,title,width,height from images limit ";
		if(page!=NULL && atoi(page)>0)
			{
			os << " 100 OFFSET "<< (1+(atoi(page)*100));
			}
		else
			{
			os <<  " 100";
			}

		
		if(sqlite3_exec(
			db.connection, /* An open database */
			os.str().c_str(), /* SQL to be evaluated */
			callback_select, /* Callback function */
			&i, /* 1st argument to callback */
			&errormsg /* Error msg written here */
			)!=SQLITE_OK)
			{
			std::cout << "Cannot select.\n";
			}
		db.close();
		std::cout << "</body></html>";
		}
	else
		{
		std::cout << "Content-Type: text/html\n\n";
		std::cout << "<html><body/></html>";
		}
	std::cout.flush();
	return 0;
	}