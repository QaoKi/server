#include "CellServer.h"

CellServer::CellServer(int id)
{
	_id = id;
	_INetEvent = NULL;
	_TaskServerPtr = make_shared<CellTaskServer>();
	_oldTime = CELLTime::getNowTimeInMilliSec();;
}

//静态成员变量，在类外分配内存空间
int CellServer::_recvMsgCount = 0;

CellServer::~CellServer()
{
	CELLLog::Info("CellServer%d.Close() begin\n",_id);
	Close();
	CELLLog::Info("CellServer%d.Close() end\n", _id);
}
void CellServer::AddClientToBuff(ClientSocketPtr& pClient)
{
	//自解锁，当走完这段程序，锁会自动解锁
	lock_guard<mutex> lock(_clientBuffMutex);
	_clientBuff.push_back(pClient);
}

void CellServer::AddClient(ClientSocketPtr& pClient)
{
	if (_clientArray.find(pClient->GetSocket()) == _clientArray.end())
	{
		_clientArray[pClient->GetSocket()] = pClient;
	}
}

ClientSocketPtr CellServer::GetClient(SOCKET cSock)
{
	auto it = _clientArray.find(cSock);
	if (it != _clientArray.end())
	{
		return it->second;
	}

	return NULL;
}

auto CellServer::ClearClient(SOCKET cSock)
{
	auto it = _clientArray.find(cSock);
	if (it != _clientArray.end())
	{
		return _clientArray.erase(it);
	}
	return _clientArray.end();
}

void CellServer::Start()
{
	_thread.Start(
		//onCreate
		nullptr,
		//onRun
		[this](CELLThread* pThread) {
		Run(pThread);
	}, 
		//onClose
		[this](CELLThread* pThread) {
		//当调用CellServer的Close(),工作线程会退出以后，将客户端清理
		ClearAllClient();
	});

	_TaskServerPtr->Start();
}

//得到这个处理数据的服务器有多少个客户端连接
int CellServer::GetClientNum()
{
	//已经进行管理的客户端，和主线程已经发过来还没开始管理的客户端
	return _clientArray.size() + _clientBuff.size();
}

bool CellServer::Run(CELLThread * pThread)
{
	_RecvCallBack = std::bind(&CellServer::RecvData, this, placeholders::_1, placeholders::_2);
	_SendCallBack = std::bind(&CellServer::SendData, this, placeholders::_1, placeholders::_2);

	while (pThread->isRun())
	{
		if (_clientBuff.size() > 0)
		{
			lock_guard<std::mutex> lock(_clientBuffMutex);
			for (auto pClient : _clientBuff)
			{
				AddClient(pClient);
				_myevent.eventAdd(pClient->GetSocket(), this);
				_myevent.eventSet(pClient->GetSocket(), EPOLLIN, _RecvCallBack);
			}
			_clientBuff.clear();
		}

		for (auto client : _clientExit)
		{
			//_clientExit中的元素都是已经退出的客户端，从_clientArray也清除

			if (_clientArray.find(client) != _clientArray.end())
			{
				//这个客户端已经退出
				_myevent.eventDel(client);
				ClearClient(client);
			}
		}
		_clientExit.clear();

		if (_clientArray.size() == 0)
		{
			//更新一下心跳计时时间
			_oldTime = CELLTime::getNowTimeInMilliSec();
			CELLThread::Sleep(1);
			continue;
		}

		int nReady = _myevent.eventStart();

		if (nReady < 0)
		{
			CELLLog::Info("CellServer.Run epoll exit\n");
			pThread->Exit();
			break;
		}
		else if (nReady == 0) {
			CELLThread::Sleep(1);
		}
		//检查心跳
		CELLThread::Sleep(100);
		checkHeart();
	}

	CELLLog::Info("CellServer%d.Run()  exit\n", _id);
	return false;

}

void CellServer::SetEventObj(INetEvent* event)
{
	_INetEvent = event;
}

void CellServer::Close()
{
	_thread.Close();
	_TaskServerPtr->Close();
}

void CellServer::ClearAllClient()
{
	_clientArray.clear();
	_clientBuff.clear();
	_clientExit.clear();
}

int CellServer::RecvData(int sock, int events)
{
	ClientSocketPtr pClient = _clientArray[sock];
	pClient->resetDTHeart();
	if (!pClient)
		return -1;

	//接收数据到客户端的接收缓冲区
	int nLen = pClient->RecvData();
	//出错则客户端退出
	if (nLen == SOCKET_ERROR)
	{
		CELLLog::Info("client <socket=%d> heart dead\n", pClient->GetSocket());
		_INetEvent->ClientLeaveEvent(pClient);
		_clientExit.insert(pClient->GetSocket());
		return -1;
	}

	//当客户端接收缓冲区中的数据，长度大于等于一条完整的消息时
	//循环处理消息
	while (pClient->hasMsg())
	{
		//处理消息
		OnNetMsg(pClient, pClient->front_msg());
		//消息处理完成以后，移除缓冲区最前的一条消息
		pClient->pop_front_msg();
	}

	//开始监听写事件
	_myevent.eventSet(pClient->GetSocket(), EPOLLOUT,_SendCallBack);
	return 0;
}

