#include "EasyServer.h"


EasyServer::EasyServer()
{
	m_sock = INVALID_SOCKET;
}


EasyServer::~EasyServer()
{
	Close();
}

void EasyServer::AddClient(SOCKET cSock)
{
	auto it = m_clientFd.begin();
	for (it; it != m_clientFd.end(); it++)
	{
		if(*it == cSock)
			return;
	}
	m_clientFd.push_back(cSock);
}

void EasyServer::ClearClient(SOCKET cSock)
{
	vector<SOCKET>::iterator it = m_clientFd.begin();
	for (size_t i = 0; i < m_clientFd.size(); i++)
	{
		if (*it == cSock)
			m_clientFd.erase(it);
		else
			it++;
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
			for (size_t i = 0; i < m_clientFd.size(); i++){
				FD_SET(m_clientFd[i], &fdRead);
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

			for (size_t i = 0; i < m_clientFd.size(); i++)
			{
				if (FD_ISSET(m_clientFd[i], &fdRead))
				{
					//接受数据
					RecvData(m_clientFd[i]);

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
	char chBuff[4096] = { 0 };
	int nLen = recv(cSock, chBuff, sizeof(chBuff), 0);
	DataHeader* header = (DataHeader*)chBuff;
	if (nLen <= 0)
	{
		printf("客户端<socket=%d>已退出，任务结束\n", cSock);
		CloseSock(cSock);
		return -1;
	}

	OnNetMsg(cSock,header);
	return 0;
}

void EasyServer::OnNetMsg(SOCKET cSock,DataHeader* header)
{
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
	for (size_t i = 0; i < m_clientFd.size(); i++) {
		SendData(m_clientFd[i], data);
	}
}
void EasyServer::Close()
{
	for (size_t i = 0; i < m_clientFd.size(); i++)
	{
		CloseSock(m_clientFd[i]);
	}
	CloseSock(m_sock);
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

