#include "TaskActuator.h"
#include "LogServer.h"
using namespace edoyun;

CTaskActuator::CTaskActuator()
{
	m_nCpuCount = GetCpuCount();
}

CTaskActuator::~CTaskActuator()
{
	Close();
}

int CTaskActuator::AddTask(const Task& task)
{
	return ThreadPool::AddTask(task);
}

int CTaskActuator::Run()
{
	return ThreadPool::Start(m_nCpuCount);
}

void CTaskActuator::Close()
{
	int ret = ThreadPool::Close();
	if (ret) {
		printf(_T("Close failed!ret = %d"), ret);
	}
}

DWORD CTaskActuator::CountSetBits(ULONG_PTR bitMask)
{//计算bitMask中bit为1的个数
	ULONG_PTR count;
	for (count = 0; bitMask; count++)
		bitMask &= (bitMask - 1);
	return count;
}

using SLPI = SYSTEM_LOGICAL_PROCESSOR_INFORMATION;
DWORD CTaskActuator::GetCpuCount()
{
	DWORD nLength = 0;
	BOOL ret = FALSE;
	if (GetLogicalProcessorInformation(NULL, &nLength) == FALSE)
	{
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
		{
			int result = 0;
			int Count = nLength / sizeof(SLPI);
			SLPI* pSLPI = new SLPI[Count];
			GetLogicalProcessorInformation(pSLPI, &nLength);
			for (int i = 0; i < Count; i++) {
				if (pSLPI[i].Relationship == RelationProcessorCore) {
					result += CountSetBits(pSLPI[i].ProcessorMask);
				}
			}
			delete[]pSLPI;
			return result;
		}
	}
	return 0;
}
