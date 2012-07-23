#include <vector>
#include <string>
#include <cstring>
#include <stdint.h>
typedef double f64_t;

class Spreadsheet;

class Row
	{
	private:
		Spreadsheet* _owner;
		f64_t* _data;
		uint32_t _size;
	public:
		Row(Spreadsheet* owner,uint32_t n):_owner(owner),_size(n)
			{
			_data=new f64_t[_size];
			std::memset((void*)_data,0,_size*sizeof(f64_t) );
			}
		~Row()
			{
			delete [] _data;
			}
		uint32_t size() const
			{
			return _size;
			}
		f64_t at(uint32_t idx) const
			{
			return _data[idx];
			}	
	};

class Spreadsheet
	{
	private:
		std::vector<Row*> rows;
	public:
		Spreadsheet()
			{
			}
		~Spreadsheet()
			{
			}
		const Row* at(uint32_t y) const
			{
			return rows.at(y);
			}
		const f64_t at(uint32_t y,uint32_t x) const
			{
			return at(y)->at(x);
			}
	};
enum ScalarType { type_floating,type_error};
struct Scalar
	{
	union 
		{
		f64_t d;
		} core;
	ScalarType type;
	};

class Context
	{
	public:
		Spreadsheet* spreadsheet;
	};

class Node
	{
	private:
		Node* parent;
	public:
		Node()
			{
			}
		virtual ~Node()
			{
			}
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)=0; 
	};

class ColumnNode:public Node
	{
	private:
		uint32_t col;
	public:
		ColumnNode(uint32_t col):col(col)
			{
			}
		virtual ~ColumnNode()
			{
			}
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)
			{
			result->core.d=ctx->spreadsheet->at(yIndex,this->col);
			}
	};

class ConstantNode:public Node
	{
	private:
		f64_t data;
	public:
		ConstantNode(f64_t data):data(data)
			{
			}
		virtual ~ConstantNode()
			{
			}
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)
			{
			result->core.d=this->data;
			}
	};

class YNode:public Node
	{
	protected:
		std::vector<Node*> children;
		YNode() {}
		
		Node* at(uint32_t idx)
			{
			return children.at(idx);
			}
	public:
		virtual ~YNode();
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)=0;
	};

class MulNode:public YNode
	{
	public:
		MulNode() {}
		virtual ~YNode();
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)
			{
			Scalar c1,c2;
			at(0)->eval(yIndex,ctx,&c1);
			if(c1.type==type_error) { result->type=c1.type; return;}
			at(1)->eval(yIndex,ctx,&c2);
			if(c2.type==type_error) { result->type=c2.type; return;}
			result->core.d=c1->core.d + c2->core.d;
			}
	};


class UnaryFunction:public YNode
	{
	public:
		UnaryFunction() {}
		virtual ~UnaryFunction();
		virtual void eval(f64_t d1,Scalar* result)=0;
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)
			{
			Scalar c1;
			at(0)->eval(yIndex,ctx,&c1);
			if(c1.type==type_error) { result->type=c1.type; return;}
			eval(c1->core.d,result);
			}
	};

class BinaryFunction:public YNode
	{
	public:
		BinaryFunction() {}
		virtual ~BinaryFunction();
		virtual void eval(f64_t d1, f64_t d2,Scalar* result)=0;
		virtual void eval(uint32_t yIndex,Context* ctx,Scalar* result)
			{
			Scalar c1,c2;
			at(0)->eval(yIndex,ctx,&c1);
			if(c1.type==type_error) { result->type=c1.type; return;}
			at(1)->eval(yIndex,ctx,&c2);
			if(c2.type==type_error) { result->type=c2.type; return;}
			eval(c1->core.d,c2->core.d,result);
			}
	};

#define MAKE_NODE2(NodeName,implement_logic) \
class NodeName:public BinaryFunction \
	{ \
	public: \
		NodeName() {} \
		virtual ~NodeName(); \
		virtual void eval(f64_t d1, f64_t d2,Scalar* result) \
			{\
			logic ;\
			}\
	};


