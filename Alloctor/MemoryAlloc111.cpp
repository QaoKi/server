#include "MemoryAlloc.h"
#include "MemoryBlock.h"

MemoryAlloc::MemoryAlloc()
{
	m_pBuff = nullptr;
	m_pHeader = nullptr;
	m_nSize = 0;
	m_nBlockSize = 0;
}


MemoryAlloc::~MemoryAlloc()
{
	//将内存池释放
	if (m_pBuff)
		free(m_pBuff); 
}

void MemoryAlloc::initMemory()
{
	//防止重复申请
	if (m_pBuff)
		return;
	//向系统申请池的内存
	m_pBuff = applicationMemory();
}

void* MemoryAlloc::allocMemory(size_t size)
{
	if (!m_pBuff)
		initMemory();

	MemoryBlock* pReturn = nullptr;
	//当内存池中没有内存块了，用户再申请时，内存池动态扩充
	if (m_pHeader == nullptr)
	{
		pReturn = (MemoryBlock*)malloc(m_nSize + sizeof(MemoryBlock));
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
		pBlock->pNext = m_pHeader;
		m_pHeader = pBlock;
	}
	else
	{
		//不在池内，直接free
		free(pBlock);
	}
}

char* MemoryAlloc::applicationMemory()
{
	//向系统申请池的内存
	char* pReturn = (char*)malloc(m_nSize * m_nBlockSize);

	//初始化内存池的头部内存块
	m_pHeader = (MemoryBlock*)pReturn;
	m_pHeader->bPool = true;
	m_pHeader->nID = 0;
	m_pHeader->nRef = 0;
	m_pHeader->pAlloc = this;
	m_pHeader->pNext = nullptr;

	//初始化内存池中其他的内存块
	MemoryBlock* next = m_pHeader;
	for (size_t n = 1; n < m_nBlockSize; n++)
	{
		MemoryBlock* temp = (MemoryBlock*)(pReturn + n * m_nSize);
		temp->bPool = true;
		temp->nID = 0;
		temp->nRef = 0;
		temp->pAlloc = this;
		next->pNext = temp;
		next = temp;
	}

	return pReturn;
}
