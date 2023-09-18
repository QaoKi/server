#include<iostream>
#include "EasyServer.h"

using namespace std;


int main()
{
	EasyServer ser;
	ser.InitSocket();
	ser.Bind("127.0.0.1", 8080);
	ser.Listen(10);
	while (ser.isRun())
	{
		ser.Run();
	}


	system("pause");
	return 0;
}