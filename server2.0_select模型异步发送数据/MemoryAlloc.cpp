#include "MemoryAlloc.h"
#include <memory.h>

MemoryAlloc::MemoryAlloc(int nSize, int nBlockCount)
{
	_pBuff = nullptr;
	_pHeader = nullptr;
	//对nSize进行处理，使其与操作系统的字节对齐，32位是4字节对齐，64位是8字节
	//指针在32位下是4字节，64位下是8字节
	//比如，nSize传进来61,如果是8字节对齐，需要转为64
	const size_t n = sizeof(void*);
	size_t size = (nSize / n) * n + (nSize % n != 0 ? n : 0);
	_nBlockCount = nBlockCount;
	_nBufSize = size + sizeof(MemoryBlock);
}

MemoryAlloc::~MemoryAlloc()
{
	//将内存池释放，只释放了内存池的内存，没有释放向系统申请的内存
	if (_pBuff)
		free(_pBuff); 
}

void MemoryAlloc::initMemory()
{
	//防止重复申请
	if (_pBuff)
		return;

		
	//向系统申请池的内存
	_pBuff = (char*)malloc(_nBufSize * _nBlockCount);

	memset(_pBuff, 0, _nBufSize * _nBlockCount);
	//初始化内存池的头部内存块
	_pHeader = (MemoryBlock*)_pBuff;
	_pHeader->bPool = true;
	_pHeader->nID = 0;
	_pHeader->nRef = 0;
	_pHeader->pAlloc = this;
	_pHeader->pNext = nullptr;

	//初始化内存池中其他的内存块
	MemoryBlock* next = _pHeader;
	for (size_t n = 1; n < _nBlockCount; n++)
	{
		MemoryBlock* temp = (MemoryBlock*)(_pBuff + n * _nBufSize);
		temp->bPool = true;
		temp->nID = n;
		temp->nRef = 0;
		temp->pAlloc = this;
		next->pNext = temp;
		next = temp;
	}
}

void* MemoryAlloc::allocMemory(size_t size)
{
	std::lock_guard<std::mutex> lg(_mutex);
	if (!_pBuff)
		initMemory();

	MemoryBlock* pReturn = nullptr;
	//当内存池中没有内存块了，用户再申请时，直接向系统申请,并加上内存单元头信息
	if (_pHeader == nullptr)
	{
		pReturn = (MemoryBlock*)malloc(_nBufSize + sizeof(MemoryBlock));
		memset(pReturn, 0, _nBufSize + sizeof(MemoryBlock));
		pReturn->bPool = false;
		pReturn->nID = -1;
		pReturn->nRef = 1;
		pReturn->pAlloc = this;
		pReturn->pNext = nullptr;
	}
	else
	{
		//内存池中有内存块，直接返回
		pReturn = _pHeader;
		pReturn->nRef = 1;
		_pHeader = _pHeader->pNext;
	}
	xPrintf("allocMem:%lx,id = %d,size = %d\n", pReturn, pReturn->nID, size);
	//只给用户返回实际可用内存，不返回内存单元块部分
	return ((char*)pReturn + sizeof(MemoryBlock));
}

void MemoryAlloc::freeMemory(void* pMem)
{ 
	if (!pMem)
		return;
	//要取到这块内存的内存单元块，才能知道它是系统的还是内存池里的
	MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));

	if (--pBlock->nRef != 0)
	{
		//被多次引用
		return;
	}
	if (pBlock->bPool)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		pBlock->pNext = _pHeader;
		_pHeader = pBlock;
	}
	else
	{
		//不在池内，直接free
		free(pBlock);
	}
}
