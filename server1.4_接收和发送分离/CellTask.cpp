#include "CellTask.h"



CellTask::CellTask()
{
}


CellTask::~CellTask()
{
}

void CellTask::doTask()
{

}

CellSendMsg2ClientTask::CellSendMsg2ClientTask(ClientSocket* pClient, DataHeader* pHeader)
{
	m_pClient = pClient;
	m_pHeader = pHeader;
}


CellSendMsg2ClientTask::~CellSendMsg2ClientTask()
{
}

void CellSendMsg2ClientTask::doTask()
{
	m_pClient->SendData(m_pHeader);
	//执行完任务以后，把数据删除，但是客户端不能删除
	if (m_pHeader)
	{
		delete m_pHeader;
		m_pHeader = NULL;
	}
}

