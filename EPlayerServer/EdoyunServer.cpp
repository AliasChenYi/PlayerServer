#include "EdoyunServer.h"


using namespace edoyun;
using namespace _sqlite3;

DECLARE_TABLE_CLASS(edoyunLogin_user, _Sqlite3_Table_)
DECLARE_ITEM(user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, INTEGER_FILED, "", "", "")
DECLARE_ITEM(user_qq, NOT_NULL, VARCHAR_FILED, "(15)", "", "")  //QQ号
DECLARE_ITEM(user_phone, DEFAULT, TEXT_FILED, "", "18888888888", "")  //手机
DECLARE_ITEM(user_name, NOT_NULL, TEXT_FILED, "", "", "")    //姓名
DECLARE_ITEM(user_nick, NOT_NULL, TEXT_FILED, "", "", "")    //昵称
DECLARE_ITEM(user_wechat, DEFAULT, TEXT_FILED, "", "none", "")
DECLARE_ITEM(user_wechat_id, DEFAULT, TEXT_FILED, "", "none", "")
DECLARE_ITEM(user_address, DEFAULT, TEXT_FILED, "", "\"长安大街1号\"", "")
DECLARE_ITEM(user_province, DEFAULT, TEXT_FILED, "", "\"北京\"", "")
DECLARE_ITEM(user_country, DEFAULT, TEXT_FILED, "", "\"中国\"", "")
DECLARE_ITEM(user_age, DEFAULT | CHECK, INTEGER_FILED, "", "18", "\"user_age\" >= 0")
DECLARE_ITEM(user_male, DEFAULT, BOOL_FILED, "", "1", "")
DECLARE_ITEM(user_flags, DEFAULT, TEXT_FILED, "", "0", "")
DECLARE_ITEM(user_experience, DEFAULT, REAL_FILED, "", "0.0", "")
DECLARE_ITEM(user_level, DEFAULT | CHECK, INTEGER_FILED, "", "0", "\"user_level\" >= 0")
DECLARE_ITEM(user_class_priority, DEFAULT, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_time_per_viewer, DEFAULT, REAL_FILED, "", "", "")
DECLARE_ITEM(user_career, NONE, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_password, NOT_NULL, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_birthday, NONE, DATETIME_FILED, "", "", "")
DECLARE_ITEM(user_describe, NONE, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_education, NONE, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_register_time, DEFAULT, DATETIME_FILED, "", "(datetime('now', 'localtime'))", "")
DECLARE_TABLE_CLASS_END()

EdoyunServer::EdoyunServer(const String& ip, unsigned short port)
	:m_server(new AsynServer(ip, port)),
	m_db(new Sqlite3Client()),
	m_tick(new edoyun::CThread(&EdoyunServer::OnTimer, this))
{}

EdoyunServer::~EdoyunServer()
{
	m_server->Close();
	m_tick->Stop();
	m_db->Close();

}

int EdoyunServer::Start(PBLayer& self)
{
	m_self = self;
	m_server->Start(m_server, self);
	m_tick->Start();
	m_db->Connect(edoyun::StrMap({ {_T("host"),_T("sqlite3.db")} }));
	m_task.Run();
	return 0;
}

void EdoyunServer::Close()
{
	m_tick->Stop();
	m_server->Close();
	m_db->Close();
	m_task.Close();
}

int EdoyunServer::ClientConnected(PAsynClient& client)
{
	return UserOnline(client);
}

int EdoyunServer::ClientRecvData(PAsynClient& client, PCPacket& packet)
{
	client->Update();
	m_task.AddTask(edoyun::Task(&EdoyunServer::DealPacket, this, client, packet));
	return 0;
}

int EdoyunServer::ClientSendDone(PAsynClient& client, PCPacket& packet)
{
	client->Update();
	return 0;
}

int EdoyunServer::OnTimer()
{
	static ULONGLONG tick = GetTickCount64();
	//每1分钟检测一次客户端的情况
	if (GetTickCount64() - tick > 60 * 1000) {
		tick = GetTickCount64();
		OnClientTimeOut(tick);
	}
	return 0;
}

