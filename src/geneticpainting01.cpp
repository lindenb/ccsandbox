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

static int min_shapes_per_solution=100;
static int max_shapes_per_solution=500;
static int parents_per_generation=10;	
static int max_size_radius=50;
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





class Circle
	{
	public:
		int fill;
		int cx;
		int cy;
		int radius;
		Image* image;
		Circle():fill(0)
		{
		
		}
	~Circle()
		{
		
		}
	void paint(Image* img)
		{
		//std::cerr << cx << "x" << cy << " " << radius << std::endl;
		::gdImageFilledEllipse(img->gd(),cx,cy,radius*2,radius*2,fill);
		}
	virtual Circle* clone()
		{
		Circle* c=new Circle;
		c->cx=cx;
		c->cy=cy;
		c->radius=radius;
		c->fill=fill;
		return c;
		}
	virtual void mute()
		{
		radius+= 5-RANDOM->nextInt(10);
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
	void svg(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out << "<circle cx='" << cx << "' cy='" << cy << "' r='" << radius << "' style='stroke:none;fill:rgb("
			<< r << "," << g << "," << b << ");fill-opacity:"<<(1.0-(a/255.0)) <<"'/>\n"
			;
		}
	void html(std::ostream& out,Image* image)
		{
		int r=gdImageRed(image->gd(),fill);
		int g=gdImageGreen(image->gd(),fill);
		int b=gdImageBlue(image->gd(),fill);
		int a=gdImageAlpha(image->gd(),fill);
		out  << cx << "," << cy << "," << radius << ","
			<< r << "," << g << "," << b << ","<< (1.0-(a/255.0))
			;
		}
	};





class Solution
	{
	public:
		long fitness;
		std::vector<Circle*> shapes;
		Solution():fitness(0L)
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
			out << "<title>" << fitness << "</title>\n";
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
			
			out << "var solution={width="<< image->width()<<",height=" << image->height() <<",shapes=[";
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

bool compareSolutions (const Solution* s1, const Solution* s2)
	{
	return s1->fitness < s2->fitness;
	}

static Solution* makeSolution(Image* image)
	{
	Solution* sol=new Solution();
	int n=min_shapes_per_solution+RANDOM->nextInt(max_shapes_per_solution-min_shapes_per_solution);
	while(n>0)
		{
		--n;
		sol->shapes.push_back(Circle::create(image));
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
		sol->shapes[n3]->mute();
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
		sol->shapes.insert(sol->shapes.begin()+n3,Circle::create(image));
		}	
	if(sol->shapes.size()>2 && RANDOM->nextBool())
		{
		int n3= RANDOM->nextInt(sol->shapes.size());
		int n4= RANDOM->nextInt(sol->shapes.size());
		Circle* s3= sol->shapes[n3];
		Circle* s4= sol->shapes[n4];
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
		std::cout << n_generation<< ":" << children[0]->fitness << std::endl;
		
		
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
				}
			else if(best->fitness > children[0]->fitness)
				{
				delete best;
				best=children[0]->clone();
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