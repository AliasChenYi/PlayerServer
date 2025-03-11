#pragma once
#include "ThreadPool.h"

class CTaskActuator :
	protected edoyun::ThreadPool
{
public:
	CTaskActuator();
	virtual ~CTaskActuator();
	int AddTask(const edoyun::Task& task);
	int Run();
	void Close();
	DWORD GetCpuCount();
	DWORD CountSetBits(ULONG_PTR bitMask);
private:
	DWORD m_nCpuCount;
};

