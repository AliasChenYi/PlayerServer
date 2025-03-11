#pragma once

void* __CRTDECL operator new(size_t size);
void __CRTDECL operator delete(void* pointer);
void* __CRTDECL operator new[](size_t size);
void __CRTDECL operator delete[](void* pointer);

#include <Windows.h>
#include <vector>
#include <memory>
#include <sys/timeb.h>
#include <string>
#include <sstream>
#include <tchar.h>

namespace edoyun {
	class ThreadPool;
}

enum {
	EDOYUN_NONE,
	EDOYUN_INFO,
	EDOYUN_DEBUG,
	EDOYUN_WARNING,
	EDOYUN_ERROR,
	EDOYUN_FATAL,
	EDOYUN_MAX
};

using CharArray = std::vector<TCHAR>;

#ifndef _UNICODE
using  tstring = std::string;
using tsstream = std::stringstream;
#define _FILE_ __FILE__
#define _FUNCTION_ __FUNCTION__
#else
using  tstring = std::wstring;
using tsstream = std::wstringstream;
#define _FILE_ __FILEW__
#define _FUNCTION_ __FUNCTIONW__
#endif


class LogInfo {
public:
	TCHAR file[MAX_PATH];
	int line;
	int level;
	timeb tick;
	static DWORD pid;
	DWORD tid;
	TCHAR function[MAX_PATH];
	CharArray text;
	bool israw;//true表示这是一个dump数据，false表示这个一个日志数据
public:
	LogInfo(const TCHAR* file,
		int line,
		int level,
		const TCHAR* function,
		DWORD tid,
		timeb* pTick,
		const TCHAR* fmt, ...);
	LogInfo(const TCHAR* file,
		int line,
		int level,
		const TCHAR* function,
		const TCHAR* fmt, ...);
	LogInfo(const TCHAR* file,
		const TCHAR* function,
		int line,
		int level,
		const void* pData, size_t nSize);
	~LogInfo();
	LogInfo(const LogInfo& info);
	LogInfo& operator=(const LogInfo& info);
};

class CLogServer
{
public:
	CLogServer& operator=(const CLogServer&) = delete;
	CLogServer(const CLogServer&) = delete;
	static CLogServer* getInstance();
	bool isValid();
	static void Trace(const LogInfo& info);
private:
	CLogServer();
	~CLogServer();
	static void releaseInstance();
	static CharArray& AppendString(CharArray& str, const TCHAR* fmt, ...);
	tstring GetTimeStr(const timeb& tmb);
	int WriteLog(LogInfo* info);
	CharArray& MakeDump(CharArray& vText, const void* pData, size_t size);
private:
	std::shared_ptr<edoyun::ThreadPool> m_pThread;
	static CLogServer* m_instance;
	SOCKET m_socket;
private:
	static class CLoggerHelper {
	public:
		CLoggerHelper() {
			CLogServer::getInstance();
		}
		~CLoggerHelper() {
			CLogServer::releaseInstance();
		}
	} m_helper;
};


#define TRACE(...)	CLogServer::getInstance()->Trace(LogInfo(_FILE_, __LINE__, EDOYUN_NONE, _FUNCTION_, __VA_ARGS__))

#define TRACEI(...) CLogServer::getInstance()->Trace(LogInfo(_FILE_, __LINE__, EDOYUN_INFO, _FUNCTION_, __VA_ARGS__))

#define TRACED(...) CLogServer::getInstance()->Trace(LogInfo(_FILE_, __LINE__, EDOYUN_DEBUG, _FUNCTION_, __VA_ARGS__))

#define TRACEW(...) CLogServer::getInstance()->Trace(LogInfo(_FILE_, __LINE__, EDOYUN_WARNING, _FUNCTION_, __VA_ARGS__))

#define TRACEE(...) CLogServer::getInstance()->Trace(LogInfo(_FILE_, __LINE__, EDOYUN_ERROR, _FUNCTION_, __VA_ARGS__))

#define TRACEF(...) CLogServer::getInstance()->Trace(LogInfo(_FILE_, __LINE__, EDOYUN_FATAL, _FUNCTION_, __VA_ARGS__))

#define DUMP(data, size) CLogServer::getInstance()->Trace(LogInfo(_FILE_, _FUNCTION_, __LINE__, EDOYUN_NONE, (const void*)data, (size_t)size))

#define DUMPI(data, size) CLogServer::getInstance()->Trace(LogInfo(_FILE_, _FUNCTION_, __LINE__, EDOYUN_INFO, (const void*)data, (size_t)size))

#define DUMPD(data, size) CLogServer::getInstance()->Trace(LogInfo(_FILE_, _FUNCTION_, __LINE__, EDOYUN_DEBUG, (const void*)data, (size_t)size))

#define DUMPW(data, size) CLogServer::getInstance()->Trace(LogInfo(_FILE_, _FUNCTION_, __LINE__, EDOYUN_WARNING, (const void*)data, (size_t)size))

#define DUMPE(data, size) CLogServer::getInstance()->Trace(LogInfo(_FILE_, _FUNCTION_, __LINE__, EDOYUN_ERROR, (const void*)data, (size_t)size))

#define DUMPF(data, size) CLogServer::getInstance()->Trace(LogInfo(_FILE_, _FUNCTION_, __LINE__, EDOYUN_FATAL, (const void*)data, (size_t)size))

class CStreamLogger {
public:
	CStreamLogger(const TCHAR* file,
		int line,
		int level,
		const TCHAR* function)
	{
		m_file = file;
		m_line = line;
		m_level = level;
		m_function = function;
		ftime(&m_tick);
		m_tid = GetThreadId(GetCurrentThread());
	}
	~CStreamLogger() {
		CLogServer::getInstance()->Trace(
			LogInfo(m_file, m_line, m_level, m_function, m_tid, &m_tick, _T("%s"), m_data.c_str())
		);
	}
	template<typename T>
	CStreamLogger& operator<<(const T& data) {
		tsstream stream;
		stream << data;
		m_data += stream.str();
		return *this;
	}

private:
	const TCHAR* m_file;
	int m_line;
	int m_level;
	const TCHAR* m_function;
	timeb m_tick;
	DWORD m_tid;
	tstring m_data;
};

#define ELOG CStreamLogger(_FILE_, __LINE__, EDOYUN_NONE, _FUNCTION_)
#define ELOGI CStreamLogger(_FILE_, __LINE__, EDOYUN_INFO, _FUNCTION_)
#define ELOGD CStreamLogger(_FILE_, __LINE__, EDOYUN_DEBUG, _FUNCTION_)
#define ELOGW CStreamLogger(_FILE_, __LINE__, EDOYUN_WARNING, _FUNCTION_)
#define ELOGE CStreamLogger(_FILE_, __LINE__, EDOYUN_ERROR, _FUNCTION_)
#define ELOGF CStreamLogger(_FILE_, __LINE__, EDOYUN_FATAL, _FUNCTION_)