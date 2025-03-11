#pragma once
#include "http_parser.h"
#include <vector>
#include <string>
#include <map>
class CHttpParser
{
private:
	http_parser m_parser;
	http_parser_settings m_settings;
	std::map<std::string, std::string> m_HeaderValues;
	std::string m_status;
	std::string m_url;
	std::string m_body;
	bool m_complete;
public:
	CHttpParser();
	~CHttpParser();
	CHttpParser(const CHttpParser& http);
	CHttpParser& operator=(const CHttpParser& http);
	size_t Parser(const std::vector<char>& data);
	size_t Parser(const std::string& data);
	unsigned Method() const { return m_parser.method; }
	std::map<std::string, std::string>& Headers() { return m_HeaderValues; }
	std::string& Status() { return m_status; }
	std::string& Url() { return m_url; }
	std::string& Body() { return m_body; }
	std::vector<char> toPackData();
	unsigned Errno() const {
		return m_parser.http_errno;
	}
protected:
	std::string m_lastField;
protected:
	static int OnMessageBegin(http_parser* parser);
	static int OnHeaderField(
		http_parser* parser,
		const char* at,
		size_t length
	);
	static int OnHeaderValue(
		http_parser* parser,
		const char* at,
		size_t length
	);
	static int OnUrl(
		http_parser* parser,
		const char* at,
		size_t length
	);
	static int OnStatus(
		http_parser* parser,
		const char* at,
		size_t length
	);
	static int OnBody(
		http_parser* parser,
		const char* at,
		size_t length
	);
	static int OnHeadersComplete(http_parser* parser);
	static int OnMessageComplete(http_parser* parser);
	int OnMessageBegin();
	int OnHeaderField(
		const char* at,
		size_t length
	);
	int OnHeaderValue(
		const char* at,
		size_t length
	);
	int OnUrl(
		const char* at,
		size_t length
	);
	int OnStatus(
		const char* at,
		size_t length
	);
	int OnBody(
		const char* at,
		size_t length
	);
	int OnHeadersComplete();
	int OnMessageComplete();
};


typedef struct UrlParam
{
	std::string protocol;
	std::string host;
	unsigned short port = 80;
	std::string uri;
	void clear()
	{
		protocol.clear();
		host.clear();
		uri.clear();
		port = 80;
	}
}TUrlParam;

class TParseUrl
{
protected:
	static inline int CompareStr(const char* pos, const char* compare, size_t& clen);
	static inline int FindStr(const char* u, const char* compare);
	static inline const char* FindStrPos(const char* u, const char* compare);
	static int ParseDomain(const char* pos, const char* posend, TUrlParam& param);
	static bool IsNumber(const char* num);
public:
	TParseUrl(const std::string& url) {
		ParseUrl(url, v_param);
	}
	virtual ~TParseUrl() {};
	TUrlParam v_param;

#define POS_JUDGE if(pos>=posend) return -1
#define POS_JUDGE_OK if(pos>=posend) return 0
	static int ParseUrl(const std::string& url, TUrlParam& param);
	std::string GetParam(const std::string& param);
	std::string GetMethod();
	void SetUrl(const std::string& url);
};