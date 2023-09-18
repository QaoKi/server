#ifndef _CellServer_H
#define _CellServer_H

/*
	将数据收发和接收客户端分离，此类用于处理客户端的数据，一个对象处理多个客户端
*/

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
//#include "CellTask.h"
//#include "CellSendMsg2ClientTask.h"
#include "CellTaskServer.h"
#include "CELLSemaphore.h"
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include <atomic>
#include <memory>

using namespace std;

#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#define SEND_BUFF_SIZE RECV_BUFF_SZIE
#endif 

//typedef std::shared_ptr<CellSendMsg2ClientTask> CellSendMsg2ClientTaskPtr;

class CellServer;
typedef std::shared_ptr<CellServer> CellServerPtr;

class CellServer
{
public:
	CellServer(int id);
	~CellServer();

public:
	//增加客户端到消息缓冲区
	void AddClientToBuff(ClientSocketPtr& pClient);

	//增加到正式客户队列
	void AddClient(ClientSocketPtr& pClient);
	ClientSocketPtr GetClient(SOCKET cSock);
	auto ClearClient(SOCKET cSock);

	void Start();
	//关闭
	void Close();

	int GetClientNum();

	bool Run(CELLThread * pThread);

	void SetEventObj(INetEvent* event);

public:

	//记录收到多少条消息
	static int _recvMsgCount;

	thread* _pThread;

private:
	//编号
	int _id;
	//线程
	CELLThread _thread;

	//清除所有客户端
	void ClearAllClient();

	//处理select返回之后客户端的可读可写事件
	void dealMsg(fd_set& fdRead, fd_set& fdWrite, fd_set& fdExc);

	//接收数据
	int RecvData(ClientSocketPtr& client);

	//发送数据
	int SendData(ClientSocketPtr& client);

	//响应数据
	virtual void OnNetMsg(ClientSocketPtr& pClient, netmsg_DataHeader* header);

	void addSendTask(ClientSocketPtr pClient, netmsg_DataHeader* header);

	//给所有客户端发数据
	//void SendDataToAll(DataHeader* data);

	//检测所管理的客户端的死亡计时时间
	void checkHeart();
private:

	//任务服务
	CellTaskServerPtr _TaskServerPtr;

	//事件通知
	INetEvent* _INetEvent;

	CELLTimestamp _tTime;

	//接收缓冲区
	char _chRecvBuff[RECV_BUFF_SZIE];
	/*
	  这里就不只存放客户端的socket值，而是把每个客户端的ClientSocket类传进去,
	  注意，ClientSocket类里面因为有缓冲区，内存比较大，所以存放指针比较好
	*/
	//vector<SOCKET> _clientArray;
	//正式客户队列
	//正式客户端列队不需要锁，因为是这个线程独占的，不会与其他线程进行交互
	map<SOCKET, ClientSocketPtr> _clientArray;

	//作为缓冲队列和主线程进行数据交互，当有新客户连接时，主线程把新客户放入这个缓冲
	//再从缓冲中取出新客户端放入正式客户队列
	vector<ClientSocketPtr> _clientBuff;
	//这里之所以需要锁，是因为_clientBuff是与主线程交互的，主线程随时可能会加入新的客户端进来，而子线程会随时取，所以需要加锁
	mutex _clientBuffMutex;
	//已经退出了的客户端的SOCKET
	set<SOCKET> _clientExit;
	
	//当Run开始执行的时候，时间初始化一下
	//当没有客户端的时候，每次需要更新一下时间
	time_t _oldTime;

};

#endif