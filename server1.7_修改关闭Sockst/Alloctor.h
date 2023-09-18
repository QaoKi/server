#ifndef _Alloctor_H
#define _Alloctor_H
#include "MemoryMar.h"

/*
	Allcotor.h和Allcotor.cpp不是类，所以无法保存一个MemoryMar的实例对象来使用
	所以需要MemoryMar设为单例
*/

void* operator new(size_t size);

void operator delete(void* p);

void* operator new[](size_t size);

void operator delete[](void* p);

void* me_alloc(size_t size);

void me_free(void* p);

#endif // !_Alloctor_H
