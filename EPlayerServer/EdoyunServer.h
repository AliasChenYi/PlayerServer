#pragma once
#include "DatabaseHelper.h"
#include "AsynchronousModel.h"
#include "Sqlite3Client.h"
#include "HttpParser.h"
#include "json.h"
#include "OpenSSLHelper.h"

using PDBClient = std::shared_ptr<edoyun::DataBaseClient>;
using String = edoyun::String;
using PThread = std::shared_ptr<edoyun::CThread>;
using MCIter = MapClient::iterator;
using MCCIter = MapClient::const_iterator;

class EdoyunServer :
	public CBusinessLayer
{
public:
	EdoyunServer(const String& ip, unsigned short port);
	~EdoyunServer();
	virtual int Start(PBLayer& self);
	virtual void Close();
public:
	//客户端连接上来了，不要阻塞
	virtual int ClientConnected(PAsynClient& client);
	//客户端收到数据了，不要阻塞
	virtual int ClientRecvData(PAsynClient& client, PCPacket& packet);
	//向客户端发送数据
	virtual int ClientSendDone(PAsynClient& client, PCPacket& packet);
protected:
	int OnTimer();
	//用户如果登录后长时间没有数据交互，则认定为离线！
	void OnClientTimeOut(ULONGLONG nTick);
	//处理收到的数据
	int DealPacket(PAsynClient& client, PCPacket& packet);
protected:
	int UserOnline(PAsynClient& client);
	int UserLogin(PAsynClient& client, 
		CHttpParser& packet, 
		TParseUrl& strUrl);
private:
	//通信模块
	PAsynServer m_server;
	//自身
	PBLayer m_self;
	//数据库模块
	PDBClient m_db;
	//定时器线程
	PThread m_tick;
	CTaskActuator m_task;
};

