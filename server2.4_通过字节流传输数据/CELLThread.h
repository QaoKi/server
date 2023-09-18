#ifndef _CELL_THREAD_H_
#define _CELL_THREAD_H_
#include "CELLSemaphore.h"
#include <mutex>

using namespace std;

class CELLThread
{
private:
	//创建一个返回值为void，参数为CELLThread*的函数指针
	typedef std::function<void(CELLThread*)> EventCall;
public:

	//启动线程,启动的时候，注册各个事件
	void Start(EventCall onCreate = nullptr, 
			   EventCall onRun = nullptr,
			   EventCall onDestory = nullptr);
	//关闭线程,暴露给外部，外部调用以后，关闭线程
	void Close();
	//退出线程，用在线程的工作函数中
	void Exit();
	
	bool isRun();
public:
	//封装一个Sleep函数，方便使用
	static void Sleep(time_t dt);
protected:
	//线程运行时的工作函数
	//线程本身不会循环运行，需要使用者自己定义
	void OnWork();

private:
	//注册线程事件
	EventCall _onCreate;
	//运行事件
	EventCall _onRun;
	//销毁线程事件
	EventCall _onDestory;
	//线程是否启动运行中
	bool  _isRun = false;
	//控制线程的运行状态
	CELLSemaphore _sem;

	//保证多个线程中运行时同步，多线程中可以一个线程调用Start()，一个调用Close()
	mutex _mutex;

};
#endif // _CELL_THREAD_H_
