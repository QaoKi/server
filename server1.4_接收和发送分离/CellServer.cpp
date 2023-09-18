#include "CellServer.h"

CellServer::CellServer(SOCKET sock)
{
	m_sock = sock;
	m_pThread = NULL;
	m_INetEvent = NULL;
	m_TskServer = new CellTaskServer();
	memset(m_chRecvBuff, 0, sizeof(m_chRecvBuff));
}

//静态成员变量，在类外分配内存空间
int CellServer::m_recvMsgCount = 0;

CellServer::~CellServer()
{
	if (m_pThread)
	{
		delete m_pThread;
		m_pThread = NULL;
	}
	if (m_TskServer)
	{
		delete m_TskServer;
		m_TskServer = NULL;
	}
	m_INetEvent = NULL;
	memset(m_chRecvBuff, 0, sizeof(m_chRecvBuff));
	m_clientArray.clear();
	m_clientBuff.clear();
}
void CellServer::AddClientToBuff(ClientSocket* pClient)
{
	//自解锁，当走完这段程序，锁会自动解锁
	//lock_guard<mutex> lock(m_clientBuffMutex);

	m_clientBuffMutex.lock();
	m_clientBuff.push_back(pClient);
	m_clientBuffMutex.unlock();
}

void CellServer::AddClient(ClientSocket* pClient)
{
	if (m_clientArray.find(pClient->GetSocket()) == m_clientArray.end())
	{
		if (pClient)
			m_clientArray[pClient->GetSocket()] = pClient;
	}
}

ClientSocket* CellServer::GetClient(SOCKET cSock)
{
	auto it = m_clientArray.find(cSock);
	if (it != m_clientArray.end())
	{
		return it->second;
	}

	return NULL;
}

auto CellServer::ClearClient(SOCKET cSock)
{
	auto it = m_clientArray.find(cSock);
	if (it != m_clientArray.end())
	{
		return m_clientArray.erase(it);
	}
	return m_clientArray.end();
}

void CellServer::Start()
{
	//&CellServer::Run   成员函数的函数指针
	//mem_fun()	更安全的转化
	m_pThread = new thread(mem_fun(&CellServer::Run), this);
	m_pThread->detach();
	m_TskServer->Start();
}

//得到这个处理数据的服务器有多少个客户端连接
int CellServer::GetClientNum()
{
	//已经进行管理的客户端，和主线程已经发过来还没开始管理的客户端
	return m_clientArray.size() + m_clientBuff.size();
}

bool CellServer::Run()
{
	while (isRun())
	{
		if (m_clientBuff.size() > 0)
		{
			m_clientBuffMutex.lock();
			for (auto pClient : m_clientBuff)
			{
				AddClient(pClient);
			}
			m_clientBuff.clear();
			m_clientBuffMutex.unlock();
		}

		//搭建一个select模型
		fd_set fdRead;
		SOCKET sMax = 0;

		FD_ZERO(&fdRead);

		for (auto it = m_clientArray.begin(); it != m_clientArray.end();) {
			//在m_clientExit中没有找到，说明这个客户端还没有退出
			auto it1 = m_clientExit.find(it->first);
			if (it1 == m_clientExit.end())
			{
				sMax = sMax > it->second->GetSocket() ? sMax : it->second->GetSocket();
				FD_SET(it->first, &fdRead);
				it++;
			}
			else
			{
				//这个客户端已经退出
				it = ClearClient(it->first);
				m_clientExit.erase(it1);
			}
		}

		if (m_clientArray.size() == 0)
		{
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
			chrono::milliseconds t(1);	//1毫秒
			this_thread::sleep_for(t);
			continue;
		}

		for (auto it = m_clientArray.begin(); it != m_clientArray.end(); it++) {
			if (FD_ISSET(it->first, &fdRead))
			{
				//接受数据
				RecvData(it->first);
				if (--nReady == 0)
					break;
			}
		}

	}

	return false;

}

bool CellServer::isRun()
{
	return m_sock != INVALID_SOCKET;
}

void CellServer::SetEventObj(INetEvent* event)
{
	m_INetEvent = event;
}

void CellServer::Close()
{
	for (auto it = m_clientArray.begin(); it != m_clientArray.end(); it++) {
		CloseSock(it->first);
	}
	m_clientArray.clear();
#ifdef _WIN32
	//清除Windows socket环境
	WSACleanup();
#endif

}

void CellServer::CloseSock(SOCKET sock)
{
	if (sock != INVALID_SOCKET)
	{
		ClearClient(sock);

#ifdef _WIN32
		closesocket(sock);
		sock = INVALID_SOCKET;
#else
		close(sock);
		sock = INVALID_SOCKET;
#endif
	}
}

int CellServer::RecvData(SOCKET cSock)
{
	ClientSocket* client = GetClient(cSock);
	if (!client)
		return -1;

	int nLen = recv(cSock, m_chRecvBuff, sizeof(m_chRecvBuff), 0);
	DataHeader* header = (DataHeader*)m_chRecvBuff;
	if (nLen <= 0)
	{
		printf("客户端<socket=%d>已退出，任务结束\n", cSock);
		m_INetEvent->ClientLeaveEvent(client);
		m_clientExit.insert(cSock);
		return -1;
	}

	//将收取到的数据拷贝到消息缓冲区的尾部,接收到多少拷贝过去多少
	memcpy(client->GetMsgBuf() + client->GetLastPos(), m_chRecvBuff, nLen);
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

void CellServer::OnNetMsg(ClientSocket* pClient, DataHeader* header)
{
	CellServer::m_recvMsgCount++;	//接收到的消息数量+1
	//auto t1 = m_tTime.getElapsedSecond();
	////记录一秒钟内，收到了多少条消息
	//if (t1 > 1.0)
	//{
	//	//将时间更新为当前时间，计数更新为0
	//	m_tTime.update();
	//	m_recvMsgCount = 0;
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

void CellServer::addSendTask(ClientSocket* pClient, DataHeader* header)
{
	CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
	m_TskServer->AddTask(task);
}

//void CellServer::SendDataToAll(DataHeader* data)
//{
//	for (auto it = m_clientArray.begin(); it != m_clientArray.end(); it++) {
//		SendData(it->first, data);
//	}
//}