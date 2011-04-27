/**
 * Author:
 *      Pierre Lindenbaum PhD
 * Contact:
 *      plindenbaum@yahoo.fr
 * WWW:
 *      http://plindenbaum.blogspot.com
 * Motivation:
 *      Genetic Painting
 * Compilation:
 *	g++  -o geneticpainting -O3 geneticpainting01.cpp  -lgd -lpthread
 */
/* example html file:
 
 
 <html>
<head>
<script type="text/javascript" src="_painting.js"></script>
<script type="text/javascript">
function paintLW0gTUxECg(param)
	{
	param.ctx=param.circle.getContext("2d");
	param.ctx.fillStyle = "rgb(255,255,255)";
	param.ctx.fillRect (0,0, solution.width,solution.height);
	for(var i=0;i+7< param.array.length;i+=7)
		{
		param.ctx.beginPath();
		param.ctx.arc(
			param.array[i+0],
			param.array[i+1], 
			(param.n > param.array[i+2] ? param.array[i+2] : param.n),
			0, Math.PI*2, true);
		param.ctx.closePath();
		param.ctx.fillStyle = "rgb("+
			param.array[i+3]+","+
			param.array[i+4] +","+
			param.array[i+5] +")";
		param.ctx.globalAlpha= param.array[i+6];
		param.ctx.fill();
		}
	param.ctx.fillStyle = "rgb(0,0,0)";
	//param.ctx.drawRect(0,0, solution.width-1,solution.height-1);
	if(param.n< param.maxRadius)
		{
		param.n++;
		setTimeout(paintLW0gTUxECg,param.time,param);
		}
	
	}
function initLW0gTUxECg()
	{
	var param={maxRadius:0,n:1,circle:null,ctx:null,array:[],time:50};
	param.circle = document.getElementById("x1");
	if (!param.circle.getContext)return;
	param.circle.setAttribute("width",solution.width);
	param.circle.setAttribute("height",solution.height);
	param.ctx=param.circle.getContext("2d");
	param.array= solution.shapes;
	
	for(var i=0;i+7< param.array.length;i+=7)
		{
		if(param.maxRadius < param.array[i+2])
			{
			param.maxRadius=param.array[i+2];
			}
		}
	setTimeout(paintLW0gTUxECg,param.time,param);
	}
</script>
</head>
<body onload="initLW0gTUxECg();"><canvas id="x1"/></body>
</html>
*/
 
 
#include <gd.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <cerrno>
#include <pthread.h>
enum e_shape_type {
	e_circle,e_rect,e_line
	};
	
static int min_shapes_per_solution=100;
static int max_shapes_per_solution=500;
static int parents_per_generation=10;	
static int max_size_radius=50;
static e_shape_type shape_type=e_circle;
static std::string filenameout("_painting");
class Image;
class Random;


class Random
	{
	public:
		Random(unsigned int seed)
			{
			std::srand(seed );
			}
		Random()
			{
			std::srand( std::time(NULL));
			}
		int nextInt(int n)
			{
			return std::rand()%n;
			}
		double nextDouble()
			{
			return std::rand()/(double)RAND_MAX;
			}
		bool nextBool()
			{
			return nextInt(10000)<5000;
			}

	};

static Random* RANDOM=NULL;
#define ENDS_WITH(a,b)  (std::strlen(a) >= std::strlen(b) && strcasecmp(&(a)[std::strlen(a)-std::strlen(b)],b)==0)

class Image
	{
	private:
		gdImagePtr img;
	public:
		Image(const char* filename):img(NULL)
			{
			
			std::FILE* in=std::fopen(filename,"rb");
			if(in==NULL) throw std::runtime_error("cannot open PNG image");
			if(ENDS_WITH(filename,".png"))
				{
				img=::gdImageCreateFromPng(in);
				}
			else if(ENDS_WITH(filename,".jpg") || ENDS_WITH(filename,".jpeg"))
				{
				img=::gdImageCreateFromJpeg(in);
				}
			else if(ENDS_WITH(filename,".gif"))
				{
				img=::gdImageCreateFromGif(in);
				}
			else
				{
				std::cerr << "unknown format" << std::endl;
				}
			std::fclose(in);
			if(img==NULL) throw std::runtime_error("cannot read image");
			}
		gdImagePtr gd()
			{
			return img;
			}
		Image(int width,int height)
			{
			img=::gdImageCreateTrueColor(width,height);
			if(img==NULL) throw std::runtime_error("cannot read PNG image");
			}
		int width() 
			{
			return gdImageSX(gd());
			}
		int height() 
			{
			return gdImageSY(gd());
			}
		~Image()
			{
			::gdImageDestroy(gd());
			}
		
		void saveAsPng(const char* filename)
			{
			//std::cerr << "saving as " << filename << "\n";
			std::FILE* out=std::fopen(filename,"wb");
			if(out==NULL) throw std::runtime_error("cannot open PNG image");
			gdImagePng(gd(),out);
			std::fclose(out);
			}
	};
