#include "CellTaskServer.h"

CellTaskServer::CellTaskServer()
{
	m_pThread = NULL;
}

CellTaskServer::~CellTaskServer()
{
	if (m_pThread)
	{
		delete m_pThread;
		m_pThread = NULL;
	}

}

void CellTaskServer::AddTask(CellTaskPtr& task)
{
	//自解锁
	lock_guard<mutex> lock(m_mutex);
	m_listTaskBuf.push_back(task);
}

void CellTaskServer::Start()
{
	m_pThread = new thread(mem_fun(&CellTaskServer::Run), this);
	m_pThread->detach();
}

void CellTaskServer::Run()
{
	while (true)
	{
		//从任务缓冲区中把任务取出来
		if (m_listTaskBuf.size() > 0)
		{
			lock_guard<mutex> lock(m_mutex);
			for (auto pTask : m_listTaskBuf)
			{
				m_listTask.push_back(pTask);
			}
			m_listTaskBuf.clear();
		}

		//没有任务
		if (m_listTask.empty())
		{
			chrono::milliseconds t(1);
			this_thread::sleep_for(t);
			continue;
		}

		//处理任务
		for (auto pTask : m_listTask)
		{
			pTask->doTask();
			//if (pTask)
			//{
			//	delete pTask;
			//	pTask = NULL;
			//}
		}
		m_listTask.clear();
	}
}
