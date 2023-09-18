#include "MemoryMar.h"
#include <memory.h>


MemoryMar::MemoryMar()
{
	init_szAlloc(0, 64, &mem64);
	init_szAlloc(65, 128, &mem128);
	init_szAlloc(129, 256, &mem256);
	init_szAlloc(257, 512, &mem512);
	init_szAlloc(513, 1024, &mem1024);
}


MemoryMar::~MemoryMar()
{
}

MemoryMar& MemoryMar::Instance()
{
	static MemoryMar mgr;
	return mgr;
}

void* MemoryMar::allocMem(size_t size)
{
	if (size <= MAX_MEMORY_SIZE)
	{
		return m_szAlloc[size]->allocMemory(size);
	}
	else
	{
		MemoryBlock* pReturn = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
		memset(pReturn, 0, size + sizeof(MemoryBlock));
		pReturn->bPool = false;
		pReturn->nID = -1;
		pReturn->nRef = 1;
		pReturn->pAlloc = nullptr;
		pReturn->pNext = nullptr;
		xPrintf("allocMem: %lx,id = %d,size = %d\n", pReturn, pReturn->nID, size);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}
	
}

void MemoryMar::freeMem(void* pMem)
{
	if (!pMem)
		return;
	//要取到这块内存的内存单元块，才能知道它是系统的还是内存池里的
	MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
	if (pBlock->bPool)
	{
		pBlock->pAlloc->freeMemory(pMem);
	}
	else
	{
		//不在池内，直接free
		if (--pBlock->nRef == 0)
			free(pBlock);
	}
}

void MemoryMar::init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA)
{
	//大小从begin到end的内存，都映射到pMemA
	for (int n = nBegin; n <= nEnd; n++)
	{
		m_szAlloc[n] = pMemA;
	}
}
