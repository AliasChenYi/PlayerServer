#include "MysqlClient.h"
#include "LogServer.h"
#include <string>
#ifdef WIN32
#pragma comment(lib, "libmysql.lib")
#endif

using namespace mysql;

MysqlClient::MysqlClient()
	:m_mysql(new MYSQL())
{
	mysql_init(m_mysql.get());
}

MysqlClient::~MysqlClient()
{
	Close();
}

int MysqlClient::Connect(const StrMap& args)
{
	MYSQL* ret = mysql_real_connect(
		m_mysql.get(),
		args.at("host").c_str(),
		args.at("user").c_str(),
		args.at("password").c_str(),
		args.at("db").c_str(),
		strtoul(args.at("port").c_str(), NULL, 10),
		NULL, 0
	);
	if (mysql_errno(m_mysql.get()) != 0)
	{
		printf("%s\r\n", mysql_error(m_mysql.get()));
	}
	return 0;
}

int MysqlClient::Exec(const String& sql)
{
	int ret = mysql_real_query(m_mysql.get(), sql.c_str(), sql.size());
	if (ret != 0) {
		printf("%s size=%d\r\n", mysql_error(m_mysql.get()), sql.size());
	}
	return ret;
}

int MysqlClient::Exec(const String& sql, TableList& result, const _Table_& table)
{
	int ret = mysql_real_query(m_mysql.get(), sql.c_str(), sql.size());
	if (ret != 0) {
		printf("%s\r\n", mysql_error(m_mysql.get()));
		return ret;
	}
	MYSQL_RES* res = mysql_store_result(m_mysql.get());
	MYSQL_ROW row;
	unsigned int num_fields = mysql_num_fields(res);
	while (row = mysql_fetch_row(res)) {
		PTable pt = table.Copy();
		for (unsigned i = 0; i < num_fields; i++) {
			if (row[i] != NULL)
				pt->Columns[i]->FromString(row[i]);

		}
		result.push_back(pt);
	}
	return 0;
}

int MysqlClient::StartTransaction()
{//mysql默认开启事务
	int ret = mysql_real_query(m_mysql.get(), "BEGIN", 6);
	if (ret != 0) {
		printf("%s\r\n", mysql_error(m_mysql.get()));
	}
	return ret;
}

int MysqlClient::CommitTransaction()
{
	int ret = mysql_real_query(m_mysql.get(), "COMMIT", 7);
	if (ret != 0) {
		printf("%s\r\n", mysql_error(m_mysql.get()));
	}
	return ret;
}

int MysqlClient::RollabckTransaction()
{
	int ret = mysql_real_query(m_mysql.get(), "ROLLBACK", 9);
	if (ret != 0) {
		printf("%s\r\n", mysql_error(m_mysql.get()));
	}
	return ret;
}

bool MysqlClient::IsConnected()
{
	return m_mysql != nullptr;
}

int MysqlClient::Close()
{
	mysql_close(m_mysql.get());
	m_mysql.reset();
	return 0;
}

