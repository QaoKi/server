#include "CELLLog.h"



CELLLog::CELLLog()
{
	taskServer.Start();
}


CELLLog::~CELLLog()
{
	taskServer.Close();
	if (_logFile)
	{
		fclose(_logFile);
		_logFile = nullptr;
	}
}

CELLLog& CELLLog::Instance()
{
	static CELLLog sLog;
	return sLog;
}

void CELLLog::setLogPath(const char* logpath, const char* mode)
{
	if (_logFile)
	{
		fclose(_logFile);
		_logFile = nullptr;
	}
	_logFile = fopen(logpath, mode);
	if (_logFile)
	{
		Info("CELLLog::setLogPath sucess,<path = \"%s\">,<mode = \"%s\">\n", logpath, mode);
	}
	else
	{
		Info("CELLLog::setLogPath failed,<path =\"%s\">,<mode = \"%s\">\n", logpath, mode);
	}
}

void CELLLog::Info(const char* pStr)
{
	auto pLog = &Instance();
	pLog->taskServer.AddTask([=]() {
		if (pLog->_logFile)
		{
			time_t tTemp;
			time(&tTemp);
			struct tm* tNow = gmtime(&tTemp);
			fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", tNow->tm_year + 1900, tNow->tm_mon + 1, tNow->tm_mday, tNow->tm_hour + 8, tNow->tm_min, tNow->tm_sec);
			fprintf(pLog->_logFile, "%s", pStr);
			//实时写入，将流中数据写入,如果没有这句，只有当程序退出才会写入
			fflush(pLog->_logFile);
		}	
	});

	printf("%s", pStr);
}

template<typename ...Args>
void CELLLog::Info(const char* pformat, Args ...args)
{
	auto pLog = &Instance();
	pLog->taskServer.AddTask([=]() {
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
	});

	printf(pformat, args...);
}