// ELogServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "AsynchronousModel.h"
#include <shlwapi.h>
#include <iostream>
#include "LogServer.h"


#pragma comment(lib,"shlwapi.lib")

using namespace edoyun;
using PThread = std::shared_ptr<edoyun::CThread>;
class ELogServer :public CBusinessLayer
{
public:
	ELogServer(const std::string& path, const std::string& ip, unsigned short port)
		:m_server(new AsynServer(ip, port)), m_logger(CLogServer::getInstance())
	{
		if (PathFileExists(_T(".\\log")) == FALSE) {
			if (CreateDirectory(_T(".\\log"), NULL) == FALSE) {
				Eprintf(_T("create log dir failed!\r\n"));
				return;
			}
		}
	}
	virtual int Start(PBLayer& self) {
		m_self = self;
		return m_server->Start(m_server, m_self);
	};
	virtual void Close() {
		m_server->Close();
	}
public:
	//客户端连接上来了，不要阻塞
	virtual int ClientConnected(PAsynClient& client) { return 0; }
	//客户端收到数据了，不要阻塞
	virtual int ClientRecvData(PAsynClient& client, PCPacket& packet) {
		if (packet->Size() == 0) {//网络已经断开
			m_server->DelClient(client);
		}
		else {//来数据了
			m_logger->Trace(packet);
		}
		return 0;
	}
	virtual int ClientSendDone(PAsynClient& client, PCPacket& packet) {
		//日志服务器不会应答！！！
		return 0;
	}
private:
	PAsynServer m_server;
	CLogServer* m_logger;
	//自身
	PBLayer m_self;
};

int main()
{
	PBLayer logserver = PBLayer(new ELogServer("log", "127.0.0.1", 10000));
	logserver->Start(logserver);
	(void)getchar();
}

