#ifndef _INetEvent_H
#define _INetEvent_H
#include "ClientSocket.h"

//定义一个事件的抽象类
class INetEvent
{
public:
	//纯虚函数
	//客户端离开事件
	virtual void ClientLeaveEvent(ClientSocket* pClient) = 0;
	//之前用的是静态变量，记录收到了多少条消息，现在用事件的形式
	virtual void RecvMsgEvent(ClientSocket* pClient) = 0;
};

#endif