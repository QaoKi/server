#include "EasyClient.h"



EasyClient::EasyClient()
{
	m_sock = INVALID_SOCKET;
	memset(m_chRecvBuff, 0, sizeof(m_chRecvBuff));
	memset(m_chMsgBuff, 0, sizeof(m_chMsgBuff));
	m_nLastPos = 0;
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
		printf("<socket=%d>close old socket...\n", (int)m_sock);
		Close();
	}
	m_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (INVALID_SOCKET == m_sock)
	{
#ifdef _WIN32
		printf("error,create socket fail...error code：%d\n", GetLastError());
#else
		perror("create socket fail:\n");
#endif
	}
	else {
		//printf("建立socket=<%d>成功...\n", (int)m_sock);
	}

	//端口复用
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
}

int EasyClient::Connect(const char* ip, unsigned short port)
{
	if (!isRun())
	{
		InitSocket();
	}
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
		printf("<socket=%d>error,connect server <%s:%d>fail,error code:%d...\n", m_sock, ip, port, GetLastError());
#else
		perror("connect server fail:\n");
#endif
	}
	else {
		printf("<socket=%d>connect server<%s:%d> succ...\n",m_sock, ip, port);
	}
	return ret;
}

void EasyClient::Start()
{
	_thread.Start(
		//onCreate
		nullptr,
		//onRun
		[this](CELLThread* pThread) {
		Run(pThread);
	},
		//onClose
		nullptr
		);
}

bool EasyClient::Run(CELLThread* pThread)
{
	//搭建一个select模型
	fd_set fdRead;

	while (pThread->isRun())
	{
		FD_ZERO(&fdRead);
		FD_SET(m_sock, &fdRead);
		timeval tTimeOut = { 0,0 };

		int nReady = select(m_sock + 1, &fdRead, NULL, NULL, &tTimeOut);

		if (nReady == 0){
			continue;
		}
		else if (nReady < 0)
		{
			printf("<socket=%d>select exit\n", m_sock);
			pThread->Exit();
			break;
		}
		else
		{
			if (FD_ISSET(m_sock, &fdRead))
			{
				FD_CLR(m_sock, &fdRead);
				RecvData();
			}
		}
	}
	return true;
}

void EasyClient::Close()
{
	_thread.Close();
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
	//先接收到接收缓存区中
	int nLen = recv(m_sock, m_chRecvBuff, sizeof(m_chRecvBuff), 0);
	if (nLen <= 0)
	{
		printf("<socket=%d>Disconnect from the server,exit\n", m_sock);
		Close();
		return -1;
	}
	//将收取到的数据拷贝到消息缓冲区的尾部,接收到多少拷贝过去多少
	memcpy(m_chMsgBuff + m_nLastPos, m_chRecvBuff, nLen);
	//消息缓冲区的数据尾部位置后移
	m_nLastPos += nLen;

	//只要消息缓冲区中的数据，长度大于消息头netmsg_DataHeader长度
	while (m_nLastPos >= sizeof(netmsg_DataHeader))
	{
		//这时就可以知道当前消息的长度
		netmsg_DataHeader* header = (netmsg_DataHeader*)m_chMsgBuff;
		//消息缓冲区中的数据长度，大于等于当前数据的长度
		if (m_nLastPos >= header->dataLength)
		{
			//因为重新memcpy之后，header的数据就变了，相应的header->dataLength也就变了，当移动nLastPos的时候会出错
			//所以先记录下来
			//消息区还未处理的消息长度
			int size = m_nLastPos - header->dataLength;
			//处理消息
			OnNetMsg(header);
			//从消息缓冲区中去除这条消息（把这条消息之后的数据移到缓冲区头部）
			memcpy(m_chMsgBuff, m_chMsgBuff + header->dataLength, size);
			//移动消息缓冲区尾部位置
			m_nLastPos = size;
		}
		else
		{
			//消息缓冲区剩余数据不够一条完整消息
			break;
		}
	}

	return 0;
}

void EasyClient::OnNetMsg(netmsg_DataHeader* header)
{
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		struct LoginResult* loginResult = (LoginResult*)header;
		printf("recv server request:CMD_LOGIN_RESULT,data length:%d,data:%s\n",loginResult->dataLength, loginResult->data);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		struct LogoutResult* logoutResult = (LogoutResult*)header;
		printf("recv server request:CMD_LOGOUT_RESULT,data length:%d\n", logoutResult->dataLength);
	}
	case CMD_NEW_USER_JOIN:
	{
		struct NewUserJoin* newUser = (NewUserJoin*)header;
		printf("recv server request:CMD_NEW_USER_JOIN,new use socket:%d,data length:%d\n", newUser->scok,newUser->dataLength);
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

int EasyClient::SendData(netmsg_DataHeader* header)
{
	if (isRun() && header)
	{
		return send(m_sock, (const char*)header, header->dataLength, 0);
	}
	return SOCKET_ERROR;

}

