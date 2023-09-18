#ifndef _EasyClient_H_
#define	_EasyClient_H_

/*
	在系统socket两端，都有一个接收缓冲区和发送缓冲区，如果发送端发送太快，接收端来不及处理
	接收端的接收缓冲区慢慢就会变满，会导致缓冲区溢出，造成无法再发送，网络阻塞

	要解决这个问题，可以在接收端申请一块内存，做一个第二缓存，快速清空接收缓冲区，以达到处理网络阻塞的目的
	比如发送端每次发数据是64字节，不断的发，在接收端申请一个4096字节的缓冲区，每次取数据的时候把数据都取过去，
	每次从socket中取出4096字节，也是不断的取，理论上接收缓冲区是不会被站满的。但是每次取出的数据，
	可能是大于64字节的（好几条数据），也可能是小于64字节（不到一条数据），这样就产生了粘包和少包的情况
*/

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
#include "CELLThread.h"
#include "CELLMsgStream.h"
using namespace std;

#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#endif // !RECV_BUFF_SZIE

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
	void Start();

	//是否工作中
	bool isRun();

	//向服务器发送命令
	int SendCmd(const char* chCmd);

	//关闭连接
	void Close();

private:

	//定义接收缓冲区,模拟系统socket的接收缓冲区
	char m_chRecvBuff[RECV_BUFF_SZIE];
	//定义第二缓冲区：消息缓冲区,每次把接收缓冲区清空
	char m_chMsgBuff[RECV_BUFF_SZIE * 10];

	//消息缓冲区的数据尾部位置
	int m_nLastPos;

	CELLThread _thread;

private:

	//工作函数
	bool Run(CELLThread* pThread);

	//接受数据
	int RecvData();

	//响应数据
	virtual void OnNetMsg(netmsg_DataHeader* header);

	//向服务器发送数据
	int SendData(netmsg_DataHeader* header);

};

#endif // !_EasyClient_H_