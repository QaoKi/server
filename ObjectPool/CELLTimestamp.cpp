#include "CELLTimestamp.h"



CELLTimestamp::CELLTimestamp()
{
	//初始化时记录当前时间
	update();
}


CELLTimestamp::~CELLTimestamp()
{
}

void CELLTimestamp::update()
{
	//high_resolution_clock::now() ：高精度的获取当前时间，精确到微妙
	m_begin = high_resolution_clock::now();
}

double CELLTimestamp::getElapsedSecond()
{
	return GetElapsedTimeInMicroSec() *  0.000001;
}

double CELLTimestamp::getElapsedTimeInMilliSec()
{
	return GetElapsedTimeInMicroSec() *  0.001;
}

long long CELLTimestamp::GetElapsedTimeInMicroSec()
{
	//用当前时间，减去之前记录的时间，就是这段时间的间隔，传化成微妙，用long long存储
	//microseconds是微妙，milliseconds是毫秒,seconds是妙
	return duration_cast<microseconds>(high_resolution_clock::now() - m_begin).count();
}
