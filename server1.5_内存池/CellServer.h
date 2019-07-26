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
#include "CellTask.h"
#include "CellSendMsg2ClientTask.h"
#include "CellTaskServer.h"
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

typedef std::shared_ptr<CellSendMsg2ClientTask> CellSendMsg2ClientTaskPtr;

class CellServer;
typedef std::shared_ptr<CellServer> CellServerPtr;

class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET);
	~CellServer();

public:
	//增加客户端到消息缓冲区
	void AddClientToBuff(ClientSocketPtr& pClient);

	//增加到正式客户队列
	void AddClient(ClientSocketPtr& pClient);
	ClientSocketPtr GetClient(SOCKET cSock);
	auto ClearClient(SOCKET cSock);

	void Start();

	int GetClientNum();

	bool Run();

	//是否工作中
	bool isRun();

	void SetEventObj(INetEvent* event);

public:

	//记录收到多少条消息
	static int m_recvMsgCount;

	thread* m_pThread;

private:
	//关闭连接
	void Close();

	//关闭指定socket
	void CloseSock(SOCKET sock);

	//接受数据
	int RecvData(SOCKET cSock);

	//响应数据
	virtual void OnNetMsg(ClientSocketPtr pClient, DataHeader* header);

	void addSendTask(ClientSocketPtr pClient, DataHeader* header);

	//给所有客户端发数据
	//void SendDataToAll(DataHeader* data);

private:
	SOCKET m_sock;

	//任务服务
	CellTaskServer* m_TaskServer;

	//事件通知
	INetEvent* m_INetEvent;

	CELLTimestamp m_tTime;

	//接收缓冲区
	char m_chRecvBuff[RECV_BUFF_SZIE];
	/*
	  这里就不只存放客户端的socket值，而是把每个客户端的ClientSocket类传进去,
	  注意，ClientSocket类里面因为有缓冲区，内存比较大，所以存放指针比较好
	*/
	//vector<SOCKET> m_clientArray;
	//正式客户队列
	map<SOCKET, ClientSocketPtr> m_clientArray;
	//作为缓冲队列和主线程进行数据交互，当有新客户连接时，主线程把新客户放入这个缓冲
	//再从缓冲中取出新客户端放入正式客户队列
	vector<ClientSocketPtr> m_clientBuff;

	//已经退出了的客户端的SOCKET
	set<SOCKET> m_clientExit;
	mutex m_clientBuffMutex;
};

#endif