#ifndef _CELLObjectPool_hpp_
#define _CELLObjectPool_hpp_

#include <stdlib.h>
#include <mutex>
#include "Alloctor.h"

//打印调试信息，只有在debug模式下才会输出
#ifdef _DEBUG
#ifndef xPrintf
	#include <stdio.h>
	//(...) 表示任意个参数 
	#define xPrintf(...) printf(__VA_ARGS__)
#endif
#else
#ifndef xPrintf
	#define xPrintf(...)
#endif
#endif

//对象池节点信息
class NodeHeader
{
public:
	//下一块位置
	NodeHeader* pNext;
	//内存块编号
	int nID;
	//引用次数
	char nRef;
	//是否在内存池中
	bool bPool;
private:
	//预留
	//char c1;
};

template<class Type,size_t nPoolSize>
class CELLObjectPool
{
public:
	CELLObjectPool()
	{
		m_nBufSize = sizeof(Type) + sizeof(NodeHeader);
		initPool();
	}
	~CELLObjectPool()
	{
		if (m_pBuff)
			delete[] m_pBuff;
	}
public:

	//申请对象
	void* allocObj(size_t size)
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		if (!m_pBuff)
			initPool();

		NodeHeader* pReturn = nullptr;
		//当对象池中没有对象了，用户再申请时，直接向内存池申请,并加上对象单元头信息
		if (m_pHeader == nullptr)
		{
			pReturn = (NodeHeader*)new char[m_nBufSize];
			memset(pReturn, 0, m_nBufSize);
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else
		{
			//对象池中有对象，直接返回
			pReturn = m_pHeader;
			pReturn->nRef = 1;
			m_pHeader = m_pHeader->pNext;
		}
		xPrintf("allocObj: %lx,id = %d,size = %d\n", pReturn, pReturn->nID, size);
		//只给用户返回实际可用内存，不返回内存单元块部分
		return ((char*)pReturn + sizeof(NodeHeader));
	}
	//释放对象
	void freeObj(void* pMem)
	{
		if (!pMem)
			return;
		//要取到这个对象的对象信息单元块，才能知道它是否是对象池中的
		NodeHeader* pBlock = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));

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
			//不在池内，直接delete
			delete[] pBlock;
		}
		
	}
private:
	//初始化对象池
	void initPool()
	{
		if (m_pBuff)
			return;
		//计算对象池的大小
		size_t size = nPoolSize * m_nBufSize;
		//像内存池申请内存
		m_pBuff = new char[size];

		//初始化对象池的头部
		m_pHeader = (NodeHeader*)m_pBuff;
		m_pHeader->bPool = true;
		m_pHeader->nID = 0;
		m_pHeader->nRef = 0;
		m_pHeader->pNext = nullptr;

		//初始化对象池中的其余节点
		NodeHeader* next = m_pHeader;
		for (size_t n = 1; n < nPoolSize; n++)
		{
			NodeHeader* temp = (NodeHeader*)(m_pBuff + n * m_nBufSize);
			temp->bPool = true;
			temp->nID = n;
			temp->nRef = 0;
			temp->pNext = nullptr;
			next->pNext = temp;
			next = temp;
		}
	}
	//头部
	NodeHeader* m_pHeader;
	//对象池内存缓冲区地址
	char* m_pBuff;
	//每个对象的大小，包括NodeHeader
	size_t m_nBufSize;
	std::mutex m_mutex;
	
};

//为了给每个类定义一个通用的createObject()和destroyObject()
//createObject()中可以做一些想做的事

template<class Type, size_t nPoolSize>
class ObjectPoolBase
{
public:

	void* operator new(size_t nSize)
	{
		return ObjectPoolBase::objectPool().allocObj(nSize);
	}

	void operator delete(void* p)
	{
		ObjectPoolBase::objectPool().freeObj(p);
	}

	//定义一个用于创建对象和销毁对象的函数
	template<typename ...Args>		//可变参数
	static Type* createObject(Args ... args)
	{
		Type* obj = new Type(args...);
		//做些想做的事
		return obj;
	}

	static void destroyObject(Type* obj)
	{
		delete obj;
	}

private:
	//同一个类，使用同一个对象池
	typedef CELLObjectPool<Type,nPoolSize> ClassTypePool;
	static ClassTypePool& objectPool()
	{
		//静态对象池对象
		static ClassTypePool sPool;
		return sPool;
	}
};

#endif // _ObjectPoolBase_H_


