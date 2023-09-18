#ifndef _MemoryAlloc_H
#define _MemoryAlloc_H
#include <stdlib.h>
#include "MemoryBlock.h"
#include<mutex>//锁

//打印调试信息，只有在debug模式下才会输出
#ifdef _DEBUG
#include <stdio.h>
//(...) 表示任意个参数 
#define xPrintf(...) printf(__VA_ARGS__)
#else
#define xPrintf(...)
#endif

//内存池
class MemoryAlloc
{
public:
	MemoryAlloc(int nSize, int nBlockCount);
	~MemoryAlloc();

	//初始化内存池
	//申请内存
	void initMemory();
	void* allocMemory(size_t size);
	//释放内存
	void freeMemory(void* p);
private:
	//内存池地址
	char* _pBuff;
	//头部内存单元,可用的第一个块
	MemoryBlock* _pHeader;
	//每个内存单元的大小，包括了单元信息块
	size_t _nBufSize;
	//内存单元的数量
	size_t _nBlockCount;

	std::mutex _mutex;
};

#endif

