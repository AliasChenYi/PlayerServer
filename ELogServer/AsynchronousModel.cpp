#include "AsynchronousModel.h"


#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")//AcceptEx
using namespace edoyun;

CPacket::CPacket(const CPacket& pack)
{
	m_data = pack.m_data;
}

CPacket::CPacket(const void* data, size_t size)
{
	m_data.resize(size);
	memcpy(const_cast<char*>(m_data.c_str()), data, size);
}

CPacket::~CPacket()
{
	m_data.clear();
}

CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack) {
		m_data = pack.m_data;
	}
	return *this;
}

void AsynClient::Init(PAsynServer& server, PAsynClient& self)
{
	m_pServer = server;
	m_self = self;
	m_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
}

AsynClient::AsynClient()
{
	m_socket = INVALID_SOCKET;
}

AsynClient::~AsynClient()
{
	Close();
}

int AsynClient::Send(const PCPacket& packet)
{
	OpOL* pOp = new OpOL(m_self, 3);//3代表send
	pOp->packet = packet;
	pOp->wsabuffer.buf = *pOp->packet;
	pOp->wsabuffer.len = pOp->packet->Size();
	pOp->recvbytes = pOp->wsabuffer.len;
	int ret = WSASend(m_socket,
		&pOp->wsabuffer,
		1,
		&pOp->recvbytes, 0,
		&pOp->overlapped,
		NULL
	);
	if (ret != 0) {
		printf("ret = %d m_socket=%d error=%d WSA_IO_PENDING(%d)\r\n",
			ret, m_socket, WSAGetLastError(), WSA_IO_PENDING);
	}
	return ret;
}

int AsynClient::Recv()
{
	OpOL* pOp = new OpOL(m_self, 2);//2代表recv
	pOp->packet = std::make_shared<CPacket>(CPacket(1024 * 512));
	pOp->wsabuffer.buf = *pOp->packet;
	pOp->wsabuffer.len = pOp->packet->Size();
	int ret = WSARecv(
		m_socket,
		&pOp->wsabuffer,
		1,
		&pOp->recvbytes,
		&pOp->flags,
		&pOp->overlapped,
		NULL
	);
	if (ret != 0) {
		printf("ret = %d m_socket=%d error=%d WSA_IO_PENDING(%d)\r\n",
			ret, m_socket, WSAGetLastError(), WSA_IO_PENDING);
	}
	return ret;
}

