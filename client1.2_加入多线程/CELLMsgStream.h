#ifndef _CELL_MSG_STREAM_H_
#define _CELL_MSG_STREAM_H_
#include "MessageHeader.hpp"
#include "CELLStream.h"

//消息数据字节流
class CELLRecvStream : public CELLStream
{
public:
	//接收的时候，消息形式过来都是netmsg_DataHeader的形式
	CELLRecvStream(netmsg_DataHeader* header);
	~CELLRecvStream();
};


class CELLSendStream : public CELLStream
{
public:
	CELLSendStream(netmsg_DataHeader* header);
	CELLSendStream(char* pData, int nSize, bool bDelete = false);
	CELLSendStream(int nSize = 1024);
	~CELLSendStream();

	//要发送的消息写完以后调用一次
	void finsh();
};

#endif // _CELL_MSG_STREAM_H_
