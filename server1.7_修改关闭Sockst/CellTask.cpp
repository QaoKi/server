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

//CellSendMsg2ClientTask::CellSendMsg2ClientTask(ClientSocketPtr pClient, DataHeader* pHeader)
//{
//	_pClient = pClient;
//	_pHeader = pHeader;
//}
//
//
//CellSendMsg2ClientTask::~CellSendMsg2ClientTask()
//{
//}
//
//void CellSendMsg2ClientTask::doTask()
//{
//	_pClient->SendData(_pHeader);
//	//执行完任务以后，把数据删除，但是客户端不能删除
//	if (_pHeader)
//	{
//		delete _pHeader;
//		_pHeader = NULL;
//	}
//}

