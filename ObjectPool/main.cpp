#include <iostream>
#include "CELLObjectPool.hpp"
#include "CELLTimestamp.h"
#include <memory>
using namespace std;

class ClassA : public ObjectPoolBase<ClassA,5>
{
public:
	ClassA(int n)
	{
		m_num = n;
		printf("ClassA\n");
	}
	~ClassA()
	{
		printf("~ClassA\n");
	}

private:
	int m_num;
};

mutex m;
const int tCount = 4;
const int mCount = 8;
const int nCount = mCount / tCount;

void workFun(int index)
{
	ClassA* data[nCount];
	for (int n = 0; n < nCount; n++)
	{
		data[n] = ClassA::createObject(n);
	}

	for (int n = 0; n < nCount; n++)
	{
		ClassA::destroyObject(data[n]);
	}

}

int main()
{
	{
		/*当使用智能指针时，make_shared<ClassA>(1) 的方式，
			不会调用对象池去申请ClassA的内存
			需要使用shared_ptr<ClassA> s2(new ClassA(1));的形式
		
		*/
		shared_ptr<ClassA> s1 = make_shared<ClassA>(1);
		shared_ptr<ClassA> s2(new ClassA(1));
	}

	//thread t[tCount];
	//for (int n = 0; n < tCount; n++)
	//{
	//	t[n] = thread(workFun, n);
	//}
	//CELLTimestamp tTime;
	//for (int n = 0; n < tCount; n++)
	//{
	//	t[n].join();
	//}
	//cout << tTime.getElapsedTimeInMilliSec() << endl;
	//cout << "Hello,main thread." << endl;

	system("pause");
	return 0;
}