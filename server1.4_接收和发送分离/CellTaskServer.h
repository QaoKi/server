#ifndef _CellTaskServer_H
#define _CellTaskServer_H

#include "CellTask.h"

//执行任务的服务
class CellTaskServer
{
public:
	CellTaskServer();
	~CellTaskServer();

	void AddTask(CellTask* task);
	void Start();

private:
	//工作函数
	void Run();
	thread* m_pThread;
	//任务数据
	list<CellTask*> m_listTask;
	//任务数据缓冲区
	list<CellTask*> m_listTaskBuf;
	mutex			m_mutex;
};
#endif