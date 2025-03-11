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
#define Fopens fopen_s
#define Fprintf fprintf
#ifndef Snprintf
#define Snprintf snprintf
#endif
#define Vsprintf _vsnprintf_s
#define GETSIZE(fmt,ap) _vsnprintf(NULL, 0, fmt, ap);
#define Eprintf printf
#define Strcpy strcpy
#define Setlocale setlocale
#else
using  tstring = std::wstring;
using tsstream = std::wstringstream;
#define _FILE_ __FILEW__
#define _FUNCTION_ __FUNCTIONW__
#define Fopens _wfopen_s
#define Fprintf fwprintf
#ifndef Snprintf
#define Snprintf _snwprintf
#endif
#define Vsprintf _vsnwprintf_s
#define GETSIZE(fmt,ap) _vsnwprintf(NULL, 0, fmt, ap);
#define Eprintf wprintf
#define Strcpy wcscpy
#define Setlocale _wsetlocale
#endif

class CPacket;
using PCPacket = std::shared_ptr<CPacket>;

class CLogServer
{
public:
	CLogServer& operator=(const CLogServer&) = delete;
	CLogServer(const CLogServer&) = delete;
	static CLogServer* getInstance();
	bool isValid();
	static void Trace(const PCPacket& info);
private:
	CLogServer();
	~CLogServer();
	static void releaseInstance();
	static CharArray& AppendString(CharArray& str, const TCHAR* fmt, ...);
	tstring GetTimeStr(const timeb& tmb);
	int WriteLog(PCPacket packet);
	CharArray& MakeDump(CharArray& vText, const void* pData, size_t size);
private:
	std::shared_ptr<edoyun::ThreadPool> m_pThread;
	static CLogServer* m_instance;
	FILE* m_pFile[2];
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

