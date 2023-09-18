#ifndef _CellTaskServer_H
#define _CellTaskServer_H

#include <functional>
#include <thread>
#include <mutex>
#include <list>

using namespace std;

class CellTaskServer;
typedef std::shared_ptr<CellTaskServer> CellTaskServerPtr;

//执行任务的服务
class CellTaskServer
{
	typedef std::function<void()> CellTask;
public:
	CellTaskServer();
	~CellTaskServer();

	void AddTask(CellTask task);
	void Start();

private:
	//工作函数
	void Run();
	thread* m_pThread;
	//任务数据,里面存放lambda表达式，可以直接执行
	list<CellTask> m_listTask;
	//任务数据缓冲区
	list<CellTask> m_listTaskBuf;
	mutex			m_mutex;
};
#endif