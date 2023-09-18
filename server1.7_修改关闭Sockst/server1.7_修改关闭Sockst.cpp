#include<iostream>
#include "EasyServer.h"
#include "Alloctor.h"

using namespace std;


int main()
{
	//EasyServer* ser = new EasyServer();
	//ser->InitSocket();
	//ser->Bind(NULL, 8080);
	//ser->Listen(10);
	//ser->Start(4);
	//ser->Close();

	CellServer* task = new CellServer(1);
	task->Start();
	Sleep(1000);
	task->Close();
	Sleep(1000);
	delete task;
	while (1)
	{
		Sleep(1000);
	}

	system("pause");
	return 0;
}