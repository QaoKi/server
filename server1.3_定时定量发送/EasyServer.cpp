#include "EasyServer.h"

ClientSocket::ClientSocket(SOCKET sock)
{
	m_ClientSocket = sock;
	m_nMsgLastPos = 0;
	m_nSendLastPos = 0;
	memset(m_chMsgBuf, 0, sizeof(m_chMsgBuf));
	memset(m_chSendBuf, 0, sizeof(m_chSendBuf));
}

ClientSocket::~ClientSocket()
{
	m_ClientSocket = INVALID_SOCKET;
	m_nMsgLastPos = 0;
	memset(m_chMsgBuf, 0, sizeof(m_chMsgBuf));
}

int ClientSocket::GetLastPos()
{
	return m_nMsgLastPos;
}

void ClientSocket::SetLastPos(int pos)
{
	m_nMsgLastPos = pos;
}
void ClientSocket::SetSocket(SOCKET sock)
{
	m_ClientSocket = sock;
}

char* ClientSocket::GetMsgBuf()
{
	return m_chMsgBuf;
}

int ClientSocket::SendData(DataHeader* header)
{
	int nRet = SOCKET_ERROR;
	if (!header)
		return nRet;
	
	//当发送缓冲区满了再发送

	//要发送数据的长度
	int nSendLen = header->dataLength;
	//数据转换一下
	const char* chData = (const char*)header;
	 
	while (true)
	{
		//如果要发送的数据长度，大于发送缓冲区空余的长度
		if (nSendLen > SEND_BUFF_SIZE - m_nSendLastPos)
		{
			//计算可以发送的长度
			int nCopyLen = SEND_BUFF_SIZE - m_nSendLastPos;
			memcpy(m_chSendBuf + m_nSendLastPos, chData, nCopyLen);
			//剩余要发送的数据长度和数据
			nSendLen -= nCopyLen;
			chData += nCopyLen;

			nRet = send(m_ClientSocket, m_chSendBuf, sizeof(m_chSendBuf), 0);
			m_nSendLastPos = 0;

			if (nRet = SOCKET_ERROR)
				return nRet;
		}
		else
		{
			memcpy(m_chSendBuf + m_nSendLastPos, chData, nSendLen);
			m_nSendLastPos += nSendLen;
			break;
		}
	}
	return 0;
}

SOCKET ClientSocket::GetSocket()
{
	return m_ClientSocket;
}