static Image* imageSource=NULL;


class Shape
	{
	protected:
		int mut255(int n)
			{
			n+= 1-RANDOM->nextInt(3);
			if(n<0) return 0;
			if(n>=255) return 255;  
			}
	public:
		int fill;
		Shape():fill(0)
			{
			}
		virtual ~Shape()
			{
			}
		virtual void paint(Image* img)=0;
		virtual Shape* clone()=0;
		virtual void mute(Image* image)=0;
		virtual void svg(std::ostream& out,Image* image)=0;
		virtual void html(std::ostream& out,Image* image)=0;
		virtual e_shape_type type()=0;
		virtual void muteColor(Image* image)
			{
			int r=mut255(gdImageRed(image->gd(),fill));
			int g=mut255(gdImageGreen(image->gd(),fill));
			int b=mut255(gdImageBlue(image->gd(),fill));
			int a=mut255(gdImageAlpha(image->gd(),fill));
			if(a<=0) a=1;
			this->fill=::gdImageColorAllocateAlpha(image->gd(),r,g,b,a);
			}
	};


class Circle:public Shape
	{
	public:

		int cx;
		int cy;
		int radius;
		Circle()
		{
		
		}
	virtual ~Circle()
		{
		
		}
	virtual void paint(Image* img)
		{
		//std::cerr << cx << "x" << cy << " " << radius << std::endl;
		::gdImageFilledEllipse(img->gd(),cx,cy,radius*2,radius*2,fill);
		}
	virtual Shape* clone()
		{
		Circle* c=new Circle;
		c->cx=cx;
		c->cy=cy;
		c->radius=radius;
		c->fill=fill;
		return c;
		}
	virtual void mute(Image* image)
		{
		for(;;)
			{
			int d= 10-RANDOM->nextInt(21);
			if(d==0 || this->radius+d <= 0) continue;
			this->radius+=d;
			break;
			};
		
		this->cx+=2-RANDOM->nextInt(5);
		this->cy+=2-RANDOM->nextInt(5);
		muteColor(image);
		}
	
	static Circle* create(Image* image)
		{
		Circle* c=new Circle;
		c->cx= RANDOM->nextInt(imageSource->width());
		c->cy= RANDOM->nextInt(imageSource->height());
		c->radius=1+RANDOM->nextInt(max_size_radius);
		int alpha=RANDOM-> nextInt(255);
		int col=::gdImageGetPixel(imageSource->gd(),c->cx,c->cy);
		assert(col!=-1);
		c->fill=::gdImageColorAllocateAlpha(
			image->gd(),
			gdImageRed(imageSource->gd(),col),
			gdImageGreen(imageSource->gd(),col),
			gdImageBlue(imageSource->gd(),col),
			alpha
			);
		assert(c->fill!=-1);
		return c;
		}
	virtual void svg(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out << "<circle cx='" << cx << "' cy='" << cy << "' r='" << radius << "' style='stroke:none;fill:rgb("
			<< r << "," << g << "," << b << ");fill-opacity:"<<(1.0-(a/255.0)) <<"'/>\n"
			;
		}
		
	virtual void html(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out  << cx << "," << cy << "," << radius << ","
			<< r << "," << g << "," << b << ","<< (1.0-(a/255.0))
			;
		}
	virtual e_shape_type type()
		{
		return e_circle;
		}
	};


