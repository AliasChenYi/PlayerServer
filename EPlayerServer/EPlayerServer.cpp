// EPlayerServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "EdoyunServer.h"
#include <iostream>
/*
#include <gtest/gtest.h>
int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	(void)getchar();
	return ret;
}
*/
int main(int argc, char* argv[])
{
	//(void)getchar();
	PBLayer server(new EdoyunServer("127.0.0.1", 10200));
	server->Start(server);
	(void)getchar();
	return 0;
}