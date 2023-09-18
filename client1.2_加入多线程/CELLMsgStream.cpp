#include "CELLMsgStream.h"


CELLRecvStream::CELLRecvStream(netmsg_DataHeader* header)
	:CELLStream((char*)header, header->dataLength)
{
	
}

CELLRecvStream::~CELLRecvStream()
{
}

CELLSendStream::CELLSendStream(netmsg_DataHeader* header)
	:CELLStream((char*)header, header->dataLength)
{

}

CELLSendStream::CELLSendStream(char* pData, int nSize, bool bDelete /*= false*/)
	: CELLStream(pData, nSize, bDelete)
{
	//预先占领消息长度所需空间
	WriteInt32(0);
}

CELLSendStream::CELLSendStream(int nSize /*= 1024*/)
	: CELLStream(nSize)
{
	//预先占领消息长度所需空间
	WriteInt32(0);
}

CELLSendStream::~CELLSendStream()
{

}

void CELLSendStream::finsh()
{
	//得到当前的偏移位置
	int pos = getWritePos();
	//偏移位置设为0
	setWritePos(0);
	WriteInt32(pos);
	setWritePos(pos);
}
