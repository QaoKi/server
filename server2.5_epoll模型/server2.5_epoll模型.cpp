#include<iostream>
#include "EasyServer.h"
#include "Alloctor.h"
using namespace std;


int main()
{
	CELLLog::Instance().setLogPath("serverLog.txt", "w");
	EasyServer* ser = new EasyServer();
	ser->InitSocket();
	ser->Bind("192.168.1.104", 8080);
	ser->Listen(10);
	ser->Start(4);

	while (true)
	{
		char cmdBuf[256] = { 0 };
		scanf("%s", cmdBuf);
		if (strcmp("%s", "exit"))
		{
			ser->Close();
			CELLLog::Info("server exit\n");
			break;
		}
		else
		{
			CELLLog::Info("server Not supported cmd\n");
		}
	}

	//CellServer* task = new CellServer(1);
	//task->Start();
	//Sleep(1000);
	//task->Close();
	//Sleep(1000);
	//delete task;
	//while (1)
	//{
	//	Sleep(1000);
	//}

	system("pause");
	return 0;
}