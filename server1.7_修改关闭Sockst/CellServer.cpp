#include "CellServer.h"

CellServer::CellServer(int id)
{
	_id = id;
	_isRun = false;
	_pThread = NULL;
	_INetEvent = NULL;
	_TaskServerPtr = make_shared<CellTaskServer>();
	memset(_chRecvBuff, 0, sizeof(_chRecvBuff));
	_oldTime = CELLTime::getNowTimeInMilliSec();;
}

//静态成员变量，在类外分配内存空间
int CellServer::_recvMsgCount = 0;

CellServer::~CellServer()
{
	if (_pThread)
	{
		delete _pThread;
		_pThread = NULL;
	}

	Close();
}
void CellServer::AddClientToBuff(ClientSocketPtr& pClient)
{
	//自解锁，当走完这段程序，锁会自动解锁
	//lock_guard<mutex> lock(_clientBuffMutex);

	_clientBuffMutex.lock();
	_clientBuff.push_back(pClient);
	_clientBuffMutex.unlock();
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
	//&CellServer::Run   成员函数的函数指针
	//mem_fun()	更安全的转化
	if (!_isRun)
	{
		_isRun = true;
		_pThread = new thread(mem_fun(&CellServer::Run), this);
		_pThread->detach();
		_TaskServerPtr->Start();
	}

}

//得到这个处理数据的服务器有多少个客户端连接
int CellServer::GetClientNum()
{
	//已经进行管理的客户端，和主线程已经发过来还没开始管理的客户端
	return _clientArray.size() + _clientBuff.size();
}

bool CellServer::Run()
{
	while (_isRun)
	{
		if (_clientBuff.size() > 0)
		{
			_clientBuffMutex.lock();
			for (auto pClient : _clientBuff)
			{
				AddClient(pClient);
			}
			_clientBuff.clear();
			_clientBuffMutex.unlock();
		}

		//搭建一个select模型
		fd_set fdRead;
		SOCKET sMax = 0;

		FD_ZERO(&fdRead);

		for (auto it = _clientArray.begin(); it != _clientArray.end();) {
			//在_clientExit中没有找到，说明这个客户端还没有退出
			auto it1 = _clientExit.find(it->first);
			if (it1 == _clientExit.end())
			{
				sMax = sMax > it->second->GetSocket() ? sMax : it->second->GetSocket();
				FD_SET(it->first, &fdRead);
				it++;
			}
			else
			{
				//这个客户端已经退出
				it = ClearClient(it->first);
				_clientExit.erase(it1);
			}
		}

		if (_clientArray.size() == 0)
		{
			//更新一下心跳计时时间
			_oldTime = CELLTime::getNowTimeInMilliSec();
			//用c++的休眠，不用windows的，不然跨平台需要修改
			chrono::milliseconds t(1);	//1毫秒
			this_thread::sleep_for(t);
			continue;
		}

		timeval tTimeout = { 0,0 };
		int nReady = select(sMax + 1, &fdRead, NULL, NULL, &tTimeout);

		if (nReady < 0)
		{
			printf("select任务结束。\n");
			Close();
			return false;
		}
		else if (nReady == 0) {
			checkHeart();
			chrono::milliseconds t(1);	//1毫秒
			this_thread::sleep_for(t);
			continue;
		}

		dealMsg(fdRead);
	}

	_sem.wakeup();
	return false;

}

void CellServer::SetEventObj(INetEvent* event)
{
	_INetEvent = event;
}

void CellServer::Close()
{
	if (_isRun)
	{
		_isRun = false;
		_TaskServerPtr->Close();
		//阻塞等待子线程结束
		_sem.wait();
		_clientArray.clear();
		_clientBuff.clear();
		_clientExit.clear();
	}

}

void CellServer::dealMsg(fd_set& fdRead)
{
	auto nowTime = CELLTime::getNowTimeInMilliSec();
	auto dt = nowTime - _oldTime;
	_oldTime = nowTime;

	for (auto it = _clientArray.begin(); it != _clientArray.end(); it++)
	{
		ClientSocketPtr client = GetClient(it->first);

		//定时发送消息检测
		client->checkSendBuff(dt);

		//如果收到客户端发过来的数据
		if (FD_ISSET(it->first, &fdRead))
		{
			//更新心跳监测时间，不管发过来的是否是断开连接的消息
			client->resetDTHeart();
			//接受数据
			RecvData(client);
		}
		else
		{
			//心跳检测
			//客户端是否已经到达死亡时间
			if (client->checkHeart(dt))
			{
				printf("客户端<socket=%d>已退出，任务结束\n", client->GetSocket());
				_INetEvent->ClientLeaveEvent(client);
				_clientExit.insert(client->GetSocket());
			}
		}
	}
}

