#include <iostream>
#include "Alloctor.h"
#include <thread>
#include <mutex>
#include "CELLTimestamp.h"
#include <memory>
using namespace std;


class A
{
public:
	A() 
	{ 
		cout << "无参构造函数" << endl;
		m_a = 1; 
	};
	A(int a)
	{
		cout << "有参构造函数" << endl;
		m_a = a;
	}
	~A()
	{
	
	};

private:
	int m_a;
};



int main()
{

	A* a = new A(2);

	system("pause");
	return 0;
}

//mutex m;
//const int tCount = 8;
//const int mCount = 1000000;
//const int nCount = mCount / tCount;
//
//void workFun(int index)
//{
//	char* data[nCount];
//	for (int n = 0; n < nCount; n++)
//	{
//		data[n] = new char[ rand()%1024 + 1];
//	}
//
//	for (int n = 0; n < nCount; n++)
//	{
//		delete[] data[n];
//	}
//
//}
//int main()
//{
//	thread t[tCount];
//	for (int n = 0; n < tCount; n++)
//	{
//		t[n] = thread(workFun, n);
//	}
//	CELLTimestamp tTime;
//	for (int n = 0; n < tCount; n++)
//	{
//		t[n].join();
//	}
//	cout << tTime.getElapsedTimeInMilliSec() << endl;
//	cout << "Hello,main thread." << endl;
//	system("pause");
//	return 0;
//}