void AsynClient::Close()
{
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

ClientMap::ClientMap()
	:m_thread(&ClientMap::ClientMapMain, this)
{
	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	m_thread.Start();
}

ClientMap::~ClientMap()
{
	if (m_iocp != INVALID_HANDLE_VALUE) {
		PostQueuedCompletionStatus(m_iocp, 0, NULL, NULL);
		WaitForSingleObject(m_iocp, 5);
		HANDLE h = m_iocp;
		m_iocp = INVALID_HANDLE_VALUE;
		m_thread.Stop();
		CloseHandle(h);
	}
}

int ClientMap::AddClient(PAsynClient& client)
{
	if (m_iocp == INVALID_HANDLE_VALUE)return -1;
	OVERLAPPED* overlapped = new OVERLAPPED;
	memset(overlapped, 0, sizeof(OVERLAPPED));
	overlapped->Pointer = new PAsynClient(client);
	if (PostQueuedCompletionStatus(m_iocp, CMP_ADD, NULL, overlapped) == FALSE) {
		delete (PAsynClient*)overlapped->Pointer;
		delete overlapped;
	}
	return 0;
}

int ClientMap::DelClient(PAsynClient& client)
{
	if (m_iocp == INVALID_HANDLE_VALUE)return -1;
	OVERLAPPED* overlapped = new OVERLAPPED;
	memset(overlapped, 0, sizeof(OVERLAPPED));
	overlapped->Pointer = new PAsynClient(client);
	if (PostQueuedCompletionStatus(m_iocp, CMP_DEL, NULL, overlapped) == FALSE) {
		delete (PAsynClient*)overlapped->Pointer;
		delete overlapped;
	}
	return 0;
}

PAsynClient ClientMap::GetClient(SOCKET s)
{
	if (m_iocp == INVALID_HANDLE_VALUE)return PAsynClient();
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	PAsynClient pclient;
	overlapped.Pointer = &pclient;
	if (PostQueuedCompletionStatus(m_iocp, CMP_QUERY, (ULONG_PTR)s, &overlapped) == FALSE)
	{
		CloseHandle(overlapped.hEvent);
		return PAsynClient();
	}
	DWORD ret = WaitForSingleObject(overlapped.hEvent, 1000);
	CloseHandle(overlapped.hEvent);
	if (ret != WAIT_OBJECT_0) {
		return PAsynClient();
	}
	return pclient;
}

int ClientMap::GetClients(MapClient& result)
{
	if (m_iocp == INVALID_HANDLE_VALUE)return -1;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	overlapped.Pointer = &result;
	if (PostQueuedCompletionStatus(m_iocp, CMP_ALL, NULL, &overlapped) == FALSE)
	{
		CloseHandle(overlapped.hEvent);
		return -2;
	}
	DWORD ret = WaitForSingleObject(overlapped.hEvent, 1000);
	CloseHandle(overlapped.hEvent);
	if (ret != WAIT_OBJECT_0) {
		return -3;
	}
	return 0;
}

int ClientMap::ClientMapMain()
{
	DWORD NOBT = 0;
	ULONG_PTR CK = 0;
	LPOVERLAPPED Overlapped = NULL;
	while (m_iocp != INVALID_HANDLE_VALUE) {
		if (GetQueuedCompletionStatus(m_iocp, &NOBT, &CK, &Overlapped, 1)) {
			switch (NOBT) {
			case CMP_NONE://退出了
				break;
			case CMP_ADD:
			{
				PAsynClient* ppClient = (PAsynClient*)Overlapped->Pointer;
				if (ppClient != NULL) {
					m_mapClient.insert(std::pair<SOCKET, PAsynClient>(**ppClient, *ppClient));
					delete ppClient;
					delete Overlapped;
				}
			}
			break;
			case CMP_DEL:
			{
				PAsynClient* ppClient = (PAsynClient*)Overlapped->Pointer;
				if (ppClient != NULL) {
					m_mapClient.erase((SOCKET) * *ppClient);
					delete ppClient;
					delete Overlapped;
				}
			}
			break;
			case CMP_QUERY:
			{
				SOCKET s = (SOCKET)CK;
				PAsynClient* ppClient = (PAsynClient*)Overlapped->Pointer;
				if (ppClient != NULL) {
					MapClient::iterator it = m_mapClient.find(CK);
					if (it != m_mapClient.end()) {
						*ppClient = it->second;
					}
					else {
						ppClient->reset();
					}
					SetEvent(Overlapped->hEvent);
				}
			}
			break;
			case CMP_ALL:
			{
				MapClient* pMap = (MapClient*)Overlapped->Pointer;
				if (pMap != NULL) {
					*pMap = m_mapClient;
					SetEvent(Overlapped->hEvent);
				}
			}
			}
		}
	}
	return 0;
}

AsynServer::WSAIniter AsynServer::_;

AsynServer::AsynServer(const std::string& sIP, short port)
	:m_iocp(INVALID_HANDLE_VALUE), m_port(-1)
{
	m_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET) {
		printf("socket 失败！errno = %d\r\n", WSAGetLastError());
		return;
	}
	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_Task.GetCpuCount());
	if (m_iocp == INVALID_HANDLE_VALUE) {
		printf("CreateIoCompletionPort 失败！errno = %d\r\n", GetLastError());
		return;
	}

	sockaddr_in addr;
	addr.sin_family = PF_INET;
	inet_pton(AF_INET, sIP.c_str(), &addr.sin_addr);
	addr.sin_port = htons(port);
	int ret = 0;
	ret = bind(m_socket, (sockaddr*)&addr, sizeof(addr));
	if (ret != 0) {
		printf("bind 失败！ret = %d errno = %d\r\n", ret, WSAGetLastError());
		return;
	}
	ret = listen(m_socket, 10);
	if (ret != 0) {
		printf("listen 失败！ret = %d errno = %d\r\n", ret, WSAGetLastError());
	}
	HANDLE handle = CreateIoCompletionPort((HANDLE)m_socket, m_iocp, NULL, 0);
	if (handle == INVALID_HANDLE_VALUE) {
		printf(_T("CreateIoCompletionPort GetLastError = %d\r\n"), GetLastError());
	}
	m_ip = sIP;
	m_port = port;
}

