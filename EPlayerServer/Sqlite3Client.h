#pragma once
#include "DatabaseHelper.h"
#include "sqlite3.h"
#include <sstream>
using namespace edoyun;

namespace _sqlite3 {
	class Sqlite3Client :
		public edoyun::DataBaseClient
	{
	public:
		Sqlite3Client();
		virtual ~Sqlite3Client();
		virtual int Connect(const StrMap& args);
		virtual int Exec(const String& sql);
		virtual int Exec(const String& sql, TableList& result,
			const _Table_& table);
		virtual int StartTransaction();
		virtual int CommitTransaction();
		virtual int RollabckTransaction();
		virtual bool IsConnected();
		virtual int Close();
	public:
		class Param {
		public:
			Param(Sqlite3Client* thiz, const _Table_& table, TableList& result)
				:m_table(table), m_client(thiz), m_result(result) {}
		public:
			const _Table_& m_table;
			Sqlite3Client* m_client;
			TableList& m_result;
		};
		static int SelectCallbackEntry(void* param, int nCol, char** values, char** names);
		int SelectCallback(int nCol, char* values[], char* names[], Sqlite3Client::Param* param);
	private:
		sqlite3_stmt* m_stmt;
		sqlite3* m_db;
	};

	class _Sqlite3_Table_ :
		public _Table_
	{
	public:
		virtual String Create();
		virtual String Insert(const SLParam& Columns, const SLParam& Values);
		virtual String Drop();
		virtual String Modify(const SLParam& Columns, const SLParam& Values);
		virtual String Query();
		virtual String FullName() const;
		virtual std::shared_ptr<_Table_> Copy() const = 0;
	};

	//sqlite3的基本类型
	template<typename T>
	class _DeclareSqlite3Field :public _Field_
	{
	public:
		_DeclareSqlite3Field() :_Field_() { value = T(); }
		_DeclareSqlite3Field(const _DeclareSqlite3Field& field) :_Field_(field) {
			value = field.value;
		}
		virtual ~_DeclareSqlite3Field() {}
		virtual _Field_& operator=(const _Field_& field) {
			if (this != &field) {
				_Field_::operator=(field);
				value = dynamic_cast<const _DeclareSqlite3Field<T>&>(field).value;
			}
			return *this;
		}
		virtual void FromString(const String& sValue) = 0;
		T& Value() { return value; }
		virtual String Create() {
			String sql = String(_T("\"")) + Name + _T("\" ") + Type + _T(" ");
			if (Attr & NOT_NULL) {
				sql += _T(" NOT NULL ");
			}
			if (Attr & DEFAULT && (Default.size() > 0)) {
				sql += _T(" DEFAULT ") + Default + _T(" ");
			}
			if (Attr & UNIQUE) {
				sql += _T(" UNIQUE ");
			}
			if (Attr & PRIMARY_KEY) {
				sql += _T(" PRIMARY KEY ");
			}
			if (Attr & CHECK && (Check.size() > 0)) {
				sql += _T(" CHECK(") + Check + _T(") ");
			}
			if (Attr & AUTOINCREMENT) {
				sql += _T(" AUTOINCREMENT ");
			}
			return sql;
		}
		virtual String edoyun::_Field_::FullName() const {
			return String(_T("\"")) + Name + String(_T("\""));
		}
	public:
		T value;
	};

	template<>
	class _DeclareSqlite3Field<void> :public _Field_
	{
	public:
		_DeclareSqlite3Field() :_Field_() { value = nullptr; }
		_DeclareSqlite3Field(const _DeclareSqlite3Field& field) :_Field_(field) {
			value = nullptr;
		}
		virtual ~_DeclareSqlite3Field() {}
		virtual _DeclareSqlite3Field& operator=(const _DeclareSqlite3Field& field) {
			return *this;
		}
		virtual void FromString(const String& sValue) {};
		void* Value() { return value; }
		virtual String Create() {
			String sql = String(_T("\"")) + Name + _T("\" ") + Type + _T(" ");
			if (Attr & NOT_NULL) {
				sql += _T(" NOT NULL ");
			}
			if (Attr & DEFAULT && (Default.size() > 0)) {
				sql += _T(" DEFAULT ") + Default + _T(" ");
			}
			if (Attr & UNIQUE) {
				sql += _T(" UNIQUE ");
			}
			if (Attr & PRIMARY_KEY) {
				sql += _T(" PRIMARY KEY ");
			}
			if (Attr & CHECK && (Check.size() > 0)) {
				sql += _T(" CHECK(") + Check + _T(") ");
			}
			if (Attr & AUTOINCREMENT) {
				sql += _T(" AUTOINCREMENT ");
			}
			return sql;
		}
		virtual String edoyun::_Field_::FullName() const {
			return String(_T("\"")) + Name + String(_T("\""));
		}
	public:
		void* value;
	};