class Line:public Shape
	{
	public:

		int x;
		int y;
		int length;
		int thickness;
		Line()
		{
		}
		
	virtual ~Line()
		{
		
		}
	virtual void paint(Image* img)
		{
		::gdImageSetThickness(img->gd(),thickness);
		::gdImageLine(img->gd(),
			x,y,x+length,y+length,fill);
		}
	virtual Shape* clone()
		{
		Line* c=new Line;
		c->x=x;
		c->y=y;
		c->length=length;
		c->thickness=thickness;
		c->fill=fill;
		return c;
		}
	virtual void mute(Image* image)
		{
		
		this->x+=2-RANDOM->nextInt(5);
		this->y+=2-RANDOM->nextInt(5);
		this->length+=2-RANDOM->nextInt(5);
		if(this->length<0) this->length=1;
		this->thickness+=2-RANDOM->nextInt(5);
		if(this->thickness<0) this->thickness=1;
		muteColor(image);
		}
	
	static Shape* create(Image* image)
		{
		Line* c=new Line;
		c->x= RANDOM->nextInt(imageSource->width());
		c->y= RANDOM->nextInt(imageSource->height());
		c->length=1+RANDOM->nextInt(max_size_radius);
		c->thickness=1+RANDOM->nextInt(max_size_radius);
		int alpha=RANDOM-> nextInt(255);
		int col=::gdImageGetPixel(imageSource->gd(),c->x,c->y);
		assert(col!=-1);
		c->fill=::gdImageColorAllocateAlpha(
			image->gd(),
			gdImageRed(imageSource->gd(),col),
			gdImageGreen(imageSource->gd(),col),
			gdImageBlue(imageSource->gd(),col),
			alpha
			);
		assert(c->fill!=-1);
		return c;
		}
	virtual void svg(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out << "<line x1='" << x << "' y1='" << y
			<< "' x2='" << (x+length) << 
			"' y2='" << (y+length) << 
			"' style='stroke-width:"<< (thickness) << "px;fill:none;stroke:rgb("
			<< r << "," << g << "," << b << ");stroke-opacity:"<<(1.0-(a/255.0)) <<";stroke-linecap:round;'/>\n"
			;
		}
		
	virtual void html(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out  << x << "," << y << ","
			<< length << ","<< thickness <<","
			<< r << "," << g << "," << b << ","<< (1.0-(a/255.0))
			;
		}
	virtual e_shape_type type()
		{
		return e_line;
		}
	};


class Rect:public Shape
	{
	public:
		int x;
		int y;
		int width;
		int height;
	Rect()
		{
		
		}
	virtual ~Rect()
		{
		
		}
	virtual void paint(Image* img)
		{
		::gdImageFilledRectangle(img->gd(),x,y,x+width,y+height,fill);
		}
	virtual Shape* clone()
		{
		Rect* c=new Rect;
		c->x=x;
		c->y=y;
		c->width=width;
		c->height=height;
		c->fill=fill;
		return c;
		}
	virtual void mute(Image* image)
		{
		this->x+=2-RANDOM->nextInt(5);
		this->y+=2-RANDOM->nextInt(5);
		this->width+=2-RANDOM->nextInt(5);
		this->height+=2-RANDOM->nextInt(5);
		this->width = std::max(1,this->width);
		this->height = std::max(1,this->height);
		muteColor(image);
		}
	
	static Rect* create(Image* image)
		{
		Rect* c=new Rect;
		c->x= RANDOM->nextInt(imageSource->width());
		c->y= RANDOM->nextInt(imageSource->height());
		c->width= RANDOM->nextInt(max_size_radius);
		c->height= RANDOM->nextInt(max_size_radius);
		
		int alpha=RANDOM-> nextInt(255);
		int col=::gdImageGetPixel(imageSource->gd(),c->x,c->y);
		assert(col!=-1);
		c->fill=::gdImageColorAllocateAlpha(
			image->gd(),
			gdImageRed(imageSource->gd(),col),
			gdImageGreen(imageSource->gd(),col),
			gdImageBlue(imageSource->gd(),col),
			alpha
			);
		assert(c->fill!=-1);
		return c;
		}
	virtual void svg(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out << "<rect x='" << x << "' y='" << y
			<< "' width='" <<  width 
			<< "' height='" <<  height 
			<< "' style='stroke:none;fill:rgb("
			<< r << "," << g << "," << b << ");fill-opacity:"<<(1.0-(a/255.0)) <<"'/>\n"
			;
		}
		
	virtual void html(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out  << x << "," << y << ","
			<< width << "," << height <<","
			<< r << "," << g << "," << b << ","<< (1.0-(a/255.0))
			;
		}
	virtual e_shape_type type()
		{
		return e_rect;
		}
	};



