#include "ClientSocket.h"

ClientSocket::ClientSocket(SOCKET sock)
			:_sendBuff(SEND_BUFF_SIZE),_recvBuff(RECV_BUFF_SZIE)
{
	_ClientSocket = sock;
	resetDTHeart();
	resetDTSendBuff();
}

ClientSocket::~ClientSocket()
{
	Close();
}

void ClientSocket::SetSocket(SOCKET sock)
{
	_ClientSocket = sock;
}

int ClientSocket::SendData(netmsg_DataHeader* header)
{
	//因为不知道客户端是否可以接收，所以不能再直接调用send发送数据
	//先把数据接收到缓冲区，如果缓冲区不够了，返回错误

	//原来等发送缓冲区满了再发送不再有意义，因为只有可写事件过来以后才能发送

	//要发送数据的长度
	int nSendLen = header->dataLength;
	//数据转换一下
	const char* chData = (const char*)header;

	if (_sendBuff.push(chData, nSendLen))
	{
		return nSendLen;
	}
	return SOCKET_ERROR;
}

int ClientSocket::SendBuffReal()
{
	//定时发送消息，计时重置
	resetDTSendBuff();
	return _sendBuff.write2socket(_ClientSocket);
}

int ClientSocket::RecvData()
{
	return _recvBuff.read4socket(_ClientSocket);
}

bool ClientSocket::hasMsg()
{
	return _recvBuff.hasMag();
}

netmsg_DataHeader* ClientSocket::front_msg()
{
	return (netmsg_DataHeader*)_recvBuff.getData();
}

void ClientSocket::pop_front_msg()
{
	if (hasMsg())
		_recvBuff.pop(front_msg()->dataLength);
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
		CELLLog::Info("checkHeart dead:c = %d,time=%d\n", _ClientSocket, _dtHeart);
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
