#ifndef _CELL_EPOLL_EVENT_
#define _CELL_EPOLL_EVENT_
#include "CELL.h"
#include <vector>
#include <time.h>
#include <map>
#include <functional>
using namespace std;
typedef std::function<int(int,int)> EventCallBack;
class CELLEpollEvent
{
	struct CellEvent_s
	{
		int sock;			//要监听的sockst
		int events;			//要监听的事件
		//给回调函数的参数，这个可有可无，因为回调函数是把类的成员函数传进去，能直接操作类的成员变量，
		//如果回调函数是普通函数的话，可以用this把对象传进去，在回调函数中，arg再转为对象
		void* arg;			
		//std::function配合bind，可以将类的成员函数作为回调函数，但是function的参数中不能传void*
		//如果要传void*，可以 typedef (*EventCallBack)(int,int,void*)
		EventCallBack callback;
		int status;			//是否在监听：1->在红黑树上(监听)，0->不在（不监听）
		int buf[1024];
		int len;			//buf的长度
		long last_active;	//记录每次加入红黑树的时间值，心跳检测
	};
public:
	CELLEpollEvent(int nSize);
	~CELLEpollEvent();

	void eventAdd(int sock, void *arg);

	void eventSet(int sock, int events,EventCallBack callback);

	void eventDel(int sock);

	int eventStart();

private:
	int _epSock;		//保存epoll_create()创建出来的红黑树树根
	map<SOCKET, CellEvent_s*> _mapCellEvent;
};
#endif // _CELL_EPOLL_EVENT_
