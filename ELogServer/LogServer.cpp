#define _CRT_SECURE_NO_WARNINGS
#include "jemalloc/jemalloc.h"
#include "AsynchronousModel.h"
#include "LogServer.h"
#include "ThreadPool.h"
#include <time.h>
#include <shlwapi.h>
#include <locale.h>


#pragma comment(lib,"shlwapi.lib")

using namespace edoyun;

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

void CLogServer::Trace(const PCPacket& info) {
	if (m_instance && m_instance->m_pThread) {
		m_instance->m_pThread->AddTask(
			Task(&CLogServer::WriteLog, m_instance, info)
		);
	}
}

CLogServer::CLogServer()
	:m_pThread(new ThreadPool)
{
	Setlocale(LC_ALL, _T("chs"));
	memset(m_pFile, 0, sizeof(m_pFile));
	m_pFile[1] = stdout;
	if (PathFileExists(_T(".\\log")) == FALSE) {
		if (CreateDirectory(_T(".\\log"), NULL) == FALSE) {
			Eprintf(_T("create log dir failed!\r\n"));
		}
	}
	timeb tmb;
	ftime(&tmb);
	tstring sTime = GetTimeStr(tmb);
	for (unsigned j = 0; j < sTime.size(); j++) {
		if (sTime[j] == _T(':')) {
			sTime[j] = _T('-');
			continue;
		}
		if (sTime[j] == _T(' ')) {
			sTime[j] = _T('_');
		}
	}

	tstring sFile = _T("log\\log-") + sTime + _T(".log");
	tstring sMode = _T("a+");
	if (PathFileExists(sFile.c_str()) == FALSE) {
		sMode = _T("w+");
	}
	errno_t ret = Fopens(m_pFile, sFile.c_str(), sMode.c_str());
	if (ret != 0)
		Eprintf(_T("open file:%s failed, ret=%d\r\n"), sFile.c_str(), ret);
	if (m_pThread->Start(1) != 0) {
		Eprintf(_T("invoke log server failed!\r\n"));
	}
}

CLogServer::~CLogServer()
{
	m_pThread->Close();
	if (m_pFile[0] != NULL) {
		fclose(m_pFile[0]);
		m_pFile[0] = NULL;
	}
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

int CLogServer::WriteLog(PCPacket packet)
{
	//调试用
	OutputDebugString((LPCTSTR)*packet);
	//文件输出
	FILE* pFile = m_pFile[0];
	if (pFile != NULL) {
		fwrite(*packet, 1, packet->Size(), pFile);
		fflush(pFile);
	}
	//标准输出
	pFile = m_pFile[1];
	if (pFile != NULL) {
		fwrite(*packet, 1, packet->Size(), pFile);
		fflush(pFile);
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



