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
#include "CELLTimestamp.h"
#include <vector>
#include <set>
#include <map>

using namespace std;

/*
	为每个客户端分配一个消息缓存区，所有客户端共用一个接收缓存区
*/

#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#endif // !RECV_BUFF_SZIE

//为每个客户端创建一个新类
class ClientSocket
{
public:
	ClientSocket(SOCKET m_ClientSocket);
	virtual ~ClientSocket();

public:
	SOCKET GetSocket();

	int GetLastPos();

	void SetLastPos(int pos);

	char* GetMsgBuf();

private:

	SOCKET m_ClientSocket;
	//消息缓存区
	char m_chMsgBuf[RECV_BUFF_SZIE * 10];
	//消息缓冲区的数据尾部位置
	int m_nLastPos;
};


class EasyServer
{
public:
	EasyServer();
	virtual ~EasyServer();

public:
	SOCKET m_sock;
	/*
	  这里就不只存放客户端的socket值，而是把每个客户端的ClientSocket类传进去,
	  注意，ClientSocket类里面因为有缓冲区，内存比较大，所以存放指针比较好
	*/
	//vector<SOCKET> m_clientFd;
	map<SOCKET,ClientSocket*> m_clientFd;

public:
	void AddClient(SOCKET cSock);
	ClientSocket* GetClient(SOCKET cSock);
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
	//接受缓冲区
	char m_chRecvBuff[RECV_BUFF_SZIE];
	CELLTimestamp m_tTime;
	//计数：接收到多少条消息
	int m_recvCount;

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

	//关闭指定socket
	void CloseSock(SOCKET sock);
};

#endif

