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
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include <atomic>

using namespace std;

/*
	1.1:为每个客户端分配一个消息缓存区，所有客户端共用一个接收缓存区
*/
/*
	1.2:主线程用来接收客户端连接和客户端断开连接事件，子线程用于处理网络数据
		主线程和子线程之间通过缓冲队列进行通信
*/


#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240
#endif // !RECV_BUFF_SZIE



//为每个客户端创建一个新类
class ClientSocket
{
public:
	ClientSocket(SOCKET sock = INVALID_SOCKET);
	virtual ~ClientSocket();

public:
	SOCKET GetSocket();

	int GetLastPos();

	void SetLastPos(int pos);

	void SetSocket(SOCKET sock);

	char* GetMsgBuf();

private:

	SOCKET m_ClientSocket;
	//消息缓存区
	char m_chMsgBuf[RECV_BUFF_SZIE * 5];
	//消息缓冲区的数据尾部位置
	int m_nLastPos;
};

class INetEvent
{
public:
	//纯虚函数
	//客户端离开事件
	virtual void ClientLeaveEvent(ClientSocket* pClient) = 0;
	//之前用的是静态变量，记录收到了多少条消息，现在用事件的形式
	virtual void RecvMsgEvent(ClientSocket* pClient) = 0;
};

class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET);
	~CellServer();

public:
	//增加客户端到消息缓冲区
	void AddClientToBuff(ClientSocket* pClient);
	
	//增加到正式客户队列
	void AddClient(ClientSocket* pClient);
	ClientSocket* GetClient(SOCKET cSock);
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
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header);

	//发送数据
	int SendData(SOCKET cSock, DataHeader* header);

	//给所有客户端发数据
	void SendDataToAll(DataHeader* data);

private:
	SOCKET m_sock;

	//事件通知
	INetEvent* m_INetEvent;

	CELLTimestamp m_tTime;

	//接收缓冲区
	char m_chRecvBuff[RECV_BUFF_SZIE * 5];
	/*
	  这里就不只存放客户端的socket值，而是把每个客户端的ClientSocket类传进去,
	  注意，ClientSocket类里面因为有缓冲区，内存比较大，所以存放指针比较好
	*/
	//vector<SOCKET> m_clientArray;
	//正式客户队列
	map<SOCKET, ClientSocket*> m_clientArray;
	//作为缓冲队列和主线程进行数据交互，当有新客户连接时，主线程把新客户放入这个缓冲
	//再从缓冲中取出新客户端放入正式客户队列
	vector<ClientSocket*> m_clientBuff;

	//已经退出了的客户端的SOCKET
	set<SOCKET> m_clientExit;
	mutex m_clientBuffMutex;

};

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
	map<SOCKET,ClientSocket*> m_clientArray;
	//保存CellServer
	vector<CellServer*> m_CellServerArray;

public:
	void AddClient(ClientSocket* pClient);
	ClientSocket* GetClient(SOCKET cSock);
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
	virtual void ClientLeaveEvent(ClientSocket* pClient);

	virtual void RecvMsgEvent(ClientSocket* pClient);

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
	void AddClientToCellServer(ClientSocket* pClient);

	//输出一秒钟收到多少条消息
	void Time4Msg();

	//关闭连接
	//void Close();

	//关闭指定socket
	void CloseSock(SOCKET sock);
};

#endif