int CellServer::SendData(int sock, int events)
{
	//当这个客户端可写的时候，直接把缓冲区的数据发送出去
	/*需要注意的是，因为是select模型，比如一个客户端可读和可写事件都来了，会先处理可读事件，
	数据经过处理之后，创建一个要回复的消息放到发送缓冲区，再去处理可写事件，将缓冲区的数据发出去，
	因为处理可读事件和可写事件不是并行的，所以客户端的发送缓冲区是线程安全的
	*/

	ClientSocketPtr pClient = _clientArray[sock];
	pClient->resetDTHeart();
	//直接将客户端发送缓冲区中的数据发给客户端
	if(pClient->SendBuffReal() == SOCKET_ERROR)
	{
		//发送失败，退出这个客户端
		CELLLog::Info("client <socket=%d> SendData error\n", pClient->GetSocket());
		_INetEvent->ClientLeaveEvent(pClient);
		_clientExit.insert(pClient->GetSocket());
		return -1;
	}

	//开始监听读事件
	_myevent.eventSet(pClient->GetSocket(), EPOLLIN, _RecvCallBack);

	return 0;
}

void CellServer::OnNetMsg(ClientSocketPtr& pClient, netmsg_DataHeader* header)
{
	CellServer::_recvMsgCount++;	//接收到的消息数量+1
	//auto t1 = _tTime.getElapsedSecond();
	////记录一秒钟内，收到了多少条消息
	//if (t1 > 1.0)
	//{
	//	//将时间更新为当前时间，计数更新为0
	//	_tTime.update();
	//	_recvMsgCount = 0;
	//}
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		//struct Login* login = (Login*)header;
		//CELLLog::Info("收到客户端<socket=%d>请求：CMD_LOGIN,数据长度：%d,userName = %s,passWord = %s\n", pClient->GetSocket(), login->dataLength, login->userName, login->PassWord);
		LoginResult* loginResult = new LoginResult;
		strcpy(loginResult->data, "login succ");
		addSendTask(pClient, (netmsg_DataHeader*)loginResult);
	}
	break;
	case CMD_HEART_C2S:
	{
		struct Heart_C2S* login = (Heart_C2S*)header;
		//CELLLog::Info("收到客户端<socket=%d>请求：CMD_HEART,数据长度：%d\n", pClient->GetSocket(), Heart_C2S->dataLength);
		pClient->resetDTHeart(); 
		//Heart_S2C* heart = new Heart_S2C;
		//addSendTask(pClient, (DataHeader*)heart);
	}
	break;
	case CMD_LOGOUT:
	{
		LogoutResult* logoutResult = new LogoutResult;
		strcpy(logoutResult->data, "logout succ");
		addSendTask(pClient, (netmsg_DataHeader*)logoutResult);
	}
	break;
	case CMD_EXIT:
	{
		struct ExitConnect* cmd_exit = (ExitConnect*)header;
		CELLLog::Info("recv client <socket=%d>request:CMD_EXIT,data length:%d\n", pClient->GetSocket(), cmd_exit->dataLength);
	}

	default:
		break;
	}

}

void CellServer::addSendTask(ClientSocketPtr pClient, netmsg_DataHeader* header)
{
	//auto task = (CellTask*)std::make_shared<CellSendMsg2ClientTask>(pClient, header);
	//创建一个lambda表达式传进去
	/*
		原来的方式是创建一个task的基类，具体的任务类，比如发送数据任务，继承
		这个基类，重写基类的doTask()方法，把任务实现
		现在直接用lambda表达式，把发送数据类的doTask()函数体放到lambda表达式中
	*/
	auto task = [pClient, header]() {

		//如果缓冲区满了

		if (pClient->SendData(header) < 0)
		{
			CELLLog::Info("<Socket = %d> SendDataBuff full\n", pClient->GetSocket());
			//可以添加别的功能，比如数据放到数据库或者写入本地文件
		}
		//执行完任务以后，把数据删除，但是客户端不能删除
		if (header)
		{
			delete header;
		}
	};
	_TaskServerPtr->AddTask(task);
}

void CellServer::checkHeart()
{
	auto nowTime = CELLTime::getNowTimeInMilliSec();
	auto dt = nowTime - _oldTime;
	_oldTime = nowTime;

	for (auto it = _clientArray.begin(); it != _clientArray.end(); it++)
	{
		if (it->second->checkHeart(dt))
		{
			CELLLog::Info("client <socket=%d> heart dead\n", it->second->GetSocket());
			_INetEvent->ClientLeaveEvent(it->second);
			_clientExit.insert(it->first);
		}
	}
}


//void CellServer::SendDataToAll(DataHeader* data)
//{
//	for (auto it = _clientArray.begin(); it != _clientArray.end(); it++) {
//		SendData(it->first, data);
//	}
//}