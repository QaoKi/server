#ifndef _EasySever_H
#define _EasySever_H

#ifdef _WIN32
	#define FD_SETSIZE      2506
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
#include "ClientSocket.h"
#include "INetEvent.h"
#include "CellServer.h"
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include <atomic>
#include <memory>

using namespace std;

/*
	1.1:为每个客户端分配一个消息缓存区，所有客户端共用一个接收缓存区
*/
/*
	1.2:主线程用来接收客户端连接和客户端断开连接事件，子线程（CellServer）用于处理网络数据
		主线程和子线程之间通过缓冲队列进行通信
*/
/*
	1.3:通过实验发现，recv的接收能力，远大于send的发送能力，所以，要对发送数据进行改进
		通过隔一定的时间发送，或者收满一定的数据再发送的方式来改进数据的发送

	1.4:另外开线程处理服务端发给客户端的数据，CellTaskServer对象中开线程发送数据，CellTaskServer是在CellServer中申请的
		内存，所有，有多少个CellServer（接收服务），就有多少个CellTaskServer（发送服务）

	1.5：内存管理
		避免内存碎片的产生，使程序长期稳定、高效的运行
		内存池：从系统中申请足够大小的内存，由程序自己管理
		对象池：创建足够多的对象，减少创建释放对象的消耗
		windows系统中的文件，都是4k对齐的（即使大小没有4k，也是占用4k的大小），占用的大小，都是4k的整数倍
		自己设计的内存池：一共分为5个池，
			1~64字节为一个池，65~128字节为一个池，129~256字节为一个池，257~512字节为一个池，513~1024字节为一个池
			如果申请的内存在64字节以内，就分配一个1~64字节的池空间，如果在申请的内存在65~128字节。。。
*/

#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240 * 5
#define SEND_BUFF_SIZE RECV_BUFF_SZIE
#endif 

class EasyServer : public INetEvent
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
	//vector<SOCKET> m_clientArray;
	map<SOCKET, ClientSocketPtr> m_clientArray;
	//保存CellServer
	vector<CellServerPtr> m_CellServerArray;

public:
	void AddClient(ClientSocketPtr& pClient);
	ClientSocketPtr GetClient(SOCKET cSock);
	void ClearClient(SOCKET cSock);
public:
	//初始化socket
	void InitSocket();

	//绑定ip和端口
	int Bind(const char* ip, unsigned short port);

	//监听端口号
	int Listen(int n);

	//开启处理消息的线程,并开始监听客户端连接
	void Start(int nCellServerCount);

	//开始工作
	bool Run();

	//是否工作中
	bool isRun();

	//客户端离开事件
	virtual void ClientLeaveEvent(ClientSocketPtr& pClient);

	virtual void RecvMsgEvent(ClientSocketPtr& pClient);

private:
	//接受缓冲区
	//char m_chRecvBuff[RECV_BUFF_SZIE];
	CELLTimestamp m_tTime;

	//记录客户端发过来多少条消息
	//因为有多个线程会同时操作，所以需要用原子类型
	atomic_int nRecvMsgCoutn;

private:
	//接受客户端连接
	SOCKET Accept();

	//把客户端发给CelleServer
	void AddClientToCellServer(ClientSocketPtr& pClient);

	//输出一秒钟收到多少条消息
	void Time4Msg();

	//关闭连接
	//void Close();

	//关闭指定socket
	void CloseSock(SOCKET sock);
};

#endif

