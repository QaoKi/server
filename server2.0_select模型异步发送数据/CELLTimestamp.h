#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#include<chrono>
using namespace std::chrono;

//得到一段时间间隔
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
	time_point<high_resolution_clock> _begin;

};

//得到当前时间
class CELLTime
{

public:
	//得到当前时间，转化为毫秒
	static time_t getNowTimeInMilliSec();

};


#endif
