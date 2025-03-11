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
		TRACED(_T("Close failed!ret = %d"), ret);
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


//---------------以下是单元测试内容------------------
#include "gtest/gtest.h"
TEST(ServerTest, GetCpuCount)
{
	CTaskActuator task;
	TRACEI(_T("GetCpuCount = %d"), task.GetCpuCount());
	EXPECT_EQ(8, task.GetCpuCount());
}

int TaskTestFunc(int a)
{
	printf("%s(%d):<%s>a=%d\r\n",
		__FILE__, __LINE__, __FUNCTION__, a);
	return 0;
}
class TestTaskActuator
{
public:
	int TaskFunc(int a)
	{
		printf("%s(%d):<%s>a=%d\r\n",
			__FILE__, __LINE__, __FUNCTION__, a);
		return 0;
	}
};

TEST(ServerTest, TaskActuatorTest)
{
	CTaskActuator actuator;
	int ret = -1;
	ret = actuator.Run();
	ASSERT_EQ(0, ret);
	ret = actuator.AddTask(Task(TaskTestFunc, 100));
	ASSERT_EQ(0, ret);
	Sleep(100);
	TestTaskActuator t;
	ret = actuator.AddTask(Task(&TestTaskActuator::TaskFunc, &t, 100));
	ASSERT_EQ(0, ret);
	Sleep(100);
	actuator.Close();
}