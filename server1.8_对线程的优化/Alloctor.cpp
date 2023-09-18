#include "Alloctor.h"
#include <stdlib.h>

void* operator new(size_t size)
{
	return MemoryMar::Instance().allocMem(size);
}

void operator delete(void* p)
{
	MemoryMar::Instance().freeMem(p);
}

void* operator new[](size_t size)
{
	return MemoryMar::Instance().allocMem(size);
}

void operator delete[](void* p)
{
	MemoryMar::Instance().freeMem(p);
}

void* me_alloc(size_t size)
{
	return malloc(size);
}

void me_free(void* p)
{
	free(p);
}
