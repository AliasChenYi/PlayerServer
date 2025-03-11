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
	//�ͻ������������ˣ���Ҫ����
	virtual int ClientConnected(PAsynClient& client);
	//�ͻ����յ������ˣ���Ҫ����
	virtual int ClientRecvData(PAsynClient& client, PCPacket& packet);
	//��ͻ��˷�������
	virtual int ClientSendDone(PAsynClient& client, PCPacket& packet);
protected:
	int OnTimer();
	//�û������¼��ʱ��û�����ݽ��������϶�Ϊ���ߣ�
	void OnClientTimeOut(ULONGLONG nTick);
	//�����յ�������
	int DealPacket(PAsynClient& client, PCPacket& packet);
protected:
	int UserOnline(PAsynClient& client);
	int UserLogin(PAsynClient& client, 
		CHttpParser& packet, 
		TParseUrl& strUrl);
private:
	//ͨ��ģ��
	PAsynServer m_server;
	//����
	PBLayer m_self;
	//���ݿ�ģ��
	PDBClient m_db;
	//��ʱ���߳�
	PThread m_tick;
	CTaskActuator m_task;
};

