#include "ClientSocket.h"

ClientSocket::ClientSocket(SOCKET sock)
{
	_ClientSocket = sock;
	_nMsgLastPos = 0;
	_nSendLastPos = 0;
	resetDTHeart();
	resetDTSendBuff();
	memset(_chMsgBuf, 0, sizeof(_chMsgBuf));
	memset(_chSendBuf, 0, sizeof(_chSendBuf));
}

ClientSocket::~ClientSocket()
{
	Close();
}

int ClientSocket::GetLastPos()
{
	return _nMsgLastPos;
}

void ClientSocket::SetLastPos(int pos)
{
	_nMsgLastPos = pos;
}
void ClientSocket::SetSocket(SOCKET sock)
{
	_ClientSocket = sock;
}

char* ClientSocket::GetMsgBuf()
{
	return _chMsgBuf;
}

int ClientSocket::SendData(netmsg_DataHeader* header)
{
	//因为不知道客户端是否可以接收，所以不能再直接调用send发送数据
	//先把数据接收到缓冲区，如果缓冲区不够了，返回错误
	int nRet = SOCKET_ERROR;

	//原来等发送缓冲区满了再发送不再有意义，因为只有可写事件过来以后才能发送

	//要发送数据的长度
	int nSendLen = header->dataLength;
	//数据转换一下
	const char* chData = (const char*)header;

	//当要发送的数据长度，小于等于发送缓冲区空余的长度，将数据放到缓冲区
	if (nSendLen <= SEND_BUFF_SIZE - _nSendLastPos)
	{
		memcpy(_chSendBuf + _nSendLastPos, chData, nSendLen);
		_nSendLastPos += nSendLen;
		nRet = nSendLen;
		if (_nSendLastPos == SEND_BUFF_SIZE)
		{
			_nSendBuffFullCount++;
		}
	}
	else
	{
		_nSendBuffFullCount++;
	}
	return nRet;
}

int ClientSocket::SendBuffReal()
{
	int nRet = SOCKET_ERROR;
	//缓冲区有数据
	//当_nSendLastPos == 0 说明缓冲区中没有数据，不算错误，返回0

	if (_nSendLastPos == 0)
	{
		nRet = 0;
	}
	else if (_nSendLastPos > 0 && INVALID_SOCKET != _ClientSocket)
	{
		nRet = send(_ClientSocket, _chSendBuf, _nSendLastPos, 0);
		
		if (nRet > 0)
		{
			//重置缓冲区指针位置
			_nSendLastPos = 0;
			//重置缓冲区满次数
			_nSendBuffFullCount = 0;
			//重置消息发送计时
			resetDTSendBuff();
		}
		else
		{
			//因为send()的返回值可能为0，send()返回0表示错误，所以要处理一下
			nRet = SOCKET_ERROR;
		}
	}
	return nRet;
}

void ClientSocket::resetDTHeart()
{
	_dtHeart = 0;
}

bool ClientSocket::checkHeart(time_t dt)
{
	_dtHeart += dt;
	if (_dtHeart >= CLIENT_HEART_DEAD_TIME)
	{
		printf("checkHeart dead:c = %d,time=%d\n", _ClientSocket, _dtHeart);
		return true;
	}
	return false;
}

void ClientSocket::resetDTSendBuff()
{
	_dtSendBuff = 0;
}

bool ClientSocket::checkSendBuff(time_t dt)
{
	_dtSendBuff += dt;
	if (_dtSendBuff >= CLIENT_SEND_BUFF_TIME)
	{
		printf("checkSendBuff c = %d,time=%d\n,length:%d\n", _ClientSocket, _dtSendBuff,_nSendLastPos);
		//立即将发送缓冲区中的数据发送出去
		SendBuffReal();
		return true;
	}
	return false;
}

void ClientSocket::Close()
{
	if (_ClientSocket != INVALID_SOCKET)
	{

#ifdef _WIN32
		closesocket(_ClientSocket);
#else
		close(_ClientSocket);

#endif
		_ClientSocket = INVALID_SOCKET;
	}
}

SOCKET ClientSocket::GetSocket()
{
	return _ClientSocket;
}
