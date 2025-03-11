#include "Sqlite3Client.h"
#include "LogServer.h"

using namespace _sqlite3;

#ifdef _UNICODE
#define SqliteOpen sqlite3_open16
#define Errmsg sqlite3_errmsg16
#else
#define SqliteOpen sqlite3_open
#define Errmsg sqlite3_errmsg
#endif

Sqlite3Client::Sqlite3Client()
	:DataBaseClient()
{
	m_db = NULL;
	m_stmt = NULL;
}

Sqlite3Client::~Sqlite3Client()
{
}

int Sqlite3Client::Connect(const StrMap& args)
{
	if (args.find(_T("host")) == args.end()) {
		return -1;
	}
	int ret = SqliteOpen(args.at(_T("host")).c_str(), &m_db);
	if (ret != 0) {
		TRACEE(_T("open failed!ret = %d,message:%s"), ret, Errmsg(m_db));
	}
	return ret;
}

int Sqlite3Client::Exec(const String& sql)
{
	int ret = sqlite3_exec(m_db, sql.c_str(), NULL, this, NULL);
	if (ret != SQLITE_OK) {
		TRACEE(_T("Exec failed!ret = %d,message:%s"), ret, Errmsg(m_db));
	}
	return ret;
}

int Sqlite3Client::Exec(const String& sql, TableList& result, const _Table_& table)
{
	Param* param = new Param(this, table, result);
	char* errmsg = NULL;
	int ret = sqlite3_exec(
		m_db, sql.c_str(),
		&Sqlite3Client::SelectCallbackEntry,
		param,
		&errmsg);
	sqlite3_free(errmsg);
	if (ret != SQLITE_OK)
	{
		TRACEE(_T("Exec failed!ret = %d,message:%s"), ret, Errmsg(m_db));
	}
	delete param;
	return ret;
}

int Sqlite3Client::StartTransaction()
{
	int ret = sqlite3_exec(m_db, "begin transaction", 0, 0, NULL);
	if (ret != SQLITE_OK)
	{
		TRACEE(_T("begin failed!ret = %d,message:%s"), ret, Errmsg(m_db));
	}
	return ret;
}

int Sqlite3Client::CommitTransaction()
{
	int ret = sqlite3_exec(m_db, "commit transaction", 0, 0, NULL);
	if (ret != SQLITE_OK)
	{
		TRACEE(_T("commit failed!ret = %d,message:%s"), ret, Errmsg(m_db));
	}
	return ret;
}

int Sqlite3Client::RollabckTransaction()
{
	int ret = sqlite3_exec(m_db, "rollback transaction", 0, 0, NULL);
	if (ret != SQLITE_OK)
	{
		TRACEE(_T("rollback failed!ret = %d,message:%s"), ret, Errmsg(m_db));
	}
	return ret;
}

bool Sqlite3Client::IsConnected()
{
	return m_db == NULL;
}

int Sqlite3Client::Close()
{
	return sqlite3_close(m_db);
}

int Sqlite3Client::SelectCallbackEntry(void* param, int nCol, char** values, char** names)
{
	Sqlite3Client::Param* thiz = (Sqlite3Client::Param*)param;
	return thiz->m_client->SelectCallback(
		nCol, values, names, thiz
	);
}

int Sqlite3Client::SelectCallback(int nCol, char* values[], char* names[], Sqlite3Client::Param* param)
{
#ifdef _MBCS
	TableList& result = param->m_result;
	const _Table_& table = param->m_table;
	PTable pTable = table.Copy();
	if (pTable == nullptr) {
		TRACEE(_T("create table failed! table:%s"), table.FullName().c_str());
		return 0;
	}
	FieldMap& fields = pTable->Fields;
	for (int i = 0; i < nCol; i++) {
		String name = names[i];
		FMapIter it = fields.find(name);
		if (it != fields.end()) {
			if (values[i] != NULL) {
				it->second->FromString(values[i]);
			}
		}
		else {
			TRACEE(_T("field not found! table:%s field:%s"),
				table.FullName().c_str(), names[i]);
		}
	}
	result.push_back(pTable);
#else
#endif
	return 0;
}

String _sqlite3::_Sqlite3_Table_::Create()
{
	String result = _T("CREATE TABLE IF NOT EXISTS ") + FullName() + _T(" (\r\n");
	for (unsigned i = 0; i < Columns.size(); i++) {
		if (i > 0)
			result += _T(',');
		result += Columns[i]->Create();
	}
	result += _T(");");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String _sqlite3::_Sqlite3_Table_::Insert(const SLParam& Columns, const SLParam& Values)
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
			result += _T("\"") + *it0 + _T("\"");
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

String _sqlite3::_Sqlite3_Table_::Drop()
{
	String result = String(_T("DROP TABLE ")) + FullName() + _T(";");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String _sqlite3::_Sqlite3_Table_::Modify(const SLParam& Columns, const SLParam& Values)
{
	if (Columns.size() != Values.size() || (Columns.size() == 0))return _T("");
	String result = _T("UPDATE ") + FullName() + _T(" SET ");
	CParamIter col = Columns.begin(), val = Values.begin();
	for (unsigned i = 0; i < Columns.size(); i++) {
		if (i != 0)result += _T(" , ");
		String Name = _T("\"") + *col + _T("\"");
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

String _sqlite3::_Sqlite3_Table_::Query()
{
	String result = _T("SELECT ");
	for (unsigned i = 0; i < Columns.size(); i++)
	{
		if (i > 0)
			result += _T(',');
		result += _T(" \"") + Columns[i]->Name + _T("\" ");
	}
	result += _T(" FROM ") + FullName() + _T(";");
	TRACEI(_T("%s"), result.c_str());
	return result;
}

String _sqlite3::_Sqlite3_Table_::FullName() const
{
	String Head;
	if (Database.size())
		Head = _T("\"") + Database + _T("\".");
	return Head + _T("\"") + Name + _T("\"");
}
//--------------以下是单元测试-----------------------
#include "gtest/gtest.h"

DECLARE_TABLE_CLASS(edoyunLogin_user_test, _Sqlite3_Table_)
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

TEST(Database, user_table)
{
	int ret = 0;
	DataBaseClient* client = new Sqlite3Client();
	ASSERT_NE((DataBaseClient*)NULL, client);
	TableList result;
	edoyunLogin_user_test user;
	ret = client->Connect(StrMap({ {_T("host"),_T("sqlite3.db")} }));
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
			SLParam(_T("\"1817619619\""), _T("\"冯老师\""), _T("\"易绝顶\""), _T("\"123456\""), _T("18888888888"))
		)
	);
	EXPECT_EQ(0, ret);
	ret = client->Exec(
		user.Insert(
			SLParam(_T("user_qq"), _T("user_name"), _T("user_nick"), _T("user_password"), _T("user_phone")),
			SLParam(_T("\"123456789\""), _T("\"test\""), _T("\"test\""), _T("\"123456\""), _T("18888888888"))
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
					SLParam(_T("\"123456789\""), _T("\"test\""), _T("\"test\""), _T("\"123456\""))
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