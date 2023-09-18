#ifndef _CELL_H_
#define _CELL_H_

//全局的设置

#ifdef _WIN32
#define FD_SETSIZE      2506
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
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

#include<stdio.h>

//缓冲区最小单元大小
#ifndef RECV_BUFF_SZIE
	#define RECV_BUFF_SZIE 10240
	#define SEND_BUFF_SIZE RECV_BUFF_SZIE
#endif 


#endif // _CELL_H_
