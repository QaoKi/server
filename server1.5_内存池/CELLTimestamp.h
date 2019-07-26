#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#include<chrono>
using namespace std::chrono;
class CELLTimestamp
{
public:
	CELLTimestamp();
	~CELLTimestamp();

public:
	//记录当前时间
	void update();

	//获取妙
	double getElapsedSecond();

	//获取毫秒
	double getElapsedTimeInMilliSec();

	//获取微妙
	long long GetElapsedTimeInMicroSec();

private:
	//创建一个时间点
	time_point<high_resolution_clock> m_begin;

};

#endif
