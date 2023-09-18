#include<iostream>
#include "EasyClient.h"
#include <thread>

using namespace std;

void GetCmd(EasyClient* client)
{
	while (true)
	{
		char chBuff[20] = { 0 };
		//cin >> chBuff;
		//scanf_s("%s", chBuff,sizeof(chBuff));
		strcpy(chBuff, "login");
		client->SendCmd(chBuff);
		
		if (strcmp(chBuff, "exit") == 0)
		{
			break;
		}
	}
	return;
}

int main()
{
	EasyClient client;
	client.InitSocket();
	client.Connect("192.168.1.104", 8080);

	//启动一个线程，接受用户输入
	std::thread t1(GetCmd, &client);
	t1.detach();

	while (client.isRun())
	{
		client.Run();
	}
	system("pause");
	return 0;
}