void EdoyunServer::OnClientTimeOut(ULONGLONG nTick)
{
	MapClient clients = m_server->GetClients();
	if (clients.size() > 0) {
		MCIter it = clients.begin();
		for (; it != clients.end(); it++) {
			if ((*it).second->IsTimeOut(nTick)) {//超时则下线用户
				m_server->DelClient((*it).second);
				TRACEI(_T("客户端下线：ip %s port %d"), (*it).second->Ip().c_str(), (*it).second->Port() & 0xFFFF);
			}
		}
	}
}

int EdoyunServer::DealPacket(PAsynClient& client, PCPacket& packet)
{
	CHttpParser parser;
	size_t size = parser.Parser(packet->Data());
	if (parser.Errno() == 0) {
		TRACEI("method %d", parser.Method());
		std::string strUrl = parser.Url();
		
		switch (parser.Method())
		{
		case HTTP_GET:
		{
			strUrl = "https://127.0.0.1" + strUrl;
			TParseUrl url(strUrl);
			String method = url.GetMethod();
			if (method == "login") {
				UserLogin(client, parser, url);
			}
		}
		break;
		default:
			break;
		}
	}
	return 0;
}

int EdoyunServer::UserOnline(PAsynClient& client)
{
	client->Update();
	TRACEI(_T("客户端上线：ip %s port %d"), client->Ip().c_str(), client->Port() & 0xFFFF);
	return 0;
}

int EdoyunServer::UserLogin(PAsynClient& client, CHttpParser& packet, TParseUrl& url)
{
	static String MD5Key = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
	if (MD5Key.size() < 37) {
		MD5Key += "h-b*g/h@n!h#";
		MD5Key += "n$d^ssx,.kl<kl";
	}
	
	TRACEI("time %s", url.GetParam("time").c_str());
	TRACEI("salt %s", url.GetParam("salt").c_str());
	TRACEI("user %s", url.GetParam("user").c_str());
	TRACEI("sign %s", url.GetParam("sign").c_str());
	std::string user = url.GetParam("user");
	String passwd, nick;
	std::string msg;
	edoyunLogin_user table;
	TableList result;
	String sql = "SELECT user_password,user_nick from edoyunLogin_user where user_qq=" + user + ";";
	int ret = m_db->Exec(sql, result, table);
	if (ret == 0 && result.size() > 0) {
		passwd = TOSTR(result.front()->Fields["user_password"]);
		nick = TOSTR(result.front()->Fields["user_nick"]);
	}
	TRACEI("passwd %s", passwd.c_str());
	std::string text = url.GetParam("time");
	text += MD5Key + passwd + url.GetParam("salt");
	TRACEI("text %s", text.c_str());
	Json::Value response;
	response["message"] = "success";
	response["status"] = 0;
	char temp[64] = "";
	std::string Url = "HTTP/1.1 200 OK\r\n";
	time_t t;
	time(&t);
	tm ptm;
	localtime_s(&ptm, &t);
	strftime(temp, sizeof(temp), "%a, %d %b %G %T GMT\r\n", &ptm);
	std::string Date = std::string("Date: ") + temp;
	memset(temp, 0, sizeof(temp));
	std::string Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Options: DENY\r\n";
	std::string sign = url.GetParam("sign");
	std::string md5 = COpenSSLHelper::MD5(text);
	TRACEI("local [%s] client [%s]", md5.c_str(), sign.c_str());
	if (md5 != sign) {
		response["status"] = -1;
		response["message"] = "用户不存在，或者密码错误!";
	}
	else {
		response["user"] = nick;
	}

	std::string json = response.toStyledString();// JsonToString(result);
	snprintf(temp, sizeof(temp), "%d", json.size());
	std::string Length = std::string("Content-Length: ") + temp + "\r\n";
	std::string Head = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
	std::string http = Url + Date + Server + Length + Head + json;
	TRACEI("http %s\r\n", http.c_str());
	TRACEI("send %d", client->Send(PCPacket(new CPacket(http))));
	return 0;
}
