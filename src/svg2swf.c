
/**
 * Author:
 *	Pierre Lindenbaum PhD
 * WWW
 *	http://plindenbaum.blogspot.com
 * Motivation:
 *	Learning MING (C API for Flash). Transform a simple SVG file (the same I used for 
 *	http://plindenbaum.blogspot.com/2008/12/genetic-algorithm-with-darwins-face.html )
 *	to flash with some random position/frame
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ming.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define NUM_FRAME 5

/** returns XML  attribute at int */
static int intProp(xmlNodePtr node,const char* name)
	{
	int i;
	xmlChar* s=xmlGetProp(node,BAD_CAST name);
	if(s==NULL) { fprintf(stderr,"@%s missing",name); exit(EXIT_FAILURE);}
	i=atoi((const char*)s);
	xmlFree(s);
	return i;
	}
/** returns XML  attribute at float */
static float decProp(xmlNodePtr node,const char* name)
	{
	double i;
	xmlChar* s=xmlGetProp(node,BAD_CAST name);
	if(s==NULL) { fprintf(stderr,"attribute missing"); exit(EXIT_FAILURE);}
	i=atof((const char*)s);
	xmlFree(s);
	return (float)i;
	}

/** returns wether the node has a given name */
static int isA(xmlNodePtr node,const char* localName)
	{
	return strcmp( (const char*) node->name, localName)==0;
	}

/** set the fill and opacity for this shape from the SVG attribute @style */
static void style(xmlNodePtr node,SWFShape shape)
	{
	int r=0,g=0,b=0,a=255;
	xmlChar* s=xmlGetProp(node,BAD_CAST "style");
	char* p=(char*)s;
	if(s==NULL) return;
	p=strstr((char*)s,"fill:");
	if(p!=NULL)
		{
		p+=5;
		while(isspace(*p)) {++p;}
		if(strncmp(p,"none",4)==0)
			{
			a=0;
			goto bail;
			}
		if(strncmp(p,"rgb(",4)==0)
			{
			char* p2;
			p+=4;
			r=(int)strtol(p,&p2,10);
			if(*p2!=',') goto bail;
			p=p2+1;
			g=(int)strtol(p,&p2,10);
			if(*p2!=',')  goto bail;
			p=p2+1;
			b=(int)strtol(p,&p2,10);
			}
		}
	p=strstr((char*)s,"opacity:");
	if(p!=NULL)
		{
		p+=8;
		a=(int)(255.0*strtod(p,NULL));
		}
	SWFShape_setRightFillStyle(shape, newSWFSolidFillStyle(r,g,b,a));
	bail:
	xmlFree(s);
	}
