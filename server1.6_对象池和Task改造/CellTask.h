#ifndef _CellTask_H
#define _CellTask_H

#include <list>
#include <mutex>
#include <thread>
#include "ClientSocket.h"

using namespace std;

class CellTask;
typedef std::shared_ptr<CellTask> CellTaskPtr;
//任务类型的基类
class CellTask
{
public:
	CellTask();
	virtual ~CellTask();

	virtual void doTask();
};

////继承任务基类，做具体的任务：发送消息到客户端的任务
//class CellSendMsg2ClientTask : public CellTask
//{
//public:
//	CellSendMsg2ClientTask(ClientSocketPtr pClient, DataHeader* pHeader);
//	~CellSendMsg2ClientTask();
//
//	void doTask();
//
//private:
//	ClientSocketPtr m_pClient;
//	DataHeader* m_pHeader;
//};

#endif
