#ifndef _CELL_LOG_H_
#define _CELL_LOG_H_
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include "CellTaskServer.h"
//运行日志创建为单例模式

/*
	使用步骤
	在使用之前，调用 CELLLog::Instance().setLogPath("serverLog.txt", "w");设置路径和文件打开方式
	CELLLog::Info("直接输出");
	int n = 10;
	const char* str = "n = %d";
	CELLLog::Info(str,n);

*/
class CELLLog
{
private:
	CELLLog();
	~CELLLog();

public:
	static CELLLog& Instance();

	void setLogPath(const char* logpath,const char* mode);

	//运行信息，输出字符串
	static void Info(const char* pStr);
	//可变参数
	template<typename ...Args>
	static void Info(const char* pformat, Args ... args)
	{

		auto pLog = &Instance();
		pLog->taskServer.AddTask([pLog, pformat, args...]() {
			if (pLog->_logFile)
			{
				time_t tTemp;
				time(&tTemp);
				struct tm* tNow = gmtime(&tTemp);
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", tNow->tm_year + 1900, tNow->tm_mon + 1, tNow->tm_mday, tNow->tm_hour + 8, tNow->tm_min, tNow->tm_sec);
				fprintf(pLog->_logFile, pformat, args...);
				//实时写入
				fflush(pLog->_logFile);
			}
			printf(pformat, args...);
		});	
	}
	//调试信息
	//警告信息
	//错误信息
private:
	FILE* _logFile;
	CellTaskServer taskServer;
};



#endif // _CELL_LOG_H_
