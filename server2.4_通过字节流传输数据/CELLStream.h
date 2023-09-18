#ifndef _CELL_STREAM_H_
#define _CELL_STREAM_H_
#include <cstdint>
#include <memory>
//字节流
class CELLStream
{
public:
	/*
	两种传入数据到字节流的方式，一种是手动申请一块内存，然后写入
	一种是数据在外部申请好内存，直接把数据写入，并告诉字节流，这块内存是否需要字节流来释放
	*/
	//直接传入数据，并告诉有多大
	CELLStream(char* pData, int nSize, bool bDelete = false);
	//手动申请内存写入数据
	CELLStream(int nSize = 1024);
	virtual ~CELLStream();
public:
	char* getData();
	//在不同的操作系统和平台上面，基础数据类型的大小是不一样的
	//int8_t是 8位整数，8位是一个字节
	//int16_t是 16位整数，16位是两个字节，int32_t是四个字节，这样就固定了大小

	//字节流写入数据以后，_nWritePos需要加上n
	//inline函数的声明和定义也写到一起
	inline void push(int n)
	{
		_nWritePos += n;
	}

	//字节流读出数据以后，_nReadPos需要加上n
	inline void pop(int n)
	{
		_nReadPos += n;
	}

	inline void setWritePos(int n)
	{
		_nWritePos = n;
	}
	inline int getWritePos()
	{
		return _nWritePos;
	}

	//read，从字节流中读数据
	int8_t ReadInt8();	
	int16_t ReadInt16();
	int32_t ReadInt32();
	float ReadFloat();
	double ReadDouble();
	//读取数组数据
	template<typename T>
	uint32_t ReadArray(T* pArr, uint32_t len)
	{
		//参数len是缓存数组能存放多少个元素
		//先读取数组元素个数
		uint32_t num = 0;
		//让_nReadPos先不要向后偏移，因为不确定读数据是否能成功
		//如果直接偏移了，数组元素个数就没法再次读了
		OnlyRead(num);
		//判断缓存数组内存是否够
		if (len > num)
		{
			//要取出数据的字节长度
			auto nLen = num * sizeof(T);
			//判断能不能读出
			//此时_nReadPos指向数组元素个数，向后偏移sizeof(uint32_t)才是数据的开始
			if (canRead(nLen + sizeof(uint32_t)))
			{
				//在这里，确定能读取数据，才让_nReadPos偏移
				pop(sizeof(uint32_t));
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				pop(nLen);
				//返回实际读取的元素的个数
				return num;
			}
		}
	}


	//write，向字节流中写数据
	//写入8位整数，相当于写入1个字节，对应char类型，返回是否成功
	bool WriteInt8(int8_t n);
	//写入16位整数，相当于写入2个字节，对应short类型，返回是否成功
	bool WriteInt16(int16_t n);
	//写入32位整数，相当于写入4个字节，对应int类型，返回是否成功
	bool WriteInt32(int32_t n);
	//写入float类型
	bool WriteFloat(float n);
	//写入double类型
	bool WriteDouble(double n);

	/*
		写入数组不需要为每一种类型,比如int，char再额外定义方法，因为数组在
		定义的时候，类型已经确定了，所以调用WriteArray的时候会隐式转换
		而上面的基础类型之所以要重定义那么多方法
		因为比如传入Write(5),此时会默认为int类型，是4个字节
		而WriteInt8(5)，会作为1个字节存储	
	*/
	//因为有模板函数，如果.h和.cpp分开，调用模板函数，编译的时候会报错
	//所以模板函数的声明和实现放到一起
	template<typename T>
	bool WriteArray(T* pData, uint32_t len)
	{
		//计算要写入的数组的字节长度
		auto nLen = sizeof(int) * len;
		//当要放入的数据长度，小于等于缓冲区空余的长度，将数据放到缓冲区
		//因为也要把数组元素个数写进去，所以要加一段 sizeof(uint32_t)
		if (canWrite(nLen + sizeof(len)))
		{
			//只写入数组的数据，当读取数组的时候，无法知道数组有多长
			//所以写入数组数据之前，先把数组的元素个数写入，所以len用uint32_t类型
			//如果用别的类型，在各个系统下面长度又会不一样
			Write(len);
			memcpy(_pBuff + _nWritePos, pData, nLen);
			push(nLen);
			return true;
		}
		return false;
	}
public:

	//读数据模板，并且读完以后，_nReadPos向下偏移
	//当读取数组数据的时候，要先读取数组元素的个数，读取完以后_nReadPos就向后移动了
	//如果接下来读数据没有成功，那么就不能再读出数组元素个数了，
	//所以加一个只读数据，不让_nReadPos偏移的方法OnlyRead()
	template<typename T>
	T Read(T& n)
	{
		//要读取的数据长度
		auto nLen = sizeof(T);
		//有没有数据
		if (canRead(nLen))
		{
			//将要读取的数据，拷贝出来
			memcpy(&n, _pBuff + _nReadPos, nLen);
			pop(nLen);
			return n;
		}
		return -1;
	}

	template<typename T>
	T OnlyRead(T& n)
	{
		//要读取的数据长度
		auto nLen = sizeof(T);
		//有没有数据
		if (canRead(nLen))
		{
			//将要读取的数据，拷贝出来，不让_nReadPos偏移
			memcpy(&n, _pBuff + _nReadPos, nLen);
			return n;
		}
		return -1;
	}

	//写入基础数据的模板函数
	template<typename T>
	bool Write(T n)
	{
		auto nlen = sizeof(T);
		//当要放入的数据长度，小于等于缓冲区空余的长度，将数据放到缓冲区
		if (canWrite(nlen))
		{
			memcpy(_pBuff + _nWritePos, &n, nlen);
			push(nlen);
			return true;
		}
		return false;
	}

private:

	//判断能否进行读写
	inline bool canRead(int n);
	inline bool canWrite(int n);

	//数据缓冲区
	char* _pBuff = nullptr;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;
	//已有的数据尾部位置，已有的数据长度
	int _nWritePos = 0;
	//已读取数据的尾部位置，从哪里开始读
	int _nReadPos = 0;
	//当_pBuff是外部传入的数据时，是否应该由字节流释放
	bool _bDelete = true;
};


#endif // _CELL_STREAM_H_
