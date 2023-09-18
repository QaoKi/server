#include "CELLThread.h"

void CELLThread::Start(EventCall onCreate /*= nullptr*/, EventCall onRun /*= nullptr*/, EventCall onDestory /*= nullptr*/)
{
	//可能多个线程同时调用Start()
	lock_guard<std::mutex> lock(_mutex);

	if (!_isRun)
	{
		_isRun = true;

		if (onCreate)
			_onCreate = onCreate;
		if (onRun)
			_onRun = onRun;
		if (onDestory)
			_onDestory = onDestory;

		//&CELLThread::OnWork   成员函数的函数指针
		//mem_fun()	更安全的转化
		std::thread t(std::mem_fun(&CELLThread::OnWork), this);
		t.detach();
	}

}

void CELLThread::Close()
{
	lock_guard<std::mutex> lock(_mutex);
	if (_isRun)
	{
		_isRun = false;
		//等待工作函数OnWork()退出
		_sem.wait();
	}
}

void CELLThread::Exit()
{
	lock_guard<std::mutex> lock(_mutex);
	if (_isRun)
	{
		_isRun = false;
		//在线程内部退出，不需要阻塞等待工作函数执行完毕
	}
}

bool CELLThread::isRun()
{
	return _isRun;
}

void CELLThread::Sleep(time_t dt)
{
	chrono::milliseconds t(dt);	
	this_thread::sleep_for(t);
}

void CELLThread::OnWork()
{
	//EventCall的参数是CELLThread*，把this传进去
	//先注册，再运行，最后销毁
	if (_onCreate)
		_onCreate(this);

	if (_onRun)
		_onRun(this);

	if (_onDestory)
		_onDestory(this);

	_sem.wakeup();
}
