#include "EasyServer.h"

ClientSocket::ClientSocket(SOCKET m_ClientSocket)
{
	m_ClientSocket = INVALID_SOCKET;
	memset(m_chMsgBuf, 0, sizeof(m_chMsgBuf));
}

int ClientSocket::GetLastPos()
{
	return m_nLastPos;
}

void ClientSocket::SetLastPos(int pos)
{
	m_nLastPos = pos;
}

char* ClientSocket::GetMsgBuf()
{
	return m_chMsgBuf;
}

ClientSocket::~ClientSocket()
{
	
}

SOCKET ClientSocket::GetSocket()
{
	return m_ClientSocket;
}


EasyServer::EasyServer()
{
	m_sock = INVALID_SOCKET;
	m_recvCount = 0;
	memset(m_chRecvBuff, 0, sizeof(m_chRecvBuff));
}


EasyServer::~EasyServer()
{
	Close();
}

void EasyServer::AddClient(SOCKET cSock)
{
	if (m_clientFd.find(cSock) == m_clientFd.end())
	{
		ClientSocket* client = new ClientSocket(cSock);
		if (client)
			m_clientFd[cSock] = client;
	}
}

ClientSocket* EasyServer::GetClient(SOCKET cSock)
{
	auto it = m_clientFd.find(cSock);
	if (it != m_clientFd.end())
	{
		return it->second;
	}

	return NULL;
}

void EasyServer::ClearClient(SOCKET cSock)
{
	auto it = m_clientFd.find(cSock);
	if (it != m_clientFd.end())
	{
		if (it->second)
			delete it->second;
		m_clientFd.erase(it);
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
		Close();
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

	int ret = bind(m_sock, (sockaddr*)&sin, sizeof(sockaddr_in));
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
	return ret;
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

bool EasyServer::Run()
{
	if (isRun())
	{
		//搭建一个select模型
		fd_set fdRead;
		SOCKET sMax = m_sock;

		//while (true)
		//{
			FD_ZERO(&fdRead);
			FD_SET(m_sock, &fdRead);
			
			for (auto it = m_clientFd.begin(); it != m_clientFd.begin();it++) {
				FD_SET(it->first, &fdRead);
			}
			timeval tTimeout = { 0,0 };

			int nReady = select(sMax + 1, &fdRead, NULL, NULL, &tTimeout);

			if (nReady < 0)
			{
				printf("服务器任务结束。\n");
				Close();
				return false;
			}
			else if (nReady == 0) {
				return false;
				//continue;
			}

			if (FD_ISSET(m_sock, &fdRead))
			{
				//接受新连接
				SOCKET cSock = Accept();
				if (cSock <= 0)
					return false;//continue;
				if (cSock > sMax)
					sMax = cSock;

				if (--nReady == 0)
					return true; //continue;
			}

			for (auto it = m_clientFd.begin(); it != m_clientFd.begin(); it++) {
				if (FD_ISSET(it->first, &fdRead))
				{
					//接受数据
					RecvData(it->first);

					if (--nReady == 0)
						break;
				}
			}

			return true;
		//}

	}
	else
	{
		return false;
	}
	
}

bool EasyServer::isRun()
{
	return m_sock != INVALID_SOCKET;
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
		NewUserJoin newUser = { };
		newUser.scok = cSock;
		SendDataToAll(&newUser);
		AddClient(cSock);
		printf("socket=<%d>收到新客户端加入：socket = %d,IP = %s ,port = %d\n", m_sock, cSock, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
	}
	return cSock;
}

int EasyServer::RecvData(SOCKET cSock)
{
	int nLen = recv(cSock, m_chRecvBuff, sizeof(m_chRecvBuff), 0);
	DataHeader* header = (DataHeader*)m_chRecvBuff;
	if (nLen <= 0)
	{
		printf("客户端<socket=%d>已退出，任务结束\n", cSock);
		CloseSock(cSock);
		return -1;
	}

	ClientSocket* client = GetClient(cSock);
	if (!client)
		return -1;

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
			OnNetMsg(client->GetSocket(),header);
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

void EasyServer::OnNetMsg(SOCKET cSock,DataHeader* header)
{
	m_recvCount++;	//接收到的消息数量+1
	auto t1 = m_tTime.getElapsedSecond();
	//记录一秒钟内，收到了多少条消息
	if (t1 > 1.0)
	{
		//将时间更新为当前时间，计数更新为0
		m_tTime.update();
		m_recvCount = 0;
	}

	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		struct Login* login = (Login*)header;
		printf("收到客户端<socket=%d>请求：CMD_LOGIN,数据长度：%d,userName = %s,passWord = %s\n", cSock, login->dataLength, login->userName, login->PassWord);
		struct LoginResult loginResult;
		strcpy(loginResult.data, "登录成功");
		SendData(cSock, &loginResult);
	}
	break;
	case CMD_LOGOUT:
	{
		struct Logout* logout = (Logout*)header;
		printf("收到客户端<socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName = %s\n", cSock, logout->dataLength, logout->userName);
		struct LogoutResult logoutResult;
		SendData(cSock, &logoutResult);
	}
	break;
	case CMD_EXIT:
	{
		struct ExitConnect* cmd_exit = (ExitConnect*)header;
		printf("收到客户端<socket=%d>请求：CMD_EXIT,数据长度：%d\n", cSock, cmd_exit->dataLength);
		struct LogoutResult logoutResult;
		SendData(cSock, &logoutResult);
	}

	default:
		break;
	}

}

int EasyServer::SendData(SOCKET cSock, DataHeader* header)
{
	if (isRun() && header != NULL) {
		return send(cSock, (const char*)header, header->dataLength, 0);
	}
	return SOCKET_ERROR;
}

void EasyServer::SendDataToAll(DataHeader* data)
{
	for (auto it = m_clientFd.begin(); it != m_clientFd.begin(); it++) {
		SendData(it->first, data);
	}
}
void EasyServer::Close()
{
	for (auto it = m_clientFd.begin(); it != m_clientFd.begin(); it++) {
		CloseSock(it->first);
	}
	CloseSock(m_sock);
	m_clientFd.clear();
#ifdef _WIN32
	//清除Windows socket环境
	WSACleanup();
#endif

}

void EasyServer::CloseSock(SOCKET sock)
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

