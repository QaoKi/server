#ifndef _CELLSEMAPHORE_H_
#define _CELLSEMAPHORE_H_
#include <chrono>
#include <thread>
#include <mutex>
//条件变量
#include <condition_variable>

using namespace std;
class CELLSemaphore
{
public:
	void wait();
	void wakeup();

private:
	//阻塞等待—条件变量
	std::condition_variable _cv;
	std::mutex _mutex;
	//等待计数
	int _wait = 0;
	//唤醒计数
	int _wakeup = 0;
};
#endif // _CELLSEMAPHORE_H_
