#include "HttpParser.h"
#include <Windows.h>
#include <functional>
#include <memory>
#include "LogServer.h"

CHttpParser::CHttpParser()
{
	memset(&m_parser, 0, sizeof(m_parser));
	http_parser_init(&m_parser, HTTP_REQUEST);
	m_settings.on_message_begin = &CHttpParser::OnMessageBegin;
	m_settings.on_header_field = &CHttpParser::OnHeaderField;
	m_settings.on_header_value = &CHttpParser::OnHeaderValue;
	m_settings.on_url = &CHttpParser::OnUrl;
	m_settings.on_status = &CHttpParser::OnStatus;
	m_settings.on_body = &CHttpParser::OnBody;
	m_settings.on_headers_complete = &CHttpParser::OnHeadersComplete;
	m_settings.on_message_complete = &CHttpParser::OnMessageComplete;
}

CHttpParser::~CHttpParser()
{
}

CHttpParser::CHttpParser(const CHttpParser& http)
{
	memset(&m_parser, 0, sizeof(m_parser));
	http_parser_init(&m_parser, HTTP_REQUEST);
	m_settings.on_message_begin = &CHttpParser::OnMessageBegin;
	m_settings.on_header_field = &CHttpParser::OnHeaderField;
	m_settings.on_header_value = &CHttpParser::OnHeaderValue;
	m_settings.on_url = &CHttpParser::OnUrl;
	m_settings.on_status = &CHttpParser::OnStatus;
	m_settings.on_body = &CHttpParser::OnBody;
	m_settings.on_headers_complete = &CHttpParser::OnHeadersComplete;
	m_settings.on_message_complete = &CHttpParser::OnMessageComplete;
	m_HeaderValues.insert(http.m_HeaderValues.begin(), http.m_HeaderValues.end());
	m_status = http.m_status;
	m_url = http.m_url;
	m_body = http.m_body;
}

CHttpParser& CHttpParser::operator=(const CHttpParser& http)
{
	if (this != &http) {
		memset(&m_parser, 0, sizeof(m_parser));
		http_parser_init(&m_parser, HTTP_REQUEST);
		m_settings.on_message_begin = &CHttpParser::OnMessageBegin;
		m_settings.on_header_field = &CHttpParser::OnHeaderField;
		m_settings.on_header_value = &CHttpParser::OnHeaderValue;
		m_settings.on_url = &CHttpParser::OnUrl;
		m_settings.on_status = &CHttpParser::OnStatus;
		m_settings.on_body = &CHttpParser::OnBody;
		m_settings.on_headers_complete = &CHttpParser::OnHeadersComplete;
		m_settings.on_message_complete = &CHttpParser::OnMessageComplete;
		m_HeaderValues.insert(http.m_HeaderValues.begin(), http.m_HeaderValues.end());
		m_status = http.m_status;
		m_url = http.m_url;
		m_body = http.m_body;
	}
	return *this;
}

size_t CHttpParser::Parser(const std::vector<char>& data)
{
	return http_parser_execute(&m_parser, &m_settings, data.data(), data.size());
}

size_t CHttpParser::Parser(const std::string& data)
{
	m_complete = false;
	size_t ret = http_parser_execute(&m_parser, &m_settings, data.c_str(), data.size());
	if (!m_complete) {
		m_parser.http_errno = -1;
		return 0;
	}
	return ret;
}

int CHttpParser::OnMessageBegin(http_parser* parser)
{
	TRACEI("");
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnMessageBegin();
	}
	return 0;
}

int CHttpParser::OnHeaderField(http_parser* parser, const char* at, size_t length)
{
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnHeaderField(at, length);
	}
	return 0;
}

int CHttpParser::OnHeaderValue(http_parser* parser, const char* at, size_t length)
{
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnHeaderValue(at, length);
	}
	return 0;
}

int CHttpParser::OnUrl(http_parser* parser, const char* at, size_t length)
{
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnUrl(at, length);
	}
	return 0;
}