class Solution
	{
	public:
		long fitness;
		int generation;
		std::vector<Shape*> shapes;
		Solution():fitness(0L),generation(0)
			{
			}
			
		~Solution()
			{
			while(!shapes.empty())
				{
				delete shapes.back();
				shapes.pop_back();
				}
			}
		void paint(Image* image)
			{
			for(int j=0;j< shapes.size();++j)
				{
				shapes[j]->paint(image);
				}
			}
		Solution* clone()
			{
			Solution* s=new Solution;
			s->fitness=fitness;
			s->generation=generation;
			for(int j=0;j< shapes.size();++j)
				{
				
				s->shapes.push_back(shapes[j]->clone());
				}
			return s;
			}
		void saveSvg(const char* filename,Image* image)
			{
			std::fstream out(filename, std::ios::out);
			if(!out.is_open())
				{
				std::cerr<< "Cannot open "<< filename << std::endl;
				return;
				}
			out 	<< "<?xml version='1.0' encoding='UTF-8' ?>\n"
				<< "<svg xmlns='http://www.w3.org/2000/svg' "
				<<" width='" << imageSource->width()<< "' height='" <<  imageSource->height()<< "'>\n"
				;
			out << "<title>" << generation <<" := "<< fitness << "</title>\n";
			out <<"<rect x='0' y='0' style='fill:none;' width='" << imageSource->width()<< "' height='" <<  imageSource->height()<< "'/>\n";
			for(int j=0;j< shapes.size();++j)
				{
				shapes[j]->svg(out,image);
				}
			out << "</svg>\n" ;
			out.flush();
			out.close();
			}
		void saveHtml(const char* filename,Image* image)
			{
			std::fstream out(filename, std::ios::out);
			if(!out.is_open())
				{
				std::cerr<< "Cannot open "<< filename << std::endl;
				return;
				}
			
			out << "var solution={width:"<< image->width()<<",height:" << image->height() <<",shapes:[";
			for(int j=0;j< shapes.size();++j)
				{
				if(j>0) out << ",";
				shapes[j]->html(out,image);
				}
			out << "]};";
			out.flush();
			out.close();
			}	
		
	};

static bool compareSolutions (const Solution* s1, const Solution* s2)
	{
	return s1->fitness < s2->fitness;
	}

static bool compareByRadius (const Shape* s1, const Shape* s2)
	{
	switch(shape_type)
		{
		case e_circle : return ((Circle*)s1)->radius > ((Circle*)s2)->radius ;
		case e_rect : return 	((Rect*)s1)->width * ((Rect*)s1)->height >
				   	((Rect*)s2)->width * ((Rect*)s2)->height 
				;
		case e_line : return ((Line*)s1)->length > ((Line*)s2)->length ;
		default: return s1 < s2;
		}
	}

static Shape* makeShape(Image* image)
	{
	switch(shape_type)
		{
		case e_circle: return Circle::create(image);
		case e_rect: return Rect::create(image);
		case e_line: return Line::create(image);
		default: break;
		}
	throw std::runtime_error("boum");
	}

static Solution* makeSolution(Image* image)
	{
	Solution* sol=new Solution();
	int n=min_shapes_per_solution+RANDOM->nextInt(max_shapes_per_solution-min_shapes_per_solution);
	while(n>0)
		{
		--n;
		Shape* newshape=::makeShape(image);
		sol->shapes.push_back(newshape);
		}
	if(RANDOM->nextBool())
		{
		std::sort(sol->shapes.begin(),sol->shapes.end(),::compareByRadius);
		}
	return sol;
	}

