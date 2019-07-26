#include "CELLEpollEvent.h"



CELLEpollEvent::CELLEpollEvent(int nSize)
{
	_epSock = epoll_create(nSize);
}


CELLEpollEvent::~CELLEpollEvent()
{
	for (auto temp : _mapCellEvent)
	{
		if (temp.second)
			delete temp.second;
	}
	_mapCellEvent.clear();
}


void CELLEpollEvent::eventAdd(int sock, void *arg)
{
	CellEvent_s* event_s = new CellEvent_s;
	memset(event_s, 0, sizeof(CellEvent_s));
	event_s->sock = sock;
	event_s->callback = nullptr;
	event_s->events = 0;
	event_s->arg = arg;
	event_s->status = 0;
	event_s->last_active = time(NULL);
	_mapCellEvent[sock] = event_s;
}


void CELLEpollEvent::eventSet(int sock, int events, EventCallBack callback)
{
	struct epoll_event ep_in;
	CellEvent_s* ev = _mapCellEvent[sock];
	ep_in.events = ev->events = events;
	ep_in.data.ptr = ev;
	ev->callback = callback;
	int op;
	if (ev->status == 1)
	{
		//已经在红黑树 g_efd里，修改其属性
		op = EPOLL_CTL_MOD;
	}
	else
	{
		//不在红黑树里，将其加入红黑树 _epSock，并将status置1
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	if (epoll_ctl(_epSock, op, sock, &ep_in) < 0)
	{
		perror("epoll_ctl error:");
	}
}

void CELLEpollEvent::eventDel(int sock)
{
	auto ev = _mapCellEvent.find(sock);

	if (ev != _mapCellEvent.end())
	{
		//如果不在树上
		if (ev->second->status == 0)
			return;

		if (ev->second)
			delete ev->second;
		_mapCellEvent.erase(ev);
	}

	if (epoll_ctl(_epSock, EPOLL_CTL_DEL, sock, NULL) < 0)
	{
		perror("epoll_ctl error:");
	}
}

int CELLEpollEvent::eventStart()
{
	//开始监听所管理的所有的socket
	struct epoll_event ep_out[FD_SETSIZE];
	int nReady = epoll_wait(_epSock, ep_out, FD_SETSIZE, 0);

	if (nReady <= 0)
	{
		return nReady;
	}

	for (int i = 0; i < nReady; i++)
	{
		CellEvent_s* event_s = (CellEvent_s *)ep_out[i].data.ptr;
		if (event_s->events == ep_out[i].events)
		{
			event_s->callback(event_s->sock, event_s->events);
		}
	}

}
