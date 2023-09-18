#include "CELLBuffer.h"



CELLBuffer::CELLBuffer(int nSize /*= 8192*/)
{
	_nSize = nSize;
	_pBuff = new char[_nSize];
}

CELLBuffer::~CELLBuffer()
{
	if (_pBuff)
	{
		delete[] _pBuff;
		_pBuff = nullptr;
	}
}

char* CELLBuffer::getData()
{
	return _pBuff;
}

bool CELLBuffer::push(const char* pData, int nLen)
{
	if (!pData || nLen <= 0)
	{
		return false;
	}
	/*
		这里提供了一个当缓冲区不够时的解决方案，当写入大量数据时，
		也可以写入数据库或者磁盘中
	*/
	//当要放入的数据长度，大于缓冲区空余的长度,动态扩展
	if (nLen > _nSize - _nLast)
	{
		//空间差多少
		int len = _nLast + nLen - _nSize;
		//需要扩展几个_nSize的长度
		int num = len / _nSize + 1;

		//原来的数据还需要一个
		char* buff = new char[(num + 1)*_nSize];
		memcpy(buff, _pBuff, _nLast);
		_nSize = (num + 1)*_nSize;
		delete[] _pBuff;
		_pBuff = buff;
	}

	//当要放入的数据长度，小于等于缓冲区空余的长度，将数据放到缓冲区
	if (nLen <= _nSize - _nLast)
	{
		memcpy(_pBuff + _nLast, pData, nLen);
		_nLast += nLen;
		if (_nLast == _nSize)
		{
			_nFullCount++;
		}
		return true;
	}
	else
	{
		_nFullCount++;
	}
	return false;
}

void CELLBuffer::pop(int nLen)
{
	//nLen是要移除的长度
	int n = _nLast - nLen;
	if (n > 0)
	{
		//将后面的数据，覆盖到前面
		memcpy(_pBuff, _pBuff + nLen, n);
		_nLast = n;
	}
	else
	{
		memset(_pBuff, 0, sizeof(_pBuff));
		_nLast = 0;
	}
	if (_nFullCount > 0)
		--_nFullCount;
}

int CELLBuffer::write2socket(SOCKET sockfd)
{
	int nRet = SOCKET_ERROR;
	//缓冲区有数据
	//当_nLast == 0 说明缓冲区中没有数据，不算错误，返回0

	if (_nLast == 0)
	{
		nRet = 0;
	}
	else if (_nLast > 0 && INVALID_SOCKET != sockfd)
	{
		nRet = send(sockfd, _pBuff, _nLast, 0);

		if (nRet > 0)
		{
			//重置缓冲区指针位置
			_nLast = 0;
			//重置缓冲区满次数
			_nFullCount = 0;
		}
		else
		{
			//因为send()的返回值可能为0，send()返回0表示错误，所以要处理一下
			nRet = SOCKET_ERROR;
		}
	}
	return nRet;
}

int CELLBuffer::read4socket(SOCKET sockfd)
{
	//有空位置才去接收数据
	if (_nSize - _nLast > 0)
	{
		int nLen = recv(sockfd, _pBuff + _nLast, _nSize - _nLast, 0);
		//recv返回<=0算错误，返回-1
		if (nLen <= 0)
		{
			return SOCKET_ERROR;
		}
		//缓冲区的数据尾部位置后移
		_nLast += nLen;
	}
	//缓冲区没有空闲，返回0，不算错误
	return 0;
}

bool CELLBuffer::hasMag()
{
	//消息缓冲区中的数据，长度是否大于消息头DataHeader长度
	if (_nLast >= sizeof(netmsg_DataHeader))
	{
		//这时就可以知道当前消息的长度
		netmsg_DataHeader* header = (netmsg_DataHeader*)_pBuff;
		//消息缓冲区中的数据长度，是否大于等于当前消息的长度
		//如果大于等于，说明缓冲区中的数据够一条消息了
		return _nLast >= header->dataLength;
	}
	return false;
}