/** process the SVG tree recursively, create some basic shapes */
static void recurs(xmlNodePtr node,SWFMovie swf,int loop)
	{
	xmlNodePtr child;	
	if(node->type == XML_ELEMENT_NODE)
		{
		if(isA(node,"rect"))
			{
			SWFShape shape;
			double x=decProp(node,"x");
			double y=decProp(node,"y");
			double width=decProp(node,"width");
			double height=decProp(node,"height");
			shape = newSWFShape();
			style(node,shape);
			SWFShape_movePenTo(shape,x,y);
			SWFShape_drawLineTo(shape,x+width,y);
			SWFShape_drawLineTo(shape,x+width,y+height);
			SWFShape_drawLineTo(shape,x,y+height);
			SWFShape_drawLineTo(shape,x,y);
			SWFMovie_add(swf, shape);
			}
		else if(isA(node,"g"))
			{
					
			}
		else if(isA(node,"polygon"))
			{
			SWFShape shape;			
			char* p;
			int nPoints=0,x0,y0;
			xmlChar* s=xmlGetProp(node,BAD_CAST "points");
			shape = newSWFShape();
			style(node,shape);
			
			if(s==NULL) { fprintf(stderr,"@points missing"); exit(EXIT_FAILURE);}
			p=(char*)s;
			
			while(p!=NULL && *p!=0)
				{
				char* p2;
				int i;
				int x,y;
				
				if(isspace(*p)) {++p; continue;}
				p2=strchr(p,',');
				if(p2==NULL) break;	
				*p2=0;
				x=atoi(p);
				p=p2+1;
				y=(int)strtol(p,&p2,10);
				p=p2;
				//add a little random
				x+=(int)((rand()%NUM_FRAME)*loop)*(rand()%2==0?-1:1);
				y+=(int)((rand()%NUM_FRAME)*loop)*(rand()%2==0?-1:1);;
				if(nPoints==0)
					{
					SWFShape_movePenTo(shape,x,y);
					x0=x;
					y0=y;
					}
				else
					{
					SWFShape_drawLineTo(shape,x,y);
					}
				
				++nPoints;
				}
			SWFShape_drawLineTo(shape,x0,y0);
			xmlFree(s);
			SWFMovie_add(swf, shape);
			}
		else
			{
			}
		}

	for (child = node->children;child!=NULL; child = child->next)
		{
		recurs(child,swf,loop);
		}

	if(isA(node,"svg"))
		{
		SWFMovie_setDimension(swf,intProp(node,"width"),intProp(node,"height"));
		}
	}

/** get the XML root and run the recursive stuff , more init should go there later */
static int run(xmlDocPtr dom,SWFMovie swf,int loop)
	{
	int status=EXIT_SUCCESS;
	xmlNodePtr root_element = xmlDocGetRootElement(dom);
	if(root_element==NULL) return -1;
	recurs(root_element,swf,loop);
	return status;
	}


int main(int argc,char** argv)
         {
         int optind=1;
	char *outfile=NULL;
        LIBXML_TEST_VERSION
	/* loop over args */
         while(optind < argc)
                {
                if(strcmp(argv[optind],"-h")==0)
                        {
                        fprintf(stderr,"%s: Pierre Lindenbaum PHD. 2010.\n",argv[0]);
                        fprintf(stderr,"Compilation: %s at %s.\n",__DATE__,__TIME__);
                        fprintf(stderr," -o output filename \n");
                        exit(EXIT_FAILURE);
                        }
                else if(strcmp(argv[optind],"-o")==0 && optind+1<argc)
                        {
                        outfile=argv[++optind];
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

	if(outfile==NULL)
		{
		fprintf(stderr,"output name missing\n");
		exit( EXIT_FAILURE);
		} 
	fprintf(stderr,"start.\n");

        if(optind+1==argc)
                {
		int i;
		SWFMovie movie;		
		xmlDocPtr dom;
		int ret=0;
		Ming_init();
                char* fname=argv[optind++];
                dom=xmlParseFile(fname);
		if(dom==NULL)
			{
			fprintf(stderr,"Cannot read XML file %s\n",fname);
			exit( EXIT_FAILURE);
			}
		//create movie
		movie=newSWFMovie();
		if(movie==NULL)
			{
			fprintf(stderr,"cannot create swf movie.\n");
			exit( EXIT_FAILURE);
			}
		//configure movie
		SWFMovie_setBackground(movie, 0x00, 0x00, 0x00);
		SWFMovie_setRate(movie, 1);
		SWFMovie_setNumberOfFrames(movie, NUM_FRAME);
		//create each frame
		for(i=0;i< NUM_FRAME;++i)
			{
			ret=run(dom,movie,i);
			SWFMovie_nextFrame(movie);
			}
		xmlFreeDoc(dom);
		
		//save movie
		Ming_setSWFCompression(9);
		
		SWFMovie_save(movie,outfile);
		destroySWFMovie(movie);
		return ret;
		}
	else
		{
		fprintf(stderr,"Illegal number of arguments\n");
		exit(EXIT_FAILURE);
		}
        return EXIT_FAILURE;
        }


