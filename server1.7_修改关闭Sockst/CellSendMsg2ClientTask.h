#ifndef _CellSendMsg2ClientTask_H
#define _CellSendMsg2ClientTask_H
#include "CellTask.h"
#include "ClientSocket.h"
#include "MessageHeader.hpp"

//继承任务基类，做具体的任务：发送消息到客户端的任务
class CellSendMsg2ClientTask : public CellTask
{
public:
	CellSendMsg2ClientTask(ClientSocketPtr& pClient, DataHeader* pHeader);
	~CellSendMsg2ClientTask();

	void doTask();

private:
	ClientSocketPtr _pClient;
	DataHeader* _pHeader;
};

#endif 

