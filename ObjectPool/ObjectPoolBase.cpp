#include "ObjectPoolBase.h"


template<class Type>
void* ObjectPoolBase<Type>::operator new(size_t nSize)
{
	return malloc(nSize);
}

template<class Type>
void ObjectPoolBase<Type>::operator delete(void* p)
{
	free(p);
}

template<class Type,typename ...Args>
static Type* createObject(Args ... args)
{
	Type* obj = new Type(args...);
	//可以做点初始化或者其他想做的事
	return obj;
}

template<class Type>
void ObjectPoolBase<Type>::destroyObject(Type* obj)
{
	delete obj;
}