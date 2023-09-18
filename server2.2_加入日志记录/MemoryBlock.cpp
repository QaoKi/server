#include "MemoryBlock.h"
#include "MemoryAlloc.h"


MemoryBlock::MemoryBlock()
{
	pAlloc = nullptr;
	pNext = nullptr;
}


MemoryBlock::~MemoryBlock()
{
}