int CHttpParser::OnStatus(http_parser* parser, const char* at, size_t length)
{
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnStatus(at, length);
	}
	return 0;
}

int CHttpParser::OnBody(http_parser* parser, const char* at, size_t length)
{
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnBody(at, length);
	}
	return 0;
}

int CHttpParser::OnHeadersComplete(http_parser* parser)
{
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnHeadersComplete();
	}
	return 0;
}

int CHttpParser::OnMessageComplete(http_parser* parser)
{
	TRACEI("");
	CHttpParser* thiz = CONTAINING_RECORD(parser, CHttpParser, m_parser);
	if (thiz) {
		thiz->OnMessageComplete();
	}
	return 0;
}

int CHttpParser::OnMessageBegin()
{
	TRACEI("");
	return 0;
}

int CHttpParser::OnHeaderField(const char* at, size_t length)
{
	m_lastField = std::string(at, length);
	return 0;
}

int CHttpParser::OnHeaderValue(const char* at, size_t length)
{
	std::string value(at, length);
	m_HeaderValues[m_lastField] = value;
	return 0;
}

int CHttpParser::OnUrl(const char* at, size_t length)
{
	m_url = std::string(at, length);
	return 0;
}

int CHttpParser::OnStatus(const char* at, size_t length)
{
	m_status = std::string(at, length);
	return 0;
}

int CHttpParser::OnBody(const char* at, size_t length)
{
	m_body = std::string(at, length);
	return 0;
}

int CHttpParser::OnHeadersComplete()
{
	TRACEI("OnHeadersComplete");
	return 0;
}

int CHttpParser::OnMessageComplete()
{
	TRACEI("OnMessageComplete");
	m_complete = true;
	return 0;
}

inline int TParseUrl::CompareStr(const char* pos, const char* compare, size_t& clen)
{
	for (size_t i = 0; i < clen; i++)
	{
		if (pos[i] != compare[i])
			return -1;
	}
	return 0;
}

inline int TParseUrl::FindStr(const char* u, const char* compare)
{
	size_t clen = strlen(compare);
	size_t ulen = strlen(u);
	if (clen > ulen)
		return -1;
	const char* pos = u;
	const char* posend = u + ulen - 1;
	for (; pos <= posend - clen; pos++)
	{
		if (CompareStr(pos, compare, clen) == 0)
		{
			return (int)(pos - u);
			//return 0;
		}
	}
	return -1;
}

inline const char* TParseUrl::FindStrPos(const char* u, const char* compare)
{
	size_t clen = strlen(compare);
	size_t ulen = strlen(u);
	if (clen > ulen)
		return NULL;
	const char* pos = u;
	const char* posend = u + ulen - 1;
	for (; pos <= posend - clen; pos++)
	{
		if (CompareStr(pos, compare, clen) == 0)
		{
			return pos;
		}
	}
	return NULL;
}

int TParseUrl::ParseDomain(const char* pos, const char* posend, TUrlParam& param)
{
	int point = FindStr(pos, ":");
	if (point >= 0)
	{
		param.host = std::string(pos, point);
		pos += point + 1;
		std::string tmp = std::string(pos, posend - pos);
		if (IsNumber(tmp.c_str()))
			param.port = atoi(tmp.c_str());
		return 0;
	}
	return -1;
}
bool TParseUrl::IsNumber(const char* num)
{
	int length = (int)strlen(num);
	for (int i = 0; i < length; i++)
	{
		if (i == 0 && (num[i] == '+' || num[i] == '-'))
		{
			if (length > 1)
				continue;
			return false;
		}
		if (!isdigit(num[i]))
			return false;
	}
	return true;
}
int TParseUrl::ParseUrl(const std::string& url, TUrlParam& param)