CellServer::CellServer(SOCKET sock)
{
	m_sock = sock;
	m_pThread = NULL;
	m_INetEvent = NULL;
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
		if(pClient)
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
		//struct LoginResult loginResult;
		//strcpy(loginResult.data, "登录成功");
		//pClient->SendData(&loginResult);
	}
	break;
	case CMD_LOGOUT:
	{
		struct Logout* logout = (Logout*)header;
		printf("收到客户端<socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName = %s\n", pClient->GetSocket(), logout->dataLength, logout->userName);
		struct LogoutResult logoutResult;
		pClient->SendData(&logoutResult);
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


//void CellServer::SendDataToAll(DataHeader* data)
//{
//	for (auto it = m_clientArray.begin(); it != m_clientArray.end(); it++) {
//		SendData(it->first, data);
//	}
//}



EasyServer::EasyServer()
{
	m_sock = INVALID_SOCKET;
	nRecvMsgCoutn = 0;
	//memset(m_chRecvBuff, 0, sizeof(m_chRecvBuff));
}


EasyServer::~EasyServer()
{
	//Close();
	closesocket(m_sock);
	for (auto pCellServer : m_CellServerArray)
	{
		if (pCellServer)
		{
			delete pCellServer;
			pCellServer = NULL;
		}
	}
	m_CellServerArray.clear();
	m_clientArray.clear();
	
}

void EasyServer::AddClient(ClientSocket* pClient)
{
	if (m_clientArray.find(pClient->GetSocket()) == m_clientArray.end())
	{
		if(pClient)
			m_clientArray[pClient->GetSocket()] = pClient;
	}
}

ClientSocket* EasyServer::GetClient(SOCKET cSock)
{
	auto it = m_clientArray.find(cSock);
	if (it != m_clientArray.end())
	{
		return it->second;
	}

	return NULL;
}

void EasyServer::ClearClient(SOCKET cSock)
{
	auto it = m_clientArray.find(cSock);
	if (it != m_clientArray.end())
	{
		if (it->second)
		{
			delete it->second;
			it->second = NULL;
		}
			
		m_clientArray.erase(it);
	}
}

void EasyServer::InitSocket()
{
#ifdef _WIN32
	//启动Windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	if (INVALID_SOCKET != m_sock)
	{
		printf("<socket=%d>关闭旧连接...\n",(int)m_sock);
		//Close();
		closesocket(m_sock);
	}
	m_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (INVALID_SOCKET == m_sock)
	{
#ifdef _WIN32
		printf("错误，建立socket失败...错误：%d\n", GetLastError());
#else
		perror("建立socket失败:");
#endif
	}
	else {
		printf("建立socket=<%d>成功...\n", (int)m_sock);
	}

	//端口复用
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
}
int EasyServer::Bind(const char* ip, unsigned short port)
{
	sockaddr_in sin = { 0 };
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

#ifdef _WIN32
	if (ip){
		sin.sin_addr.S_un.S_addr = inet_addr(ip);
	}
	else {
		sin.sin_addr.S_un.S_addr = INADDR_ANY;
	}
#else
	if (ip) {
		sin.sin_addr.s_addr = inet_addr(ip);
	}
	else {
		sin.sin_addr.s_addr = INADDR_ANY;
	}

#endif

	//c++11中的bind函数和socket中的bind不一样，如果使用了std的命名空间，默认会使用c++11中的bind
	//所以在bind前面加上::，表示使用socket的bind
	int ret = ::bind(m_sock, (sockaddr*)&sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
#ifdef _WIN32
		printf("错误,绑定网络端口<%d>失败...错误：%d\n", port, GetLastError());
#else
		perror("绑定网络端口失败:");
#endif
	}
	else {
		printf("绑定网络端口<%d>成功...\n", port);
	}
	return 0;
}

int EasyServer::Listen(int n)
{
	int ret = listen(m_sock, n);
	if (SOCKET_ERROR == ret)
	{
#ifdef _WIN32
		printf("socket=<%d>错误,监听网络端口失败...错误：%d\n", m_sock,GetLastError());
#else
		perror("监听网络端口失败:");
#endif
	}
	else {
		printf("socket=<%d>监听网络端口成功...\n",m_sock);
	}
	return ret;

}

void EasyServer::Start(int nCellServerCount)
{
	for (int i = 0; i < nCellServerCount; i++)
	{
		auto pSer = new CellServer(m_sock);
		m_CellServerArray.push_back(pSer);
		pSer->Start();
		pSer->SetEventObj(this);
	}
	Run();
}

bool EasyServer::Run()
{

	//搭建一个select模型
	fd_set fdRead;
	SOCKET sMax = m_sock;

	timeval tTimeout = { 0,0 };

	while (isRun())
	{			
		Time4Msg();
		FD_ZERO(&fdRead);
		FD_SET(m_sock, &fdRead);

		int nReady = select(sMax + 1, &fdRead, NULL, NULL, &tTimeout);

		if (nReady < 0)
		{
			printf("服务器任务结束。\n");
			CloseSock(m_sock);
			return false;
		}
		else if (nReady == 0) {
			chrono::milliseconds t(1);
			this_thread::sleep_for(t);
			continue;
		}

		if (FD_ISSET(m_sock, &fdRead))
		{
			//接受新连接
			Accept();
		}

	}
	return true;
}

bool EasyServer::isRun()
{
	return m_sock != INVALID_SOCKET;
}

void EasyServer::ClientLeaveEvent(ClientSocket* pClient)
{
	CloseSock(pClient->GetSocket());
}

void EasyServer::RecvMsgEvent(ClientSocket* pClient)
{
	nRecvMsgCoutn++;
}

SOCKET EasyServer::Accept()
{
	SOCKET cSock = INVALID_SOCKET;
	sockaddr_in clientAddr = { 0 };
	int nAddrlen = sizeof(sockaddr_in);

#ifdef _WIN32
	cSock = accept(m_sock, (sockaddr*)&clientAddr, &nAddrlen);
#else
	cSock = accept(m_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrlen);
#endif
	if (cSock <= 0) {
#ifdef _WIN32
		printf("socket=<%d>错误,接受到无效客户端SOCKET...错误：%d\n", m_sock, GetLastError());
#else
		perror("接受到无效客户端SOCKET:");
#endif
		return -1;
	}
	else {
		//向其他客户端发送收到新用户登录的消息
		//NewUserJoin newUser = { };
		//newUser.scok = cSock;
		//SendDataToAll(&newUser);

		ClientSocket* pClient = new ClientSocket(cSock);
		AddClient(pClient);
		//将这个客户端，发给CellServer进行数据管理
		AddClientToCellServer(pClient);

		//printf("socket=<%d>收到新客户端加入：socket = %d,IP = %s ,port = %d\n", m_sock, cSock, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		//printf("socket=<%d>收到新客户端加入：socket = %d,客户端数量：%d\n", m_sock, cSock, m_clientArray.size());
	}
	return cSock;
}

void EasyServer::AddClientToCellServer(ClientSocket* pClient)
{
	//发给客户数量最少的CellServer
	//如果都一样，保存到第一个
	auto pMinNumClient = m_CellServerArray[0];
	for (size_t i = 1; i < m_CellServerArray.size(); i++)
	{
		pMinNumClient = pMinNumClient->GetClientNum() > m_CellServerArray[i]->GetClientNum() ? m_CellServerArray[i] : pMinNumClient;
	}

	pMinNumClient->AddClientToBuff(pClient);
}

void EasyServer::Time4Msg()
{
	auto t1 = m_tTime.getElapsedSecond();
	//记录一秒钟内，收到了多少条消息
	if (t1 > 1.0)
	{
		//将时间更新为当前时间，计数更新为0
		m_tTime.update();
		printf("socket=<%d>,时间：%lf,客户端数量：%d,消息数量：%d\n", m_sock, t1, m_clientArray.size(),CellServer::m_recvMsgCount);
		CellServer::m_recvMsgCount = 0;
	}

}

//void EasyServer::Close()
//{
//	for (auto it = m_clientArray.begin(); it != m_clientArray.end(); it++) {
//		CloseSock(it->first);
//	}
//	CloseSock(m_sock);
//	m_clientArray.clear();
//#ifdef _WIN32
//	//清除Windows socket环境
//	WSACleanup();
//#endif
//
//}

void EasyServer::CloseSock(SOCKET sock)
{
	if (sock != INVALID_SOCKET)
	{
		ClearClient(sock);
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		sock = INVALID_SOCKET;
	}
}

