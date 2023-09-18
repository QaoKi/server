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
	在linux下编译命令
	g++ -std=c++11 server.cpp -o server -pthread 
*/
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

	1.6：添加了对象池，原理和内存池相似，并用lambda表达式对task进行了改造
		加入了心跳机制，当一定时间内没有收到客户端发过来的消息，就关闭和客户端的连接
		定时发送消息:当到达一定的时间，把缓冲区里的数据全部发给客户端

	1.7： 1.当客户端退出，关闭客户端Socket的操作，交给ClientSocket类来完成，EasyServer和CellServer类只负责清除所管理的ClientSocket对象
		  2.CellServer和CellTaskServer中的Run()函数都是新建线程运行的，里面会用到类中的成员变量，当CellServer和CellTaskServer析构了以后，
		  如果子线程还没有释放，Run()会继续执行，再使用到成员变量会造成异常，使用条件变量，使子线程退出之后，对象再析构（使用智能指针管理
		  对象，不会出现这个问题，因为子线程的循环运行，是靠变量isRun来控制，可以保证当对象析构之前，会被调用Close()，将isRun改为false）

	1.8：封装了线程

	2.0：之前的架构，socket都为阻塞型，select只是监听了可读事件，没有监听可写事件，当服务器像客户端返回数据的时候，直接调用了send函数，
		这是因为客户端也在我们的掌握之内，我们知道客户端只要正常运行就能够接收数据，但是如果客户端不能正常接收，send就会阻塞
		当客户端不可掌控时，阻塞send是不可以的
		使用阻塞模式send的意义：随时可写，控制收发简单	

		修改ClientSocket类中SendData的逻辑，要发送的数据进来，先判断缓冲区剩余的大小能不能盛开，如果能，把数据放到缓冲区，当收到客户端的写
		事件以后把数据发过去，如果不能，返回错误

		可写事件：接收端可以接收数据

		定时发送数据和定量发送数据在异步发送数据中意义不大

	2.1：分离消息缓冲区，封装缓冲区CELLBuffer
		  加入运行日志记录


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

	//初始化socket
	void InitSocket();

	//绑定ip和端口
	int Bind(const char* ip, unsigned short port);

	//监听端口号
	int Listen(int n);

	//开启处理消息的线程,并开始监听客户端连接
	void Start(int nCellServerCount);

	//关闭服务
	void Close();

private:
	SOCKET _sock;

	//线程
	CELLThread _thread;

	/*
	这里就不只存放客户端的socket值，而是把每个客户端的ClientSocket类传进去,
	 注意，ClientSocket类里面因为有缓冲区，内存比较大，所以存放指针比较好
	*/
	map<SOCKET, ClientSocketPtr> _clientArray;
	//如果有多个线程，一起监听_sock的客户端连接事件，这里需要加锁
	mutex _clientArrayMutex;
	//保存CellServer
	vector<CellServerPtr> _CellServerArray;

	//接受缓冲区
	//char _chRecvBuff[RECV_BUFF_SZIE];
	CELLTimestamp _tTime;

	//记录客户端发过来多少条消息
	//因为有多个线程会同时操作，所以需要用原子类型
	atomic_int nRecvMsgCoutn;

private:

	void AddClient(ClientSocketPtr& pClient);
	void ClearClient(SOCKET cSock);


	//开始工作
	bool Run(CELLThread* pThread);

	//是否工作中
	bool isRun();

	//接受客户端连接
	SOCKET Accept();

	//把客户端发给CelleServer
	void AddClientToCellServer(ClientSocketPtr& pClient);

	//输出一秒钟收到多少条消息
	void Time4Msg();

	//客户端离开事件
	virtual void ClientLeaveEvent(ClientSocketPtr& pClient);

	virtual void RecvMsgEvent(ClientSocketPtr& pClient);

};

#endif

