#ifndef _EasySever_H
#define _EasySever_H

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h>
	#include<arpa/inet.h>
	#include<string.h>
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <iostream>
#include "MessageHeader.hpp"
#include <vector>
#include <set>

using namespace std;


class EasyServer
{
public:
	EasyServer();
	virtual ~EasyServer();

public:
	SOCKET m_sock;
	vector<SOCKET> m_clientFd;

public:
	void AddClient(SOCKET cSock);
	void ClearClient(SOCKET cSock);

public:
	//初始化socket
	void InitSocket();

	//绑定ip和端口
	int Bind(const char* ip, unsigned short port);

	//监听端口号
	int Listen(int n);

	//开始工作
	bool Run();

	//是否工作中
	bool isRun();

private:
	//接受客户端连接
	SOCKET Accept();

	//接受数据
	int RecvData(SOCKET cSock);

	//响应数据
	virtual void OnNetMsg(SOCKET cSock,DataHeader* header);

	//给指定客户端发送数据
	int SendData(SOCKET cSock, DataHeader* header);

	//给所有客户端发数据
	void SendDataToAll(DataHeader* data);

	//关闭连接
	void Close();
private:
	//关闭指定socket
	void CloseSock(SOCKET sock);
};

#endif

