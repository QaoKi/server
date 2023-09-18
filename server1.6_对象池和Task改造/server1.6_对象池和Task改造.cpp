#include<iostream>
#include "EasyServer.h"
#include "Alloctor.h"

using namespace std;


int main()
{
	EasyServer* ser = new EasyServer();
	ser->InitSocket();
	ser->Bind(NULL, 8080);
	ser->Listen(10);
	ser->Start(4);


	system("pause");
	return 0;
}