	class NULL_FILED :public _DeclareSqlite3Field<void>
	{
	public:
		NULL_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :
			_DeclareSqlite3Field<void>()
		{
			Name = name;
			Attr = attr;
			Type = "NULL" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {}
		virtual String toEqualExp() const { return FullName() + _T("= NULL"); }
		virtual String toString() const { return _T("NULL"); }
	};

	class INTEGER_FILED :public _DeclareSqlite3Field<int>
	{
	public:
		INTEGER_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<int>()
		{
			Name = name;
			Attr = attr;
			Type = "INTEGER" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			sscanf_s(sValue.c_str(), "%d", &value);
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << value;
			return ss.str();
		}
		virtual String toString() const { 
			StrStream ss;
			ss << value;
			return ss.str(); 
		}
	};

	class DATETIME_FILED :public _DeclareSqlite3Field<int>
	{
	public:
		DATETIME_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<int>()
		{
			Name = name;
			Attr = attr;
			Type = "DATETIME" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			sscanf_s(sValue.c_str(), "%d", &value);
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << value;
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << value;
			return ss.str();
		}
	};

	class BOOL_FILED :public _DeclareSqlite3Field<bool>
	{
	public:
		BOOL_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<bool>()
		{
			Name = name;
			Attr = attr;
			Type = "BOOL" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			int data = 0;
			sscanf_s(sValue.c_str(), "%d", &data);
			value = data != 0;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << value;
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << (int)value;
			return ss.str();
		}
	};

	class REAL_FILED :public _DeclareSqlite3Field<double>
	{
	public:
		REAL_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<double>()
		{
			Name = name;
			Attr = attr;
			Type = "REAL" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			sscanf_s(sValue.c_str(), "%lf", &value);
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << value;
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << value;
			return ss.str();
		}
	};

	class TEXT_FILED :public _DeclareSqlite3Field<String>
	{
	public:
		TEXT_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<String>()
		{
			Name = name;
			Attr = attr;
			Type = "TEXT" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			value = sValue;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << _T("\"") + value + _T("\"");
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << _T("\"") + value + _T("\"");
			return ss.str();
		}
	};

	class VARCHAR_FILED :public _DeclareSqlite3Field<String>
	{
	public:
		VARCHAR_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<String>()
		{
			Name = name;
			Attr = attr;
			Type = "VARCHAR" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			value = sValue;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << _T("\"") + value + _T("\"");
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << _T("\"") + value + _T("\"");
			return ss.str();
		}
	};

	class BLOB_FILED :public _DeclareSqlite3Field<String>
	{
	public:
		BLOB_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareSqlite3Field<String>()
		{
			Name = name;
			Attr = attr;
			Type = "BLOB" + size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			value = sValue;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << _T("\"") + Str2Hex(value) + _T("\"");
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << _T("\"") + Str2Hex(value) + _T("\"");
			return ss.str();
		}
		String Str2Hex(const String& data) const
		{
			const String hex = "0123456789ABCDEF";
			StrStream ss;
			for (char ch : data)
				ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xf];
			return ss.str();
		}
	};

#define TOSTR(field) dynamic_cast<_DeclareSqlite3Field<String>*>(field.get())->value
#define TOINT(field) dynamic_cast<_DeclareSqlite3Field<int>*>(field.get())->value
#define TODOUBLE(field) dynamic_cast<_DeclareSqlite3Field<double>*>(field.get())->value
#define TOBOOL(field) dynamic_cast<_DeclareSqlite3Field<bool>*>(field.get())->value
}
