// EAcceptServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "AsynchronousModel.h"
#include <iostream>
#include "LogServer.h"


class EAcceptServer
	:public CBusinessLayer
{
public:
	EAcceptServer(const std::string& ip, unsigned short port)
		:m_server(new AsynServer(ip, port))
	{}
public:
	virtual int Start(PBLayer& self) {
		m_self = self;
		return m_server->Start(m_server, m_self);
	};
	virtual void Close() {
		m_server->Close();
	}
public:
	//客户端连接上来了，不要阻塞
	virtual int ClientConnected(PAsynClient& client) {
		TRACEI(_T("客户端连接完成！"));
		if (client->Pair() == nullptr) {
			PAsynClient pair = PAsynClient(new AsynClient());
			TRACED(_T("client = %d pair = %d"), (SOCKET)*client, (SOCKET)*pair);
			pair->SetPair(client);
			client->SetPair(pair);
			pair->Connect("127.0.0.1", 10200, m_server, pair);
		}
		else {
			//内网连接成功
		}
		return 0;
	}
	//客户端收到数据了，不要阻塞
	virtual int ClientRecvData(PAsynClient& client, PCPacket& packet) {
		PAsynClient pair = client->Pair();
		if (pair) {//转发到pair
			pair->Send(packet);
		}
		return 0;
	}
	virtual int ClientSendDone(PAsynClient& client, PCPacket& packet) {
		return 0;
	}
	//通信模块
	PAsynServer m_server;
	//自身
	PBLayer m_self;
};

int main()
{
	PBLayer server = PBLayer(new EAcceptServer("127.0.0.1", 19527));
	server->Start(server);
	(void)getchar();
	return 0;
}
