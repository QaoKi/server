#include "EasyClient.h"



EasyClient::EasyClient()
{
	m_sock = INVALID_SOCKET;
}


EasyClient::~EasyClient()
{
	Close();
}

void EasyClient::InitSocket()
{
#ifdef _WIN32
	//启动Windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	if (INVALID_SOCKET != m_sock)
	{
		printf("<socket=%d>关闭旧连接...\n", (int)m_sock);
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

int EasyClient::Connect(const char* ip, unsigned short port)
{
	struct sockaddr_in serverAddr = { 0 };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
#ifdef _WIN32
	serverAddr.sin_addr.S_un.S_addr = inet_addr(ip);
#else
	serverAddr.sin_addr.s_addr = inet_addr(ip);
#endif

	int ret = connect(m_sock, (sockaddr*)&serverAddr, sizeof(sockaddr_in));
	if (ret < 0)
	{
#ifdef _WIN32
		printf("<socket=%d>错误，连接服务器<%s:%d>失败,错误码：%d...\n", m_sock, ip, port, GetLastError());
#else
		perror("连接服务器失败:");
#endif
	}
	else {
		printf("<socket=%d>连接服务器<%s:%d>成功...\n",m_sock, ip, port);
	}
	return ret;
}

bool EasyClient::Run()
{
	//搭建一个select模型
	fd_set fdRead;

	//while (isRun())
	//{
		FD_ZERO(&fdRead);
		FD_SET(m_sock, &fdRead);
		timeval tTimeOut = { 0,0 };

		int nReady = select(m_sock + 1, &fdRead, NULL, NULL, &tTimeOut);

		if (nReady == 0){
			return false;
		}
		else if (nReady < 0)
		{
			printf("<socket=%d>select任务结束\n", m_sock);
			Close();
			return false;
		}
		else
		{
			if (FD_ISSET(m_sock, &fdRead))
			{
				FD_CLR(m_sock, &fdRead);
				RecvData();
			}
		}
	//}
	return true;
}

void EasyClient::Close()
{
	if (m_sock != INVALID_SOCKET)
	{
#ifdef _WIN32
		closesocket(m_sock);
		//清除Windows socket环境
		WSACleanup();
#else
		close(m_sock);
#endif // _WIN32
		m_sock = INVALID_SOCKET;
	}
}

bool EasyClient::isRun()
{
	return m_sock != INVALID_SOCKET;
}

int EasyClient::RecvData()
{
	char chBuff[4096] = { 0 };
	int nLen = recv(m_sock, chBuff, sizeof(chBuff), 0);
	if (nLen <= 0)
	{
		printf("<socket=%d>与服务器断开连接，任务结束。\n", m_sock);
		Close();
		return -1;
	}
	DataHeader* header = (DataHeader*)chBuff;
	OnNetMsg(header);

	return 0;
}

void EasyClient::OnNetMsg(DataHeader* header)
{
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		struct LoginResult* loginResult = (LoginResult*)header;
		printf("收到服务端回复：CMD_LOGIN_RESULT,数据长度：%d，内容：%s\n",loginResult->dataLength, loginResult->data);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		struct LogoutResult* logoutResult = (LogoutResult*)header;
		printf("收到服务端回复：CMD_LOGOUT_RESULT,数据长度：%d\n", logoutResult->dataLength);
	}
	case CMD_NEW_USER_JOIN:
	{
		struct NewUserJoin* newUser = (NewUserJoin*)header;
		printf("收到服务端回复：CMD_NEW_USER_JOIN,新用户socket:%d,数据长度：%d\n", newUser->scok,newUser->dataLength);
	}
	break;
	default:
		break;
	}

}


int EasyClient::SendCmd(const char* chCmd)
{
	string strCmd(chCmd);

	if (strCmd.compare("login") == 0){
		Login login;
		strcpy(login.userName, "zf");
		strcpy(login.PassWord, "123");
		SendData(&login);
	}
	else if (strCmd.compare("logout") == 0) {
		Logout logout;
		strcpy(logout.userName, "zf");
		SendData(&logout);
	}
	else if(strCmd.compare("exit") == 0) {
		ExitConnect exit;
		SendData(&exit);
	}
	else
	{
		return -1;
	}
	return 0;
}

int EasyClient::SendData(DataHeader* header)
{
	if (isRun() && header)
	{
		return send(m_sock, (const char*)header, header->dataLength, 0);
	}
	return SOCKET_ERROR;

}