static Solution* mate(Solution* sol1,Solution* sol2,Image* image)
	{
	if(RANDOM->nextBool()) std::swap(sol1,sol2);
	Solution* sol=new Solution();

	int n1= RANDOM->nextInt(sol1->shapes.size());
	int n2= RANDOM->nextInt(sol2->shapes.size());
	for(int i=0;i< n1;++i)
		{
		sol->shapes.push_back( sol1->shapes[i]->clone() );
		}
	for(int i=n2;i< sol2->shapes.size(); ++i)
		{
		sol->shapes.push_back( sol2->shapes[i]->clone() );
		}
	if(RANDOM->nextBool())
		{
		int n3= RANDOM->nextInt(sol->shapes.size());
		sol->shapes[n3]->mute(image);
		};
	if(sol->shapes.size()>2 && RANDOM->nextBool())
		{
		int n3= RANDOM->nextInt(sol->shapes.size());
		delete sol->shapes[n3];
		sol->shapes.erase(sol->shapes.begin()+n3);
		}
	if(RANDOM->nextBool())
		{
		int n3= RANDOM->nextInt(sol->shapes.size());
		sol->shapes.insert(sol->shapes.begin()+n3,makeShape(image));
		}	
	if(sol->shapes.size()>2 && RANDOM->nextBool())
		{
		int n3= RANDOM->nextInt(sol->shapes.size());
		int n4= RANDOM->nextInt(sol->shapes.size());
		Shape* s3= sol->shapes[n3];
		Shape* s4= sol->shapes[n4];
		sol->shapes[n3]=s4;
		sol->shapes[n4]=s3;
		}
	while(sol->shapes.size()>600)
		{
		delete *(sol->shapes.begin());
		sol->shapes.erase(sol->shapes.begin());
		}
	return sol;
	}

struct Runnable
	{
	int status;
	pthread_t thread;
	gdImagePtr gd;
	int rowStart;
	int rowEnd;
	long count;
	};

static void* fitness( void *ptr )
	{
	Runnable* t=(Runnable*)ptr;
	t->count=0;
	for(int y=t->rowStart;y< t->rowEnd;++y)
		{
		for(int x=0;x< imageSource->width();++x)
			{
			int c1 = ::gdImageGetPixel(imageSource->gd(), x,y);
			int c2 = ::gdImageGetPixel(t->gd, x,y);
			
			t->count+=
				(
				
				std::abs( gdImageRed(imageSource->gd(),c1) - gdImageRed(t->gd,c2) ) +
				std::abs( gdImageGreen(imageSource->gd(),c1) - gdImageGreen(t->gd,c2) ) +
				std::abs( gdImageBlue(imageSource->gd(),c1) - gdImageBlue(t->gd,c2)) 
				)
				;
			
			}
		}
	return NULL;
	}
	

	
static void train()
	{
	std::vector<Solution*> solutions;
	Image img2(imageSource->width(),imageSource->height());
	int white=gdImageColorAllocate(img2.gd(),0,0,0);
	int n_generation=0;
	Solution* best=NULL;
	
	int nThread=std::min(imageSource->height()/10,5);
	if(nThread<=0) nThread=1;
	int nRowsByThread=imageSource->height()/nThread;
	std::vector<Runnable> threads(nThread);
	
	for(;;)
		{
		++n_generation;
		while(solutions.size()< parents_per_generation+1)
			{
			Solution* solution=makeSolution(&img2);
			solutions.push_back(solution);
			}
		if(best!=NULL && RANDOM->nextBool())
			{
			solutions.push_back(best->clone());
			}
		
		std::vector<Solution*> children;
		for(int i=0;i< solutions.size();++i)
			{
			for(int j=0;j< solutions.size();++j)
				{
				//if(i==j) continue;
				children.push_back(mate(solutions[i],solutions[j],&img2));
				}
			}
		for(int i=0;i< children.size();++i)
			{
			
			::gdImageFilledRectangle(img2.gd(),0,0,img2.width(),img2.height(),white);
			Solution* solution=children[i];		
			solution->paint(&img2);
			solution->fitness=0L;
			for(int t=0;t< threads.size();++t)
				{
				threads[t].gd=img2.gd();
				threads[t].rowStart=(t*nRowsByThread);
				threads[t].rowEnd=(t+1==threads.size()?img2.height():(t+1)*nRowsByThread);
				threads[t].status= ::pthread_create( &(threads[t].thread), NULL,fitness,(void*)&threads[t]);
				if(threads[t].status!=0)
					{
					std::cerr << "cannot create thread " << t  <<std::endl;
					std::exit(EXIT_FAILURE);
					}
				}
			solution->fitness=0;
			for(int t=0;t< threads.size();++t)
				{
				::pthread_join( threads[t].thread, NULL);
				solution->fitness+= threads[t].count;
				}
			
			}
		std::sort(children.begin(),children.end(),::compareSolutions);
		
		
		
		while(children.size()>parents_per_generation)
			{
			delete children.back();
			children.pop_back();
			}
		if(!children.empty())
			{
			if(best==NULL)
				{
				best=children[0]->clone();
				best->generation=n_generation;
				}
			else if(best->fitness > children[0]->fitness)
				{
				delete best;
				std::cout << n_generation<< ":" << children[0]->fitness << std::endl;
				best=children[0]->clone();
				best->generation=n_generation;
				std::cerr << "SAVING " << n_generation << std::endl;
				::gdImageFilledRectangle(img2.gd(),0,0,img2.width(),img2.height(),white);	
				best->paint(&img2);
				std::string s(filenameout);
				s.append(".png");
				img2.saveAsPng(s.c_str());
				s.assign(filenameout).append(".svg");
				best->saveSvg(s.c_str(),&img2);
				s.assign(filenameout).append(".js");
				best->saveHtml(s.c_str(),&img2);
				}
			}	
			
		while(!solutions.empty())
			{
			delete solutions.back();
			solutions.pop_back();
			}
		
		for(int i=0;i< children.size();++i)
			{
			solutions.push_back(children[i]);
			}
		
		}
	}


