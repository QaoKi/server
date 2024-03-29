#include "CellTaskServer.h"

CellTaskServer::CellTaskServer()
{
}

CellTaskServer::~CellTaskServer()
{
	Close();
}

void CellTaskServer::AddTask(CellTask task)
{
	//自解锁
	lock_guard<mutex> lock(_mutex);
	_listTaskBuf.push_back(task);
}

void CellTaskServer::Start()
{
	//要想调用成员函数，需要捕获CellTaskServer对象
	_thread.Start(
		//onCreate
		nullptr,
		//onRun
		[this](CELLThread* pThread) {
		Run(pThread);
	}, 
		//onClose
		nullptr);
}

void CellTaskServer::Close()
{
	_thread.Close();
}

void CellTaskServer::Run(CELLThread* pThread)
{
	//因为Run()的运行，需要判断线程的.isRun()，如果有两个线程，这个地方就不好处理了，
	//所以线程通过参数传进来判断
	while (pThread->isRun())
	{
		//从任务缓冲区中把任务取出来
		if (_listTaskBuf.size() > 0)
		{
			lock_guard<mutex> lock(_mutex);
			for (auto pTask : _listTaskBuf)
			{
				_listTask.push_back(pTask);
			}
			_listTaskBuf.clear();
		}

		//没有任务
		if (_listTask.empty())
		{
			chrono::milliseconds t(1);
			this_thread::sleep_for(t);
			continue;
		}

		//处理任务
		for (auto pTask : _listTask)
		{
			//直接执行匿名函数
			pTask();
		}
		_listTask.clear();
	}
	//当线程结束，任务队列中的任务肯定执行完了，但是任务缓冲区中可能还有剩的任务没有处理
	//处理任务缓冲区中可能残留的任务
	lock_guard<mutex> lock(_mutex);
	for (auto pTask : _listTaskBuf)
	{
		//直接执行匿名函数
		pTask();
	}
	printf("CellTaskServer.Run()  exit\n");
}
