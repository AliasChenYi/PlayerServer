#pragma once
#include <WinSock2.h>
#include <Mswsock.h>
#include <WS2tcpip.h>
#include "LogServer.h"
#include "TaskActuator.h"
#include <string>
#include <memory>
#include <map>
#include <list>

class AsynClient;
class AsynServer;
class CPacket;
using PAsynClient = std::shared_ptr<AsynClient>;
using MapClient = std::map<SOCKET, PAsynClient>;
using PAsynServer = std::shared_ptr<AsynServer>;
using PCPacket = std::shared_ptr<CPacket>;
using CPacketList = std::list<PCPacket>;


class CPacket
{
public:
	CPacket() {}
	CPacket(const std::string& data) { m_data = data; }
	CPacket(size_t size) { m_data.resize(size); }
	CPacket(const CPacket& pack);
	CPacket(const void* data, size_t size);
	~CPacket();
	CPacket& operator=(const CPacket& pack);
	operator const char* () const { return m_data.c_str(); }
	operator char* () const {
		return const_cast<char*>(m_data.c_str());
	}
	const std::string& Data() const { return m_data; }
	size_t Size() const { return m_data.size(); }
	void Resize(size_t size) { m_data.resize(size); }
private:
	std::string m_data;
};

#define PK(data, size) PCPacket(new CPacket(data,size))

struct SockAddrIn
	:public sockaddr_in
{
public:
	SockAddrIn(int family = AF_INET) {
		memset(this, 0, sizeof(sockaddr_in));
		sin_family = family;
	}
	SockAddrIn(const sockaddr_in& addr) {
		sin_family = addr.sin_family;
		sin_addr = addr.sin_addr;
		sin_port = addr.sin_port;
		memcpy(sin_zero, &addr.sin_zero, sizeof(sin_zero));
	}
	SockAddrIn(const SockAddrIn& addr) { memcpy(this, &addr, sizeof(SockAddrIn)); }
	SockAddrIn& operator=(const short& port) { sin_port = htons(port); }
	SockAddrIn& operator=(const std::string& ip) { inet_pton(AF_INET, ip.c_str(), &sin_addr); }
	SockAddrIn& operator=(const SockAddrIn& addr) {
		if (this != &addr) {
			memcpy(this, &addr, sizeof(SockAddrIn));
		}
		return *this;
	}
	std::string ip() {
		std::string result;
		result.resize(32);
		result = inet_ntop(sin_family, &sin_addr, const_cast<char*>(result.c_str()), result.size());
		return result;
	}
	operator sockaddr* () { return (sockaddr*)this; }
	size_t size() { return sizeof(sockaddr_in); }
};

class AsynClient
{
public:
	AsynClient();
	~AsynClient();
	//禁止复制
	AsynClient(const AsynClient&) = delete;
	AsynClient& operator=(const AsynClient&) = delete;

	void Connect(const std::string ip, WORD port, PAsynServer& iocp, PAsynClient& self);
	void Init(PAsynServer& server, PAsynClient& self);
	int Send(const PCPacket& packet);//异步写，立刻返回
	int Recv();//异步读，立刻返回

	void Close();

	operator SOCKET() { return m_socket; }
	operator sockaddr* () { return (sockaddr*)&m_addr; }
	std::string Ip() { return m_addr.ip(); }
	short Port() { return ntohs(m_addr.sin_port); }
	void Update() { m_tick = GetTickCount64(); }
	//返回true表示超时了，返回false表示没有超时
	bool IsTimeOut(ULONGLONG nTick, ULONGLONG nDelay = 600000) { return m_tick + nDelay < nTick; }

	void SetPair(PAsynClient& pair) { m_pair = pair; }
	const PAsynClient& Pair() const { return m_pair; }
	PAsynClient& Pair() { return m_pair; }
private:
	PAsynServer m_pServer;
	PAsynClient m_self;
	PAsynClient m_pair;
	SOCKET m_socket;
	SockAddrIn m_addr;
	ULONGLONG m_tick;
};

struct OperatorOverlapped
{
public:
	OperatorOverlapped(PAsynClient pclient, DWORD Type) {
		memset(&overlapped, 0, sizeof(overlapped));
		memset(&wsabuffer, 0, sizeof(wsabuffer));
		recvbytes = 0;
		flags = 0;
		type = Type;
		client = pclient;
	}
	OVERLAPPED overlapped;
	WSABUF wsabuffer;
	DWORD recvbytes;
	DWORD flags;
	DWORD type;//1代表accept 2代表recv  3代表send 4代表connect
	PAsynClient client;
	PCPacket packet;
};
using OpOL = OperatorOverlapped;

class ClientMap
{
public:
	ClientMap();
	~ClientMap();
	int AddClient(PAsynClient& client);//添加一个客户端
	int DelClient(PAsynClient& client);//删除一个客户端
	PAsynClient GetClient(SOCKET s);//获取一个指定的客户端
	int GetClients(MapClient& result);//获取所有客户端的副本
private:
	int ClientMapMain();
private:
	edoyun::CThread m_thread;
	HANDLE m_iocp;
	MapClient m_mapClient;
	enum {
		CMP_NONE,//无操作
		CMP_ADD,//添加
		CMP_QUERY,//单个查询
		CMP_ALL,//获取全部
		CMP_DEL,//删除一个客户端
	};
};
class CBusinessLayer;
using PBLayer = std::shared_ptr<CBusinessLayer>;
class CBusinessLayer
{
public:
	virtual int Start(PBLayer& self) { return -1; };
	virtual void Close() {}
public:
	//客户端连接上来了，不要阻塞
	virtual int ClientConnected(PAsynClient& client) = 0;
	//客户端收到数据了，不要阻塞
	virtual int ClientRecvData(PAsynClient& client, PCPacket& packet) = 0;
	virtual int ClientSendDone(PAsynClient& client, PCPacket& packet) = 0;
};



class AsynServer
{
public:
	AsynServer(const std::string& sIP, short port = 8112);
	~AsynServer();
	//禁止复制
	AsynServer(const AsynServer&) = delete;
	AsynServer& operator=(const AsynServer&) = delete;

	int Start(PAsynServer& self, PBLayer& business);
	void Close();
	MapClient GetClients();
	int DelClient(PAsynClient& client);
	int AddClient(PAsynClient& client);
private:
	int Accept();
	int ServerMain();
private:
	CTaskActuator m_Task;
	ClientMap m_mapClients;
	SOCKET m_socket;
	HANDLE m_iocp;
	std::string m_ip;
	short m_port;
	PAsynServer m_self;
	PBLayer m_business;
private:
	static class WSAIniter {
	public:
		WSAIniter() {
			WSADATA wsadata;
			if (WSAStartup(MAKEWORD(1, 1), &wsadata)) {
				TRACEW(_T("WSAStartup 失败！error = %d"), WSAGetLastError());
			}
		}
		~WSAIniter() {
			if (WSACleanup()) {
				TRACEW(_T("WSACleanup 失败！error = %d"), WSAGetLastError());
			}
		}
	}_;
};

