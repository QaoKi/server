#ifndef _Alloctor_H
#define _Alloctor_H
#include "MemoryMar.h"

/*
	Allcotor.h和Allcotor.cpp不是类，所以无法保存一个MemoryMar的实例对象来使用
	所以需要MemoryMar设为单例
*/

void* operator new(size_t size);

//delete和delete[]是不会抛出异常的，所以要声明为不抛出异常函数
void operator delete(void* p) noexcept;

void* operator new[](size_t size);

void operator delete[](void* p) noexcept;

void* me_alloc(size_t size);

void me_free(void* p);

#endif // !_Alloctor_H