int main(int argc,char** argv)
	{
	int optind=1;
	 while(optind < argc)
                {
                if(std::strcmp(argv[optind],"-h")==0)
                        {
                        std::cerr << argv[0] << ": Pierre Lindenbaum PHD. 2011" << std::endl;
                        std::cerr << "Compilation: "<< __DATE__ << " at " << __TIME__ << "." << std::endl;
                        std::cerr << "Options: "<< std::endl;
                        std::cerr << " -o <string> base filename out: "<<  filenameout << std::endl;
                        std::cerr << " -n1 <int> min shapes per solution: "<<min_shapes_per_solution <<  std::endl;
                        std::cerr << " -n2 <int> max shapes per solution: "<< max_shapes_per_solution << std::endl;
                        std::cerr << " -n3 <int> parents per generation: "<< parents_per_generation<< std::endl;
                        std::cerr << " -n4 <int> max size circle radius: "<<  max_size_radius << std::endl;
                        std::cerr << " -t <type> 'c'=circle 'r'=rect 'l'=line" << std::endl;
                        return(EXIT_SUCCESS);
                        }
                 else if(std::strcmp(argv[optind],"-n1")==0)
                        {
                        min_shapes_per_solution=atoi(argv[++optind]);
                        }
                else if(std::strcmp(argv[optind],"-n2")==0)
                        {
                        max_shapes_per_solution=atoi(argv[++optind]);
                        }
                else if(std::strcmp(argv[optind],"-n3")==0)
                        {
                        parents_per_generation=atoi(argv[++optind]);
                        }
                else if(std::strcmp(argv[optind],"-n4")==0)
                        {
                        max_size_radius=atoi(argv[++optind]);
                        } 
                else if(std::strcmp(argv[optind],"-o")==0)
                        {
                        filenameout.assign(argv[++optind]);
                        }
                else if(std::strcmp(argv[optind],"-t")==0)
                        {
                        ++optind;
                        if(std::strcmp(argv[optind],"c")==0)
                        	{
                        	shape_type=e_circle;
                        	}
                        else if(std::strcmp(argv[optind],"r")==0)
                        	{
                        	shape_type=e_rect;
                        	}
                         else if(std::strcmp(argv[optind],"l")==0)
                        	{
                        	shape_type=e_line;
                        	}
                        else 	{
                        	 std::cerr << "unknown shape type '"<<argv[optind] << "'" << std::endl;
                        	return(EXIT_FAILURE);
                        	}
                        } 
                else if(std::strcmp(argv[optind],"--")==0)
                        {
                        ++optind;
                        break;
                        }
                else if(argv[optind][0]=='-')
                        {
                        std::cerr << "unknown option '"<<argv[optind] << "'" << std::endl;
                        return(EXIT_FAILURE);
                        }
                else
                        {
                        break;
                        }
                ++optind;
                }
	if(optind+1!=argc)
		{
		std::cerr << "Illegal number of arguments" << std::endl;
                 return(EXIT_FAILURE);
		}
	RANDOM=new Random;
	imageSource=new Image(argv[optind++]);
	train();
	delete imageSource;
	delete RANDOM;
	return 0;
	}