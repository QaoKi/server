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
#include "CELLObjectPool.hpp"

using namespace std;

#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#define SEND_BUFF_SIZE RECV_BUFF_SZIE
#endif 

//监测心跳死亡计时时间
#define  CLIENT_HEART_DEAD_TIME 60000
//指定的时间内，把缓冲区中数据发出去
#define  CLIENT_SEND_BUFF_TIME 20

class ClientSocket;
typedef std::shared_ptr<ClientSocket> ClientSocketPtr;

class ClientSocket : public ObjectPoolBase<ClientSocket,1000>
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

	//发送数据,当发送缓冲区满了以后
	int SendData(DataHeader* header);

	//将发送缓冲区中的数据发送给客户端
	int SendBuffReal();

	//当服务器收到这个客户端发过来的心跳之后，重置计时时间
	void resetDTHeart();
	//监测心跳
	bool checkHeart(time_t dt);

	//重置发送消息时间
	void resetDTSendBuff();
	//检测时间，定时发送消息
	bool checkSendBuff(time_t dt);

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
	//心跳死亡计时
	time_t m_dtHeart;
	//上次发送消息数据的时间
	time_t m_dtSendBuff;
};

#endif // !_Client_Socket_H_
