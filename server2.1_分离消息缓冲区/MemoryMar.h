#ifndef _MemoryMar_H
#define _MemoryMar_H
#include <stdlib.h>
#include "MemoryAlloc.h"

//内存管理工具

//打印调试信息，只有在debug模式下才会输出
#ifdef _DEBUG
#include <stdio.h>
//(...) 表示任意个参数 
	#define xPrintf(...) Printf(__VA_ARGS__)
#else
	#define xPrintf(...)
#endif



//最大内存单元
#define MAX_MEMORY_SIZE 1024

class MemoryMar
{
private:
	MemoryMar();
	~MemoryMar();

public:
	static MemoryMar& Instance();
	//申请内存
	void* allocMem(size_t size);
	//释放内存
	void freeMem(void* p);
private:
	//初始化内存池映射数组
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA);

	MemoryAlloc mem64{64,100000};
	MemoryAlloc mem128{ 128,100000 };
	MemoryAlloc mem256{ 256,100000 };
	MemoryAlloc mem512{ 512,100000 };
	MemoryAlloc mem1024{ 1024,100000 };
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];
};

#endif
