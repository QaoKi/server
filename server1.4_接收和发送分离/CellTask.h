#ifndef _CellTask_H
#define _CellTask_H

#include <list>
#include <mutex>
#include <thread>
#include "ClientSocket.h"

using namespace std;

//任务类型的基类
class CellTask
{
public:
	CellTask();
	virtual ~CellTask();

	virtual void doTask();
};

#endif
