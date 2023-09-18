#ifndef _CELL_THREAD_POOL_H_
#define _CELL_THREAD_POOL_H_

#include <functional>
#include <list>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include "CELLSemaphore.h"
#include "CELLThread.h"
using namespace std;

class CELLThreadPool
{
	typedef std::function<void()> CellTask;
public:
	CELLThreadPool(int threadCount);
	~CELLThreadPool();

	void AddTask(CellTask task);

private:
	void Run();
	void adjustThread();
	vector<thread> _vecThreads;
	thread* _adjust;	//	管理者线程

	CELLSemaphore _task_empty;	//	没有任务了
	int _threadCount;
	//任务数据,里面存放lambda表达式，可以直接执行
	list<CellTask> _listTask;
	//任务数据缓冲区
	list<CellTask> _listTaskBuf;
	mutex	_mutexTaskBuff;
	mutex	_mutexTaskList;
	
};

#endif

