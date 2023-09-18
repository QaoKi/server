#include "CELLSemaphore.h"


void CELLSemaphore::wait()
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (--_wait < 0)
	{
		//阻塞等待
		/*
			wait()的第二个参数设置一个Lambda表达式
			添加一个唤醒的条件，只有当_wakeup > 0的时候才能被唤醒
			防止伪唤醒，只有真正的wakeup()才能唤醒
		*/
	
		_cv.wait(lock, [this]() -> bool {
			return _wakeup > 0;
		});
		--_wakeup;
	}
		
}

void CELLSemaphore::wakeup()
{
	std::unique_lock<std::mutex> lock(_mutex);
	//当有两个线程t1和t2同时调用了wake()，此时_wait是-2，所以要判断 ++_wait<=0
	if (++_wait <= 0)
	{
		++_wakeup;
		_cv.notify_one();
	}
}
