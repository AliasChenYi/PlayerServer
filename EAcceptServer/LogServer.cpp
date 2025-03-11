#define _CRT_SECURE_NO_WARNINGS
#include "jemalloc/jemalloc.h"
#include <WS2tcpip.h>
#include "LogServer.h"
#include "ThreadPool.h"
#include <time.h>
#include <shlwapi.h>
#include <locale.h>


#pragma comment(lib,"shlwapi.lib")

using namespace edoyun;

#ifndef _UNICODE
#define Fopens fopen_s
#define Fprintf fprintf
#define Snprintf snprintf
#define Vsprintf _vsnprintf_s
#define GETSIZE(fmt,ap) _vsnprintf(NULL, 0, fmt, ap);
#define Eprintf printf
#define Strcpy strcpy
#define Setlocale setlocale
#else
#define Fopens _wfopen_s
#define Fprintf fwprintf
#define Snprintf _snwprintf
#define Vsprintf _vsnwprintf_s
#define GETSIZE(fmt,ap) _vsnwprintf(NULL, 0, fmt, ap);
#define Eprintf wprintf
#define Strcpy wcscpy
#define Setlocale _wsetlocale
#endif

CLogServer* CLogServer::m_instance = NULL;
CLogServer::CLoggerHelper CLogServer::m_helper;

CLogServer* CLogServer::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CLogServer();
	}
	return m_instance;
}

bool CLogServer::isValid()
{
	return m_pThread != nullptr;
}

void CLogServer::Trace(const LogInfo& info) {
	if (m_instance && m_instance->m_pThread) {
		m_instance->m_pThread->AddTask(
			Task(&CLogServer::WriteLog, m_instance, new LogInfo(info))
		);
	}
}

CLogServer::CLogServer()
	:m_pThread(new ThreadPool)
{
	Setlocale(LC_ALL, _T("chs"));
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(10000);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	connect(m_socket, (sockaddr*)&addr, sizeof(sockaddr_in));
	if (m_pThread->Start(1) != 0) {
		Eprintf(_T("invoke log server failed!\r\n"));
	}
}

