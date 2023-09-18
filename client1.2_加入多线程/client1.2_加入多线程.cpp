#include<iostream>
#include "EasyClient.h"
#include <thread>

using namespace std;

//客户端数量
const int cCount = 1;
//发送线程数量
const int tCount = 1;
//客户端数组
EasyClient* client[cCount];


void sendThread(int id)
{
	//4个线程，id 1~4,1000个客户端，每个线程负责250个
	int c = cCount / tCount;
	int begin = (id - 1)*c;
	int end = id * c;

	for (int i = begin; i < end; i++)
	{
		client[i] = new EasyClient();
	}

	for (int i = begin; i < end; i++)
	{
		client[i]->Connect("192.168.1.106", 8081);
		client[i]->Start();
		//printf("thread<%d>,Connect=%d\n", id, i);
	}

	Login login[10];
	for (int i = 0; i < 10; i++)
	{
		strcpy(login[i].userName, "lyd");
		strcpy(login[i].PassWord, "lydmm");
	}

	const int nLen = sizeof(login);
	bool isSend = true;
	while (true)
	{
		for (int i = begin; i < end; i++)
		{
			//if (isSend)
			//{
			client[i]->SendCmd("login");
			//client[i]->SendData(login, nLen);
			//client[i]->Run();
			//isSend = false;
			//}
		}
		chrono::milliseconds t(1000);
		this_thread::sleep_for(t);
	}

	for (int n = begin; n < end; n++)
	{
		client[n]->Close();
	}

}
int main()
{

	for (int i = 0; i < tCount; i++)
	{
		std::thread t1(sendThread,i + 1);	//把是第几个线程传过去
		t1.detach();	//detach()是让线程和主线程分离，主线程不会等待线程的结束
		//t1.join();	//join()是线程执行完以后，主线程才会继续向下执行，如果创建了很多线程，这些线程之间是并行的
	}

	while (true)
	{
		chrono::milliseconds t(100);
		this_thread::sleep_for(t);
	}
	system("pause");
	return 0;
}