AsynServer::~AsynServer()
{
	Close();
}

int AsynServer::Start(PAsynServer& self, PBLayer& business)
{
	int ret = 0;
	m_self = self;
	ret = m_Task.Run();
	if (ret != 0) {
		printf(_T("Run ret = %d\r\n"), ret);
	}
	ret = m_Task.AddTask(Task(&AsynServer::Accept, this));
	if (ret != 0) {
		printf(_T("AddTask失败 ret = %d\r\n"), ret);
	}
	for (UINT i = 1; i < m_Task.GetCpuCount(); i++)
	{
		ret = m_Task.AddTask(Task(&AsynServer::ServerMain, this));
		if (ret != 0) {
			printf(_T("AddTask失败 ret = %d\r\n"), ret);
		}
	}
	m_business = business;
	return ret;
}

void AsynServer::Close()
{
	SOCKET s = m_socket;
	HANDLE h = m_iocp;
	m_socket = INVALID_SOCKET;
	m_iocp = INVALID_HANDLE_VALUE;
	PostQueuedCompletionStatus(h, 0, NULL, NULL);
	m_Task.Close();
	closesocket(s);
	CloseHandle(h);
}

MapClient AsynServer::GetClients()
{
	MapClient clients;
	m_mapClients.GetClients(clients);
	return clients;
}

int AsynServer::DelClient(PAsynClient& client)
{
	return m_mapClients.DelClient(client);
}

int AsynServer::Accept()
{
	PAsynClient client(new AsynClient());
	client->Init(m_self, client);
	CreateIoCompletionPort((HANDLE)(SOCKET)*client, m_iocp, NULL, NULL);
	OpOL* olp = new OpOL(client, 1);
	olp->packet = std::make_shared<CPacket>(CPacket(128));
	BOOL ret = AcceptEx(m_socket, *client,
		(char*)*olp->packet,
		0, sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		&olp->recvbytes, &olp->overlapped);
	if (ret == FALSE) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			printf(_T("AcceptEx WSAGetLastError = %d WSA_IO_PENDING(%d)\r\n"), WSAGetLastError(), WSA_IO_PENDING);
		}
	}
	return 0;
}

int AsynServer::ServerMain()
{
	DWORD NOBT = 0;
	ULONG_PTR CK = 0;
	LPOVERLAPPED Overlapped = NULL;
	OpOL* opl = NULL;
	int len = 0;
	int ret = 0;
	if (GetQueuedCompletionStatus(m_iocp, &NOBT, &CK, &Overlapped, INFINITE)) {
		if (Overlapped != 0) {
			opl = CONTAINING_RECORD(Overlapped, OperatorOverlapped, overlapped);
			switch (opl->type)
			{
			case 0://终止程序
				break;
			case 1://接入客户端
				//添加新的等待
				ret = m_Task.AddTask(Task(&AsynServer::Accept, this));
				if (ret != 0) {
					printf(_T("AddTask失败 ret = %d\r\n"), ret);
				}
				setsockopt(*opl->client, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
					(char*)&m_socket, sizeof(m_socket));
				len = sizeof(sockaddr_in);
				getpeername(*opl->client, *opl->client, &len);
				m_mapClients.AddClient(opl->client);//加入客户端映射表
				opl->client->Recv();//客户端添加关注接收
				m_business->ClientConnected(opl->client);//交给业务层处理
				delete opl;//释放请求数据
				break;
			case 2://客户端收到数据
				if (NOBT > 0) {//对方套接字仍然保持连接
					opl->client->Recv();//客户端继续关注接收
				}
				opl->packet->Resize(opl->overlapped.InternalHigh);//处理包大小
				m_business->ClientRecvData(opl->client, opl->packet);//交给业务层处理
				delete opl;//释放请求数据
				break;
			case 3://客户端数据发送完成
				m_business->ClientSendDone(opl->client, opl->packet);//交给业务层处理
				delete opl;//释放请求数据
				break;
			}
		}
	}
	if (m_socket != INVALID_SOCKET && (m_iocp != INVALID_HANDLE_VALUE)) {
		//服务器正常的话 继续下一次任务。
		int ret = m_Task.AddTask(Task(&AsynServer::ServerMain, this));
		if (ret != 0) {
			printf(_T("AddTask失败 ret = %d\r\n"), ret);
		}
	}
	return 0;
}
