#ifndef _MemoryAlloc_H
#define _MemoryAlloc_H
#include <stdlib.h>
//#include "MemoryBlock.h"

class MemoryBlock;
//内存池
class MemoryAlloc
{
public:
	MemoryAlloc();
	~MemoryAlloc();

	//初始化内存池
	//申请内存
	void initMemory();
	void* allocMemory(size_t size);
	//释放内存
	void freeMemory(void* p);

private:
	char* applicationMemory();
private:
	//内存池地址
	char* m_pBuff;
	//头部内存单元,可用的第一个块
	MemoryBlock* m_pHeader;
	//每个内存单元的大小，包括了单元信息块
	size_t m_nSize;
	//内存单元的数量
	size_t m_nBlockSize;

};

#endif

