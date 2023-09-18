#ifndef _CellTask_H
#define _CellTask_H

#include <list>
#include <mutex>
#include <thread>
#include "ClientSocket.h"

//这个不再使用
using namespace std;

class CellTask;
typedef std::shared_ptr<CellTask> CellTaskPtr;
//任务类型的基类
class CellTask
{
public:
	CellTask();
	virtual ~CellTask();

	virtual void doTask();
};

#endif
