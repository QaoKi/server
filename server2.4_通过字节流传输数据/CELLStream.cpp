#include "CELLStream.h"



CELLStream::CELLStream(char* pData, int nSize, bool bDelete /*= false*/)
{
	_nSize = nSize;
	_pBuff = pData;
	//push(nSize);
	_bDelete = bDelete;
}


CELLStream::CELLStream(int nSize /*= 1024*/)
{
	_nSize = nSize;
	_pBuff = new char[_nSize];
	_bDelete = true;
}

CELLStream::~CELLStream()
{
	if (_pBuff && _bDelete)
	{
		delete[] _pBuff;
		_pBuff = nullptr;
	}
}

char* CELLStream::getData()
{
	return _pBuff;
}
int8_t CELLStream::ReadInt8()
{
	int8_t def = 0;
	Read(def);
	return def;
}

int16_t CELLStream::ReadInt16()
{
	int16_t def = 0;
	Read(def);
	return def;
}

int32_t CELLStream::ReadInt32()
{
	int32_t def = 0;
	Read(def);
	return def;
}

float CELLStream::ReadFloat()
{
	float def = 0;
	Read(def);
	return def;
}

double CELLStream::ReadDouble()
{
	double def = 0;
	Read(def);
	return def;
}

bool CELLStream::WriteInt8(int8_t n)
{
	//因为n已经确定了类型，调用Write的时候，会隐式转换为Write<int8_t>(n)
	return Write(n);
}

bool CELLStream::WriteInt16(int16_t n)
{
	//因为n已经确定了类型，调用Write的时候，会隐式转换为Write<int16_t>(n)
	return Write(n);
}

bool CELLStream::WriteInt32(int32_t n)
{
	//因为n已经确定了类型，调用Write的时候，会隐式转换为Write<int32_t>(n)
	return Write(n);
}

bool CELLStream::WriteFloat(float n)
{
	//因为n已经确定了类型，调用Write的时候，会隐式转换为Write<float>(n)
	return Write(n);
}

bool CELLStream::WriteDouble(double n)
{
	//因为n已经确定了类型，调用Write的时候，会隐式转换为Write<double>(n)
	return Write(n);
}

bool CELLStream::canRead(int n)
{
	return _nSize - _nReadPos >= n;
}

bool CELLStream::canWrite(int n)
{
	return n + _nWritePos <= _nSize;
}