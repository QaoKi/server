#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

/*
	定义消息结构
*/

enum CMD
{
	CMD_LOGIN = 0,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_EXIT,
	CMD_NEW_USER_JOIN,
	CMD_HEART_C2S,
	CMD_HEART_S2C,
	CMD_ERROR
};

struct netmsg_DataHeader
{
	netmsg_DataHeader()
	{
		dataLength = sizeof(netmsg_DataHeader);
		cmd = CMD_ERROR;
	}
	unsigned short dataLength;
	short cmd;
};

struct Login :public netmsg_DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
	char data[32];
};

struct LoginResult : public netmsg_DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char data[92];
};

struct Logout : public netmsg_DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult : public netmsg_DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct ExitConnect : public netmsg_DataHeader
{
	ExitConnect()
	{
		dataLength = sizeof(ExitConnect);
		cmd = CMD_EXIT;
		result = 0;
	}
	int result;
};


struct NewUserJoin : public netmsg_DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		scok = 0;
	}
	int scok;
};

struct Heart_C2S :public netmsg_DataHeader
{
	Heart_C2S()
	{
		dataLength = sizeof(Heart_C2S);
		cmd = CMD_HEART_C2S;
	}
};

struct Heart_S2C :public netmsg_DataHeader
{
	Heart_S2C()
	{
		dataLength = sizeof(Heart_S2C);
		cmd = CMD_HEART_S2C;
	}
};
#endif