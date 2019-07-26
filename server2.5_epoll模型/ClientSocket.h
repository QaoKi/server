#ifndef _ClientSocket_H
#define _ClientSocket_H

//为每个客户端创建一个类，每个对象对应一个客户端

#include <stdio.h>
#include <iostream>
#include <memory>
#include <memory.h>
#include "CELL.h"
#include "MessageHeader.hpp"
#include "CELLObjectPool.hpp"
#include "CELLBuffer.h"
#include "CELLLog.h"

using namespace std;


//监测心跳死亡计时时间
#define  CLIENT_HEART_DEAD_TIME 60000
//指定的时间内，把缓冲区中数据发出去
#define  CLIENT_SEND_BUFF_TIME 200

class ClientSocket;
typedef std::shared_ptr<ClientSocket> ClientSocketPtr;

class ClientSocket : public ObjectPoolBase<ClientSocket,1000>
{
public:
	ClientSocket(SOCKET sock = INVALID_SOCKET);
	virtual ~ClientSocket();

public:
	SOCKET GetSocket();

	void SetSocket(SOCKET sock);

	//将数据放入发送缓冲区
	int SendData(netmsg_DataHeader* header);

	//将发送缓冲区中的数据发送给客户端
	int SendBuffReal();

	//因为ClientSocket内部声明的缓冲区，所以对于外部调用者，还需要再封装一个方法
	//接收数据到接收缓冲区，对接收缓冲区的read4socket()的再封装
	int RecvData();

	//是否有消息可以处理，对接收缓冲区的hasMsg()的再封装
	bool hasMsg();

	//返回接收缓冲区的第一条完整消息
	netmsg_DataHeader* front_msg();
	//移除缓冲区中的第一条完整消息
	void pop_front_msg();

	//当服务器收到这个客户端发过来的心跳之后，重置计时时间
	void resetDTHeart();
	//监测心跳
	bool checkHeart(time_t dt);

	//重置定时发送消息的时间
	void resetDTSendBuff();
	//检测时间，定时发送消息
	bool checkSendBuff(time_t dt);

private:

	//客户端退出
	void Close();

	SOCKET _ClientSocket;
	//接收消息缓冲区
	CELLBuffer _recvBuff = { RECV_BUFF_SZIE };

	//发送缓冲区,在栈上分配的成员变量，有两种初始化方式，构造函数初始化列表和中括号
	CELLBuffer _sendBuff = { SEND_BUFF_SIZE };
	//心跳死亡计时
	time_t _dtHeart;
	//上次发送消息数据的时间
	time_t _dtSendBuff;

	//发送缓冲区遇到写满的情况次数，当每次把缓冲区的数据发出去以后清零
	//当这个值大于0，说明之前有业务使缓冲区满了，那么当客户端可写的时候，直接发过去
	int _nSendBuffFullCount = 0;
};

#endif // !_Client_Socket_H_
