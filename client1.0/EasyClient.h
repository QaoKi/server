#ifndef _EasyClient_H_
#define	_EasyClient_H_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <iostream>
#include "MessageHeader.hpp"
#include <vector>
using namespace std;
class EasyClient
{
public:
	EasyClient();
	virtual ~EasyClient();

public:
	SOCKET m_sock;

public:
	//初始化socket
	void InitSocket();

	//连接服务器
	int Connect(const char* ip, unsigned short port);

	//开始工作
	bool Run();

	//是否工作中
	bool isRun();

	//向服务器发送命令
	int SendCmd(const char* chCmd);

	//关闭连接
	void Close();

private:

	//接受数据
	int RecvData();

	//响应数据
	virtual void OnNetMsg(DataHeader* header);

	//向服务器发送数据
	int SendData(DataHeader* header);

};

#endif // !_EasyClient_H_