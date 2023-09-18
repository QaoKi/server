#include "CELLMsgStream.h"


CELLRecvStream::CELLRecvStream(netmsg_DataHeader* header)
	: CELLStream((char*)header, header->dataLength)
{
	push(header->dataLength);
}

uint16_t CELLRecvStream::getNetMsgCmd()
{
	uint16_t cmd = CMD_ERROR;
	Read<uint16_t>(cmd);
	return cmd;
}


CELLSendStream::CELLSendStream(char* pData, int nSize, bool bDelete /*= false*/)
	: CELLStream(pData, nSize, bDelete)
{

}

CELLSendStream::CELLSendStream(int nSize /*= 1024*/)
	: CELLStream(nSize)
{

}

void CELLSendStream::setNetMsgCmd(uint16_t cmd)
{
	Write(cmd);
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
