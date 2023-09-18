#include "MemoryAlloc.h"
#include <memory.h>

MemoryAlloc::MemoryAlloc(int nSize, int nBlockCount)
{
	m_pBuff = nullptr;
	m_pHeader = nullptr;
	//对nSize进行处理，使其与操作系统的字节对齐，32位是4字节对齐，64位是8字节
	//指针在32位下是4字节，64位下是8字节
	//比如，nSize传进来61,如果是8字节对齐，需要转为64
	const size_t n = sizeof(void*);
	size_t size = (nSize / n) * n + (nSize % n != 0 ? n : 0);
	m_nBlockCount = nBlockCount;
	m_nBufSize = size + sizeof(MemoryBlock);
}

MemoryAlloc::~MemoryAlloc()
{
	//将内存池释放，只释放了内存池的内存，没有释放向系统申请的内存
	if (m_pBuff)
		free(m_pBuff); 
}

void MemoryAlloc::initMemory()
{
	//防止重复申请
	if (m_pBuff)
		return;
	
	//向系统申请池的内存
	m_pBuff = (char*)malloc(m_nBufSize * m_nBlockCount);

	memset(m_pBuff, 0, m_nBufSize * m_nBlockCount);
	//初始化内存池的头部内存块
	m_pHeader = (MemoryBlock*)m_pBuff;
	m_pHeader->bPool = true;
	m_pHeader->nID = 0;
	m_pHeader->nRef = 0;
	m_pHeader->pAlloc = this;
	m_pHeader->pNext = nullptr;

	//初始化内存池中其他的内存块
	MemoryBlock* next = m_pHeader;
	for (size_t n = 1; n < m_nBlockCount; n++)
	{
		MemoryBlock* temp = (MemoryBlock*)(m_pBuff + n * m_nBufSize);
		temp->bPool = true;
		temp->nID = n;
		temp->nRef = 0;
		temp->pAlloc = this;
		temp->pNext = nullptr;
		next->pNext = temp;
		next = temp;
	}
}

void* MemoryAlloc::allocMemory(size_t size)
{
	std::lock_guard<std::mutex> lg(m_mutex);
	if (!m_pBuff)
		initMemory();

	MemoryBlock* pReturn = nullptr;
	//当内存池中没有内存块了，用户再申请时，直接向系统申请,并加上内存单元头信息
	if (m_pHeader == nullptr)
	{
		pReturn = (MemoryBlock*)malloc(m_nBufSize);
		memset(pReturn, 0, m_nBufSize);
		pReturn->bPool = false;
		pReturn->nID = -1;
		pReturn->nRef = 1;
		pReturn->pAlloc = this;
		pReturn->pNext = nullptr;
	}
	else
	{
		//内存池中有内存块，直接返回
		pReturn = m_pHeader;
		pReturn->nRef = 1;
		m_pHeader = m_pHeader->pNext;
	}
	xPrintf("allocMem: %lx,id = %d,size = %d\n", pReturn, pReturn->nID, size);
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
		std::lock_guard<std::mutex> lg(m_mutex);
		pBlock->pNext = m_pHeader;
		m_pHeader = pBlock;
	}
	else
	{
		//不在池内，直接free
		free(pBlock);
	}
}