CLogServer::~CLogServer()
{
	m_pThread->Close();
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

void CLogServer::releaseInstance()
{
	if (m_instance) {
		delete m_instance;
		m_instance = NULL;
	}
}

CharArray& CLogServer::AppendString(CharArray& str, const TCHAR* fmt, ...)
{
	CharArray buffer;
	va_list ap;
	va_start(ap, fmt);
	int count = GETSIZE(fmt, ap);
	if (count > 0) {//格式没有错误
		if (count >= (int)buffer.size()) {
			buffer.resize(count + 1);
		}
		memset(buffer.data(), 0, (count + 1) * sizeof(TCHAR));
		Vsprintf(buffer.data(), count + 1, _TRUNCATE, fmt, ap);
		if (buffer[count] == 0)buffer.resize(count);
		CharArray::reverse_iterator it = str.rbegin();
		while (it != str.rend() && (*it == 0)) it++;
		str.insert(it.base(), buffer.begin(), buffer.end());
	}
	va_end(ap);
	return str;
}

tstring CLogServer::GetTimeStr(const timeb& tmb)
{
	tstring result;
	result.resize(1024);
	tm* pTm = localtime(&tmb.time);
	int nSize = Snprintf((TCHAR*)result.c_str(), result.size(),
		_T("%04d-%02d-%02d %02d:%02d:%02d %03d"),
		pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
		pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
		tmb.millitm);
	result.resize(nSize);
	return result;
}

int CLogServer::WriteLog(LogInfo* info)
{
	if (info) {
		const TCHAR sLevel[][8] = {
			_T("NONE"),_T("INFO"),
			_T("DEBUG"),_T("WARNING"),
			_T("ERROR"),_T("FATAL"),_T("STDOUT")
		};
		if (info->level > EDOYUN_MAX)info->level = EDOYUN_MAX;
		if (info->level < EDOYUN_NONE)info->level = EDOYUN_NONE;
		CharArray head(1024);
		int nSize = Snprintf(head.data(), head.size(),
			_T("%s(%d):[%s][%s]<%d-%d %s> "),
			info->file, info->line, sLevel[info->level],
			GetTimeStr(info->tick).c_str(),
			info->pid, info->tid, info->function);
		head.resize(nSize);
		CharArray Text;
		if (info->israw) {
			MakeDump(Text, info->text.data(), info->text.size());
		}

		std::string out = (LPCTSTR)head.data();
		if (info->israw) {
			out += "\r\n";
			if (Text.size() > 0)
				out += Text.data();
		}
		else {
			if (info->text.size() > 0)
				out += info->text.data();
		}
		out += "\r\n";
		delete info;
		send(m_socket, out.c_str(), out.size(), 0);
	}
	return 0;
}

CharArray& CLogServer::MakeDump(CharArray& vText, const void* pData, size_t nSize)
{
	tstring out;
	size_t i = 0;
	for (; i < nSize; i++) {
		AppendString(vText, _T("%02X "), 0xFF & *((PBYTE)pData + i));
		if (0 == (i + 1) % 16) {
			AppendString(vText, _T("\t; "));
			for (size_t j = i - 15; j <= i; j++) {
				if (((PBYTE)pData)[j] > 31)
					AppendString(vText, _T("%c"), ((PBYTE)pData)[j]);
				else
					AppendString(vText, _T("."));
			}
			AppendString(vText, _T("\r\n"));
		}
	}
	size_t k = i % 16;
	if (k != 0) {
		for (size_t j = 0; j < 16 - k; j++) {
			AppendString(vText, _T("   "));
		}
		AppendString(vText, _T("\t; "));
		for (size_t j = i - k; j <= i; j++) {
			if (((PBYTE)pData)[j] > 31)
				AppendString(vText, _T("%c"), ((PBYTE)pData)[j]);
			else
				AppendString(vText, _T("."));
		}
		AppendString(vText, _T("\r\n"));
	}
	vText.insert(vText.end(), _T('\0'));
	return vText;
}


DWORD LogInfo::pid = GetCurrentProcessId();

LogInfo::LogInfo(
	const TCHAR* sFile,
	int nLine,
	int nLevel,
	const TCHAR* sFunction,
	DWORD nTid,
	timeb* pTick,
	const TCHAR* fmt, ...)
{
	Strcpy(file, sFile);
	line = nLine;
	level = nLevel;
	memcpy(&tick, pTick, sizeof(tick));
	tid = nTid;
	Strcpy(function, sFunction);
	va_list ap;
	va_start(ap, fmt);
	int count = GETSIZE(fmt, ap);
	if (count > 0) {//格式没有错误
		text.resize(count + 1);
		memset(text.data(), 0, (count + 1) * sizeof(TCHAR));
		Vsprintf(text.data(), count + 1, _TRUNCATE, fmt, ap);
	}
	va_end(ap);
	israw = false;
}

LogInfo::LogInfo(
	const TCHAR* sFile,
	int nLine, int nLevel,
	const TCHAR* sFunction,
	const TCHAR* fmt, ...)
{
	Strcpy(file, sFile);
	line = nLine;
	level = nLevel;
	ftime(&tick);
	tid = GetThreadId(GetCurrentThread());
	Strcpy(function, sFunction);
	va_list ap;
	va_start(ap, fmt);
	int count = GETSIZE(fmt, ap);
	if (count > 0) {//格式没有错误
		text.resize(count + 1);
		memset(text.data(), 0, (count + 1) * sizeof(TCHAR));
		Vsprintf(text.data(), count + 1, _TRUNCATE, fmt, ap);
	}
	va_end(ap);
	israw = false;
}

LogInfo::LogInfo(
	const TCHAR* sFile, const TCHAR* sFunction,
	int nLine, int nLevel,
	const void* pData, size_t nSize)
{
	Strcpy(file, sFile);
	line = nLine;
	level = nLevel;
	ftime(&tick);
	tid = GetThreadId(GetCurrentThread());
	Strcpy(function, sFunction);
	text.resize(nSize);
	memcpy(text.data(), pData, nSize);
	israw = true;
}

LogInfo::~LogInfo()
{
	text.clear();
}

LogInfo::LogInfo(const LogInfo& info)
{
	Strcpy(file, info.file);
	line = info.line;
	level = info.level;
	memcpy(&tick, &info.tick, sizeof(timeb));
	tid = info.tid;
	Strcpy(function, info.function);
	text = info.text;
	israw = info.israw;
}

LogInfo& LogInfo::operator=(const LogInfo& info)
{
	if (this != &info) {
		Strcpy(file, info.file);
		line = info.line;
		level = info.level;
		memcpy(&tick, &info.tick, sizeof(timeb));
		tid = info.tid;
		Strcpy(function, info.function);
		text = info.text;
		israw = info.israw;
	}
	return *this;
}

void* __CRTDECL operator new(size_t size)
{
	return je_malloc(size);
}

void __CRTDECL operator delete(void* pointer)
{
	je_free(pointer);
}

void* __CRTDECL operator new[](size_t size)
{
	return je_malloc(size);
}

void __CRTDECL operator delete[](void* pointer)
{
	je_free(pointer);
}

