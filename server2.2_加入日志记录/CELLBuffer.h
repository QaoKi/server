#ifndef _CELL_BUFFER_H_
#define _CELL_BUFFER_H_

#ifdef _WIN32
#include<windows.h>
#else
#include<sys/socket.h>
#include<unistd.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include <memory>
#include "MessageHeader.hpp"

class CELLBuffer
{
public:
	//缓冲区默认是8k的大小
	CELLBuffer(int nSize = 8192);
	~CELLBuffer();

	//返回缓冲区
	char* getData();

	//向缓冲区加入数据
	bool push(const char* pData,int nLen);

	//从头删除缓冲区数据
	void pop(int nLen);

	//将缓冲区数据发送到socket
	int write2socket(SOCKET sockfd);
	
	//从socket中读取数据都缓冲区
	int read4socket(SOCKET sockfd);

	//判断缓冲区中的数据长度是否够一条消息的长度
	bool hasMag();

private:
	//缓冲区
	char* _pBuff = nullptr;
	//缓冲区也可以是链表或队列来管理缓冲数据库
	//list<char*> _pBuffList;
	//缓冲区的数据尾部位置，已有数据长度
	int _nLast = 0;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;

	//缓冲区写满次数计数，主要用于当做发送缓冲区时使用
	int _nFullCount = 0;

};
#endif