String mysql::_Mysql_Table_::Create()
{
	String result = _T("CREATE TABLE IF NOT EXISTS ") + FullName() + _T(" (\r\n");
	for (unsigned i = 0; i < Columns.size(); i++) {
		if (i > 0)
			result += _T(",\r\n");
		result += Columns[i]->Create();
		if (Columns[i]->Attr & PRIMARY_KEY) {
			result += _T(",\r\n PRIMARY KEY (`") + Columns[i]->Name + _T("`)");
		}
		if (Columns[i]->Attr & UNIQUE) {
			result += _T(",\r\n UNIQUE INDEX `") + Columns[i]->Name + _T("_UNIQUE` (");
			result += Columns[i]->FullName() + _T(" ASC) VISIBLE ");
		}
	}
	result += _T(");");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String mysql::_Mysql_Table_::Insert(const SLParam& Columns, const SLParam& Values)
{
	//INSERT INTO TABLE_NAME (column1, column2, column3,...columnN)  
	//VALUES(value1, value2, value3, ...valueN);
	if (Values.size() == 0)return _T("");
	if (Columns.size() > 0 && (Columns.size() != Values.size()))
		return _T("");
	String result = _T("INSERT INTO ") + FullName() + _T(" ");
	CParamIter it0, it1;
	String sValue;
	if (Columns.size() > 0) {
		result += _T(" ( ");
		it0 = Columns.begin();
		for (unsigned i = 0; it0 != Columns.end(); i++, it0++) {
			if (i > 0)result += _T(" , ");
			result += _T("`") + *it0 + _T("`");
		}
		result += _T(" ) ");
	}
	result += _T(" VALUES ( ");
	it1 = Values.begin();
	for (unsigned i = 0; it1 != Values.end(); i++, it1++) {
		if (i > 0)result += _T(" , ");
		result += *it1;
	}
	result += _T(" );");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String mysql::_Mysql_Table_::Drop()
{
	return _T("DROP TABLE ") + FullName();
}

String mysql::_Mysql_Table_::Modify(const SLParam& Columns, const SLParam& Values)
{
	if (Columns.size() != Values.size() || (Columns.size() == 0))return _T("");
	String result = _T("UPDATE ") + FullName() + _T(" SET ");
	CParamIter col = Columns.begin(), val = Values.begin();
	for (unsigned i = 0; i < Columns.size(); i++) {
		if (i != 0)result += _T(" , ");
		String Name = _T("`") + *col + _T("`");
		result += Name + _T(" = ") + *val;
	}
	bool bfound = false;
	for (PField field : this->Columns) {
		if (field->Attr & PRIMARY_KEY) {
			result += _T(" WHERE ") + field->toEqualExp() + _T(" ;");
			bfound = true;
			break;
		}
	}
	if (!bfound)
		result += _T(" WHERE ") + this->Columns[0]->toEqualExp() + _T(" ;");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String mysql::_Mysql_Table_::Query()
{
	String result = _T("SELECT ");
	for (unsigned i = 0; i < Columns.size(); i++)
	{
		if (i > 0)
			result += _T(',');
		result += _T(" ") + Columns[i]->FullName() + _T(" ");
	}
	result += _T(" FROM ") + FullName() + _T(";");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String mysql::_Mysql_Table_::FullName() const
{
	String Head;
	if (Database.size())
		Head = _T("`") + Database + _T("`.");
	return Head + _T("`") + Name + _T("`");
}

//--------------以下是单元测试-----------------------
#include "gtest/gtest.h"


DECLARE_TABLE_CLASS(edoyunLogin_user_mysql, _Mysql_Table_)
DECLARE_ITEM(user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, INTEGER_FILED, "", "", "")
DECLARE_ITEM(user_qq, NOT_NULL, VARCHAR_FILED, "(15)", "", "")  //QQ号
DECLARE_ITEM(user_phone, DEFAULT, VARCHAR_FILED, "(11)", "'18888888888'", "")  //手机
DECLARE_ITEM(user_name, NOT_NULL, TEXT_FILED, "", "", "")    //姓名
DECLARE_ITEM(user_nick, NOT_NULL, TEXT_FILED, "", "", "")    //昵称
DECLARE_ITEM(user_wechat, DEFAULT, TEXT_FILED, "", "NULL", "")
DECLARE_ITEM(user_wechat_id, DEFAULT, TEXT_FILED, "", "NULL", "")
DECLARE_ITEM(user_address, DEFAULT, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_province, DEFAULT, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_country, DEFAULT, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_age, DEFAULT | CHECK, INTEGER_FILED, "", "18", "")
DECLARE_ITEM(user_male, DEFAULT, BOOL_FILED, "", "1", "")
DECLARE_ITEM(user_flags, DEFAULT, TEXT_FILED, "", "0", "")
DECLARE_ITEM(user_experience, DEFAULT, REAL_FILED, "", "0.0", "")
DECLARE_ITEM(user_level, DEFAULT | CHECK, INTEGER_FILED, "", "0", "")
DECLARE_ITEM(user_class_priority, DEFAULT, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_time_per_viewer, DEFAULT, REAL_FILED, "", "", "")
DECLARE_ITEM(user_career, NONE, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_password, NOT_NULL, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_birthday, NONE, DATETIME_FILED, "", "", "")
DECLARE_ITEM(user_describe, NONE, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_education, NONE, TEXT_FILED, "", "", "")
DECLARE_ITEM(user_register_time, DEFAULT, DATETIME_FILED, "", "LOCALTIME()", "")
DECLARE_TABLE_CLASS_END()

TEST(Database, Mysqltest)
{
	int ret = 0;
	DataBaseClient* client = new MysqlClient();
	ASSERT_NE((DataBaseClient*)NULL, client);
	TableList result;
	edoyunLogin_user_mysql user;
	user.Database = _T("edoyun");
	ret = client->Connect(StrMap({
		{_T("db"),_T("edoyun")},
		{_T("host"),_T("127.0.0.1")},
		{_T("user"),_T("root")},
		{_T("password"),_T("FengPan12#$56")},
		{_T("port"),_T("3306")},
		}));
	EXPECT_EQ(0, ret);
	ret = client->StartTransaction();
	EXPECT_EQ(0, ret);
	ret = client->Exec(user.Create());
	EXPECT_EQ(0, ret);
	ret = client->CommitTransaction();
	EXPECT_EQ(0, ret);
	ret = client->Exec(user.Query(), result, user);
	EXPECT_EQ(0, ret);
	EXPECT_EQ((size_t)0, result.size());
	ret = client->StartTransaction();
	EXPECT_EQ(0, ret);
	ret = client->Exec(
		user.Insert(
			SLParam(_T("user_qq"), _T("user_name"), _T("user_nick"), _T("user_password"), _T("user_phone")),
			SLParam(_T("'1817619619'"), _T("'冯老师'"), _T("'易绝顶'"), _T("'123456'"), _T("18888888888"))
		)
	);
	EXPECT_EQ(0, ret);
	ret = client->Exec(
		user.Insert(
			SLParam(_T("user_qq"), _T("user_name"), _T("user_nick"), _T("user_password"), _T("user_phone")),
			SLParam(_T("'123456789'"), _T("'test'"), _T("'test'"), _T("'123456'"), _T("18888888888"))
		)
	);
	EXPECT_EQ(0, ret);
	ret = client->CommitTransaction();
	EXPECT_EQ(0, ret);
	ret = client->Exec(user.Query(), result, user);
	EXPECT_EQ(0, ret);
	EXPECT_LE((size_t)0, result.size());
	ret = client->StartTransaction();
	EXPECT_EQ(0, ret);
	for (PTable pTable : result) {
		if (TOSTR(pTable->Columns[1]) == _T("123456789")) {
			ret = client->Exec(
				pTable->Modify(
					SLParam(_T("user_qq"), _T("user_name"), _T("user_nick"), _T("user_password")),
					SLParam(_T("'123456789'"), _T("'test'"), _T("'test'"), _T("'123456'"))
				)
			);
			EXPECT_EQ(0, ret);
		}
	}
	ret = client->CommitTransaction();
	EXPECT_EQ(0, ret);
	ret = client->Exec(user.Query(), result, user);
	EXPECT_EQ(0, ret);
	EXPECT_LE((size_t)0, result.size());
	ret = client->Exec(user.Drop());
	EXPECT_EQ(0, ret);
	delete client;
}