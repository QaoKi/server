#include "CELLThreadPool.h"


CELLThreadPool::CELLThreadPool(int threadCount)
{
	_threadCount = threadCount;
	if (_threadCount <= 0 || _threadCount > 10)
		_threadCount = 4;
	for (int i = 0; i < _threadCount; i++)
	{
		thread t(std::mem_fun(&CELLThreadPool::Run));
		_vecThreads.push_back(t);
	}
	_adjust = new thread(std::mem_fun(&CELLThreadPool::Run));
}


CELLThreadPool::~CELLThreadPool()
{
	if (_adjust)
		delete _adjust;
}

void CELLThreadPool::AddTask(CellTask task)
{
	//×Ô½âËø
	lock_guard<mutex> lock(_mutexTaskBuff);
	_listTaskBuf.push_back(task);
	_task_empty.wait();
}

void CELLThreadPool::Run()
{
	while (true)
	{
		if (!_listTask.empty())
		{
			lock_guard<mutex> lock(_mutexTaskBuff);
			for (auto task : _listTaskBuf)
				_listTask.push_back(task);

			_listTask.clear();
		}

		if (_listTask.empty())
			_task_empty.wait();

		lock_guard<mutex> lock(_mutexTaskList);
		CellTask task = _listTask.front();
		task();
		_listTask.pop_front();
	}
}

void CELLThreadPool::adjustThread()
{

}

