#ifndef _ClientSocket_H
#define _ClientSocket_H

//为每个客户端创建一个类，每个对象对应一个客户端

#ifdef _WIN32
#include<windows.h>
#else
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <iostream>
#include "MessageHeader.hpp"

using namespace std;

#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240 * 5
#define SEND_BUFF_SIZE RECV_BUFF_SZIE
#endif 

class ClientSocket
{
public:
	ClientSocket(SOCKET sock = INVALID_SOCKET);
	virtual ~ClientSocket();

public:
	SOCKET GetSocket();

	int GetLastPos();

	void SetLastPos(int pos);

	void SetSocket(SOCKET sock);

	char* GetMsgBuf();

	//发送数据
	int SendData(DataHeader* header);

private:

	SOCKET m_ClientSocket;
	//消息缓存区
	char m_chMsgBuf[RECV_BUFF_SZIE];
	//消息缓冲区的数据尾部位置
	int m_nMsgLastPos;

	//发送缓冲区，当发送缓冲区满了再发送
	char m_chSendBuf[SEND_BUFF_SIZE];
	//发送缓冲区的数据尾部位置
	int m_nSendLastPos;
};

#endif // !_Client_Socket_H_
