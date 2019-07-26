#ifndef _CELL_NET_WORK_H_
#define _CELL_NET_WORK_H_
#include "CELL.h"

//设为单例，无论有多少个EasyServer对象，都共用一个
class CELLNetWork
{
private:
	CELLNetWork();
	/*
		静态变量在全局区，只有当程序退出的时候，才会被释放，在构造的时候，
		构造了一个全局的静态对象，之后无论申请多少个EasyServer对象，
		CELLNetWork的构造函数不再被调用
		当程序退出的时候，析构函数调用一次，网络环境被清除
	
	*/
	~CELLNetWork();

public:
	//不用返回对象，因为只要调用类的构造函数，网络环境就能初始化
	static void Init();
};

#endif // _CELL_NET_WORK_H_