{
	const char* posend = url.c_str() + url.size() - 1;
	param.uri = url;
	const char* pos = url.c_str();
	int point = 0;
	if ((point = FindStr(pos, "://")) >= 0)
	{
		param.protocol = std::string(url, point);
	}
	else
		return -1;
	pos += point + 3; //strlen("://")
	POS_JUDGE;
	if ((point = FindStr(pos, "/")) >= 0)
	{
		param.host = std::string(pos, point);
		const char* end = pos + point;
		ParseDomain(pos, end, param);
		param.uri = std::string(pos + point + 1);
	}
	else
	{
		//the left all is domain
		int hlen = (int)(posend - pos + 1);
		param.host = std::string(pos, hlen);
		const char* end = pos + hlen - 1;
		ParseDomain(pos, end, param);
		param.uri = "/";
		return 0;
	}
	return 0;
}

std::string TParseUrl::GetParam(const std::string& param)
{
	int point = -1;
	const char* ustart = v_param.uri.c_str();

	const char* start = FindStrPos(ustart, "?");
	if (start != NULL)
	{
		++start;
		//?a=abc&b=ddd
		std::string par = param;
		par += "=";
		start = FindStrPos(start, par.c_str());
		if (start != NULL)
		{
			const char* j = start - 1;
			char c = *j;
			if (c == '&' || c == '?')
			{
				start += par.length();
				const char* end = FindStrPos(start, "&");
				if (end != NULL)
				{
					return std::string(start, end);
				}
				return std::string(start);
			}
		}
	}
	return "";
}

std::string TParseUrl::GetMethod()
{
	const char* start = v_param.uri.c_str();
	const char* end = FindStrPos(start, "?");
	if (end == NULL)return start;
	return std::string(start, end);
}

void TParseUrl::SetUrl(const std::string& url)
{
	v_param.clear();
	ParseUrl(url, v_param);
}

//--------------以下是单元测试-----------------------
#include "gtest/gtest.h"

TEST(HttpModule, CHttpParser)
{
	std::string str = "GET /favicon.ico HTTP/1.1\r\n"
		"Host: 0.0.0.0=5000\r\n"
		"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language: en-us,en;q=0.5\r\n"
		"Accept-Encoding: gzip,deflate\r\n"
		"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
		"Keep-Alive: 300\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
	CHttpParser parser;
	size_t size = parser.Parser(str);
	EXPECT_EQ(0, parser.Errno());
	EXPECT_EQ(365, size);
	TRACEI("errno = %d size = %d", parser.Errno(), size);
	if (parser.Errno() == 0) {
		TRACEI("Method %d url %s", parser.Method(), parser.Url().c_str());
		EXPECT_EQ(HTTP_GET, parser.Method());
	}
	//下面是一个错误的解析，应该解析失败！
	str = "GET /favicon.ico HTTP/1.1\r\n"
		"Host: 0.0.0.0=5000\r\n"
		"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
	CHttpParser parser1;
	size = parser1.Parser(str);
	TRACEI("errno = %d size = %d", parser1.Errno(), size);
	EXPECT_EQ(127, parser1.Errno());
	EXPECT_EQ(0, size);
}

TEST(HttpModule, TParseUrl)
{
	TParseUrl url1("https://www.baidu.com/s?ie=utf8&oe=utf8&wd=httplib&tn=98010089_dg&ch=3");
	EXPECT_STREQ("utf8", url1.GetParam("ie").c_str());
	EXPECT_STREQ("utf8", url1.GetParam("oe").c_str());
	EXPECT_STREQ("httplib", url1.GetParam("wd").c_str());
	EXPECT_STREQ("98010089_dg", url1.GetParam("tn").c_str());
	EXPECT_STREQ("3", url1.GetParam("ch").c_str());
	TParseUrl url2("http://127.0.0.1:19811/?time=144000&salt=9527&user=test&sign=1234567890abcdef");
	EXPECT_STREQ("144000", url2.GetParam("time").c_str());
	EXPECT_STREQ("9527", url2.GetParam("salt").c_str());
	EXPECT_STREQ("test", url2.GetParam("user").c_str());
	EXPECT_STREQ("1234567890abcdef", url2.GetParam("sign").c_str());
}

