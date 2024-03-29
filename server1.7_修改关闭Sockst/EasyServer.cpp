#include "EasyServer.h"


EasyServer::EasyServer()
{
	_sock = INVALID_SOCKET;
	nRecvMsgCoutn = 0;
	//memset(_chRecvBuff, 0, sizeof(_chRecvBuff));
}


EasyServer::~EasyServer()
{
	Close();
}

void EasyServer::AddClient(ClientSocketPtr& pClient)
{
	if (_clientArray.find(pClient->GetSocket()) == _clientArray.end())
	{
		_clientArray[pClient->GetSocket()] = pClient;
	}
}

void EasyServer::ClearClient(SOCKET cSock)
{
	auto it = _clientArray.find(cSock);
	if (it != _clientArray.end())
	{
		_clientArray.erase(it);
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
	if (INVALID_SOCKET != _sock)
	{
		printf("<socket=%d>关闭旧连接...\n",(int)_sock);
		Close();
	}
	_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (INVALID_SOCKET == _sock)
	{
#ifdef _WIN32
		printf("错误，建立socket失败...错误：%d\n", GetLastError());
#else
		perror("建立socket失败:");
#endif
	}
	else {
		printf("建立socket=<%d>成功...\n", (int)_sock);
	}

	//端口复用
	int opt = 1;
	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
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
	int ret = ::bind(_sock, (sockaddr*)&sin, sizeof(sockaddr_in));
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
	int ret = listen(_sock, n);
	if (SOCKET_ERROR == ret)
	{
#ifdef _WIN32
		printf("socket=<%d>错误,监听网络端口失败...错误：%d\n", _sock,GetLastError());
#else
		perror("监听网络端口失败:");
#endif
	}
	else {
		printf("socket=<%d>监听网络端口成功...\n",_sock);
	}
	return ret;

}

void EasyServer::Start(int nCellServerCount)
{
	for (int i = 0; i < nCellServerCount; i++)
	{
		auto pSer = std::make_shared<CellServer>(i+1);
		_CellServerArray.push_back(pSer);
		pSer->Start();
		pSer->SetEventObj(this);
	}
	Run();
}

bool EasyServer::Run()
{

	//搭建一个select模型
	fd_set fdRead;
	SOCKET sMax = _sock;

	timeval tTimeout = { 0,0 };

	while (isRun())
	{			
		Time4Msg();
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		int nReady = select(sMax + 1, &fdRead, NULL, NULL, &tTimeout);

		if (nReady < 0)
		{
			printf("服务器任务结束。\n");
			Close();
			return false;
		}
		else if (nReady == 0) {
			chrono::milliseconds t(1);
			this_thread::sleep_for(t);
			continue;
		}

		if (FD_ISSET(_sock, &fdRead))
		{
			//接受新连接
			Accept();
		}

	}
	return true;
}

bool EasyServer::isRun()
{
	return _sock != INVALID_SOCKET;
}

void EasyServer::ClientLeaveEvent(ClientSocketPtr& pClient)
{
	ClearClient(pClient->GetSocket());
}

void EasyServer::RecvMsgEvent(ClientSocketPtr& pClient)
{
	nRecvMsgCoutn++;
}

SOCKET EasyServer::Accept()
{
	SOCKET cSock = INVALID_SOCKET;
	sockaddr_in clientAddr = { 0 };
	int nAddrlen = sizeof(sockaddr_in);

#ifdef _WIN32
	cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrlen);
#else
	cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrlen);
#endif
	if (cSock <= 0) {
#ifdef _WIN32
		printf("socket=<%d>错误,接受到无效客户端SOCKET...错误：%d\n", _sock, GetLastError());
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

		//用智能指针，如果用make_shared构造，不会调用对象池
		//auto pClient = std::make_shared<ClientSocket>(cSock);
		ClientSocketPtr pClient(new ClientSocket(cSock));
		AddClient(pClient);
		//将这个客户端，发给CellServer进行数据管理
		AddClientToCellServer(pClient);

		//printf("socket=<%d>收到新客户端加入：socket = %d,IP = %s ,port = %d\n", _sock, cSock, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		//printf("socket=<%d>收到新客户端加入：socket = %d,客户端数量：%d\n", _sock, cSock, _clientArray.size());
	}
	return cSock;
}

void EasyServer::AddClientToCellServer(ClientSocketPtr& pClient)
{
	//发给客户数量最少的CellServer
	//如果都一样，保存到第一个
	auto pMinNumClient = _CellServerArray[0];
	for (size_t i = 1; i < _CellServerArray.size(); i++)
	{
		pMinNumClient = pMinNumClient->GetClientNum() > _CellServerArray[i]->GetClientNum() ? _CellServerArray[i] : pMinNumClient;
	}

	pMinNumClient->AddClientToBuff(pClient);
}

void EasyServer::Time4Msg()
{
	auto t1 = _tTime.getElapsedSecond();
	//记录一秒钟内，收到了多少条消息
	if (t1 > 1.0)
	{
		//将时间更新为当前时间，计数更新为0
		_tTime.update();
		printf("socket=<%d>,时间：%lf,客户端数量：%d,消息数量：%d\n", _sock, t1, _clientArray.size(),CellServer::_recvMsgCount);
		CellServer::_recvMsgCount = 0;
	}

}

void EasyServer::Close()
{

	if (_sock != INVALID_SOCKET)
	{
		_CellServerArray.clear();
		_clientArray.clear();

#ifdef _WIN32
		closesocket(_sock);
#else
		close(_sock);

#endif
		_sock = INVALID_SOCKET;
	}
}


