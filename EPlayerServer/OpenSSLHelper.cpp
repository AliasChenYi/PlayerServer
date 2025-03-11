#include "OpenSSLHelper.h"
#include "openssl/md5.h"
#ifdef WIN32
#pragma warning(disable: 4996)
#endif

std::string COpenSSLHelper::MD5(const std::string& text)
{

	std::string result;
	std::vector<unsigned char> data;
	data.resize(16);
	MD5_CTX md5;
	MD5_Init(&md5);
	MD5_Update(&md5, text.c_str(), text.size());
	MD5_Final(data.data(), &md5);
	char temp[3] = "";
	for (size_t i = 0; i < data.size(); i++)
	{
		snprintf(temp, sizeof(temp), "%02x", data[i] & 0xFF);
		result += temp;
	}
	return result;
}


//--------------以下是单元测试-----------------------
#include "gtest/gtest.h"

TEST(OpenSSL, Md5)
{
	std::string expect = "9f8fc891e19b6a520f416cd595ac999c";
	std::string text = "hello,edoyun";
	EXPECT_STREQ(expect.c_str(), COpenSSLHelper::MD5(text).c_str());
}