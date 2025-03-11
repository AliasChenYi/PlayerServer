#include "Thread.h"
#include <thread>
#include "LogServer.h"


using namespace edoyun;

edoyun::CThread::CThread() :m_pFunction(NULL)
{
	m_bValid = false;
	m_hThread = CreateThread(
		NULL, 0, CThread::ThreadEntry,
		this, CREATE_SUSPENDED, &m_nThreadID);
}

edoyun::CThread::~CThread()
{
	Stop();
	CFunctionBase* func = m_pFunction;
	m_pFunction = NULL;
	delete func;
}

int edoyun::CThread::Start()
{
	if (m_pFunction == NULL) {
		return THREAD_IS_INVALID;
	}
	if ((m_hThread != NULL) && (m_bValid == false)) {
		DWORD ret = ResumeThread(m_hThread);
		if (ret == -1) {
			return ret;
		}
	}
	return THREAD_OK;
}

int edoyun::CThread::Pause()
{
	if ((m_hThread != NULL) && (m_bValid == true)) {
		DWORD ret = SuspendThread(m_hThread);
		if (ret == -1) {
			return THREAD_PAUSE_ERROR;
		}
	}
	else {//线程无效
		return THREAD_IS_INVALID;
	}
	return THREAD_OK;
}

int edoyun::CThread::Stop()
{
	m_bValid = false;//设计标志
	if (m_hThread != NULL) {
		//析构的时候没有结束，则强制结束线程
		DWORD ret = WaitForSingleObject(m_hThread, 10);
		if (ret == WAIT_TIMEOUT) {
			//尽量不要走到这一步来，否则在线程中new出来的内存将失控！！！
			TerminateThread(m_hThread, THREAD_IS_BUSY);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	return THREAD_OK;
}

int edoyun::CThread::Restart()
{
	if (m_pFunction == NULL) {//自定义函数有效
		return THREAD_IS_INVALID;
	}
	if (m_bValid != false || (m_hThread != NULL)) {
		//线程仍然在运行
		return THREAD_IS_BUSY;
	}
	m_hThread = CreateThread(
		NULL, 0, CThread::ThreadEntry,
		this, 0, &m_nThreadID);
	return THREAD_OK;
}

bool edoyun::CThread::isValid() const
{
	return m_bValid && (m_pFunction != NULL);
}

DWORD WINAPI edoyun::CThread::ThreadEntry(LPVOID lpParam)
{
	CThread* thiz = (CThread*)lpParam;
	thiz->EnterThread();
	ExitThread(THREAD_OK);
	return THREAD_OK;
}

void edoyun::CThread::EnterThread()
{
	int ret = 0;
	m_bValid = true;
	do {
		while (m_pFunction == NULL) {
			if (m_bValid == false)return;
			Sleep(1);
		}
		CFunctionBase* func = m_pFunction;
		if (func == NULL)break;
		ret = (*func)();
		TRACED(_T("ret=%d"), ret);
	} while (ret == THREAD_AGAIN);
	m_bValid = false;
}

//---------------以下是单元测试内容------------------
#include "gtest/gtest.h"
int Func(int a, const char* b) {
	Sleep(50);
	TRACED(_T("a=%d b=%s tick=%d"), a, b, GetTickCount());
	return 0;
}
class TestThread {
public:
	int Func(int a, double c) {
		static int ret = 1;
		TRACED(_T("a=%d c=%f ret = %d"), a, c, ret);
		Sleep(1);
		return ret--;
	}
};

TEST(ServerTest, ThreadTest)
{
	int ret = 0;
	CThread thread01(&Func, 10, "hello");
	CThread thread02;
	TestThread tt;
	EXPECT_EQ(THREAD_IS_INVALID, thread02.Start());
	ret = thread02.SetThreadFunc(&TestThread::Func, &tt, 1, 100.0);
	ASSERT_EQ(edoyun::THREAD_OK, ret);
	ret = thread01.Start();
	TRACED(_T("tick=%d\r\n"), GetTickCount());
	Sleep(10);
	bool value = thread01.isValid();
	TRACED(_T("value=%d tick=%d\r\n"), value, GetTickCount());
	EXPECT_EQ(value, true);
	ASSERT_EQ(edoyun::THREAD_OK, ret);
	ret = thread02.Start();
	ASSERT_EQ(edoyun::THREAD_OK, ret);
	Sleep(200);
	ret = thread01.Stop();
	EXPECT_EQ(edoyun::THREAD_OK, ret);
	ret = thread02.Stop();
	EXPECT_EQ(edoyun::THREAD_OK, ret);
	ret = thread02.Restart();
	EXPECT_EQ(edoyun::THREAD_OK, ret);
	Sleep(20);
	ret = thread02.Stop();
	EXPECT_EQ(edoyun::THREAD_OK, ret);
}