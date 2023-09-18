#ifndef _MemoryBlock_H
#define _MemoryBlock_H
//#include "MemoryAlloc.h"

class MemoryAlloc;
//内存块  最小单元
//64为是8字节对齐，32为是4字节对齐
class MemoryBlock
{
public:
	MemoryBlock();
	~MemoryBlock();

public:
	//内存块编号
	int nID;
	//引用次数
	int nRef;
	//所属大内存块（池）
	MemoryAlloc* pAlloc;
	//下一块位置
	MemoryBlock* pNext;
	//是否在内存池中(当内存池中的块用完了，用户再申请，直接向系统申请)
	bool bPool;
private:
	//预留
	//char cNULL;
};

//int MemoryBlockSize = sizeof(MemoryBlock);
#endif // 