int CellServer::RecvData(ClientSocketPtr& client)
{
	if (!client)
		return -1;

	int nLen = recv(client->GetSocket(), _chRecvBuff, sizeof(_chRecvBuff), 0);
	DataHeader* header = (DataHeader*)_chRecvBuff;
	if (nLen <= 0)
	{
		printf("客户端<socket=%d>已退出，任务结束\n", client->GetSocket());
		_INetEvent->ClientLeaveEvent(client);
		_clientExit.insert(client->GetSocket());
		return -1;
	}

	//将收取到的数据拷贝到消息缓冲区的尾部,接收到多少拷贝过去多少
	memcpy(client->GetMsgBuf() + client->GetLastPos(), _chRecvBuff, nLen);
	//消息缓冲区的数据尾部位置后移
	client->SetLastPos(client->GetLastPos() + nLen);

	//只要消息缓冲区中的数据，长度大于消息头DataHeader长度
	while (client->GetLastPos() >= sizeof(DataHeader))
	{
		//这时就可以知道当前消息的长度
		DataHeader* header = (DataHeader*)client->GetMsgBuf();
		//消息缓冲区中的数据长度，大于等于当前数据的长度
		if (client->GetLastPos() >= header->dataLength)
		{
			//因为重新memcpy之后，header的数据就变了，相应的header->dataLength也就变了，当移动nLastPos的时候会出错
			//所以先记录下来
			//消息区还未处理的消息长度
			int size = client->GetLastPos() - header->dataLength;
			//处理消息
			OnNetMsg(client, header);
			//从消息缓冲区中去除这条消息（把这条消息之后的数据移到缓冲区头部）
			memcpy(client->GetMsgBuf(), client->GetMsgBuf() + header->dataLength, size);
			//移动消息缓冲区尾部位置
			client->SetLastPos(size);
		}
		else
		{
			//消息缓冲区剩余数据不够一条完整消息
			break;
		}
	}
	return 0;
}

void CellServer::OnNetMsg(ClientSocketPtr& pClient, DataHeader* header)
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
		//printf("收到客户端<socket=%d>请求：CMD_LOGIN,数据长度：%d,userName = %s,passWord = %s\n", pClient->GetSocket(), login->dataLength, login->userName, login->PassWord);
		LoginResult* loginResult = new LoginResult;
		strcpy(loginResult->data, "登录成功");
		addSendTask(pClient, (DataHeader*)loginResult);
	}
	break;
	case CMD_HEART_C2S:
	{
		struct Heart_C2S* login = (Heart_C2S*)header;
		//printf("收到客户端<socket=%d>请求：CMD_HEART,数据长度：%d\n", pClient->GetSocket(), Heart_C2S->dataLength);
		pClient->resetDTHeart(); 
		//Heart_S2C* heart = new Heart_S2C;
		//addSendTask(pClient, (DataHeader*)heart);
	}
	break;
	case CMD_LOGOUT:
	{
		struct Logout* logout = (Logout*)header;
		printf("收到客户端<socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName = %s\n", pClient->GetSocket(), logout->dataLength, logout->userName);
		struct LogoutResult logoutResult;
	}
	break;
	case CMD_EXIT:
	{
		struct ExitConnect* cmd_exit = (ExitConnect*)header;
		printf("收到客户端<socket=%d>请求：CMD_EXIT,数据长度：%d\n", pClient->GetSocket(), cmd_exit->dataLength);
	}

	default:
		break;
	}

}

void CellServer::addSendTask(ClientSocketPtr pClient, DataHeader* header)
{
	//auto task = (CellTask*)std::make_shared<CellSendMsg2ClientTask>(pClient, header);
	//创建一个lambda表达式传进去
	/*
		原来的方式是创建一个task的基类，具体的任务类，比如发送数据任务，继承
		这个基类，重写基类的doTask()方法，把任务实现
		现在直接用lambda表达式，把发送数据类的doTask()函数体放到lambda表达式中
	*/
	auto task = [pClient, header]() {

		pClient->SendData(header);
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
			printf("客户端<socket=%d>已退出，任务结束\n", it->second->GetSocket());
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