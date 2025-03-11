#pragma once
#include "DatabaseHelper.h"
#include <mysql.h>
#include <sstream>
#include <memory>
using namespace edoyun;

namespace mysql {
	class MysqlClient :
		public DataBaseClient
	{
	public:
		MysqlClient();
		virtual ~MysqlClient();
		virtual int Connect(const StrMap& args);
		virtual int Exec(const String& sql);
		virtual int Exec(const String& sql, TableList& result,
			const _Table_& table);
		virtual int StartTransaction();
		virtual int CommitTransaction();
		virtual int RollabckTransaction();
		virtual bool IsConnected();
		virtual int Close();
	private:
		std::shared_ptr<MYSQL> m_mysql;
	};

	class _Mysql_Table_ :
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

	template<typename T>
	class _DeclareMysqlField :public _Field_
	{
	public:
		_DeclareMysqlField() :_Field_() {
			value = T();
		}
		_DeclareMysqlField(const _DeclareMysqlField& field) :_Field_(field) {
			value = field.value;
		}
		virtual ~_DeclareMysqlField() {}
		virtual _Field_& operator=(const _Field_& field) {
			if (this != &field) {
				_Field_::operator=(field);
				value = dynamic_cast<const _DeclareMysqlField<T>&>(field).value;
			}
			return *this;
		}
		virtual void FromString(const String& sValue) = 0;
		T& Value() { return value; }
		virtual String Create() {
			String sql = String(_T("`")) + Name + _T("` ") + Type + Size + _T(" ");
			if (Attr & NOT_NULL) {
				sql += _T(" NOT NULL ");
			}
			else {
				sql += _T(" NULL ");
			}
			//mysql规定 BLOB, TEXT, GEOMETRY 或 JSON 不能有默认值！！！
			if (Type != _T("TEXT") &&
				(Type != _T("BLOB")) &&
				(Type != _T("GEOMETRY") &&
					(Type != _T("JSON"))))
			{
				if (Attr & DEFAULT && (Default.size() > 0)) {
					sql += _T(" DEFAULT ") + Default + _T(" ");
				}
			}

			if (Attr & UNIQUE) {
				//mysql要特殊处理
			}
			if (Attr & PRIMARY_KEY) {
				//mysql主键要特殊处理
			}
			if (Attr & CHECK && (Check.size() > 0)) {
				//mysql 不支持check
			}
			if (Attr & AUTOINCREMENT) {
				sql += _T(" AUTO_INCREMENT ");
			}
			return sql;
		}
		virtual String FullName() const {
			return String(_T("`")) + Name + String(_T("`"));
		}
	public:
		T value;
	};

	template<>
	class _DeclareMysqlField<void> :public _Field_
	{
	public:
		_DeclareMysqlField() :_Field_() { value = nullptr; }
		_DeclareMysqlField(const _DeclareMysqlField& field) :_Field_(field) {
			value = nullptr;
		}
		virtual ~_DeclareMysqlField() {}
		virtual _DeclareMysqlField& operator=(const _DeclareMysqlField& field) {
			return *this;
		}
		virtual void FromString(const String& sValue) = 0;
		void* Value() { return value; }
		virtual String FullName() const {
			return String(_T("'")) + Name + String(_T("'"));
		}
	public:
		void* value;
	};

	class NULL_FILED :public _DeclareMysqlField<void>
	{
	public:
		NULL_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :
			_DeclareMysqlField<void>()
		{
			Name = name;
			Attr = attr;
			Type = "NULL";
			Size = size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {}
		virtual String toEqualExp() const { return FullName() + _T("= NULL"); }
		virtual String toString() const { return _T("NULL"); }
	};

	class INTEGER_FILED :public _DeclareMysqlField<int>
	{
	public:
		INTEGER_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<int>()
		{
			Name = name;
			Attr = attr;
			Type = "INTEGER";
			Size = size;
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

	class DATETIME_FILED :public _DeclareMysqlField<struct tm>
	{
	public:
		DATETIME_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<struct tm>()
		{
			Name = name;
			Attr = attr;
			Type = "DATETIME";
			Size = size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			sscanf_s(sValue.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
				&value.tm_year, &value.tm_mon, &value.tm_mday,
				&value.tm_hour, &value.tm_min, &value.tm_sec);
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << toString();
			return ss.str();
		}
		virtual String toString() const {
			TCHAR buffer[64] = _T("");
			Snprintf(buffer, sizeof(buffer) / sizeof(TCHAR),
				_T("'%04d-%02d-%02d %02d:%02d:%02d'"),
				value.tm_year, value.tm_mon, value.tm_mday,
				value.tm_hour, value.tm_min, value.tm_sec);
			return buffer;
		}
	};

	class BOOL_FILED :public _DeclareMysqlField<bool>
	{
	public:
		BOOL_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<bool>()
		{
			Name = name;
			Attr = attr;
			Type = "BOOL";
			Size = size;
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

	class REAL_FILED :public _DeclareMysqlField<double>
	{
	public:
		REAL_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<double>()
		{
			Name = name;
			Attr = attr;
			Type = "REAL";
			Size = size;
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

	class TEXT_FILED :public _DeclareMysqlField<String>
	{
	public:
		TEXT_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<String>()
		{
			Name = name;
			Attr = attr;
			Type = "TEXT";
			Size = size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			value = sValue;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << _T("'") + value + _T("'");
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << _T("'") + value + _T("'");
			return ss.str();
		}
	};

	class VARCHAR_FILED :public _DeclareMysqlField<String>
	{
	public:
		VARCHAR_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<String>()
		{
			Name = name;
			Attr = attr;
			Type = "VARCHAR";
			Size = size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			value = sValue;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << _T("'") + value + _T("'");
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << _T("'") + value + _T("'");
			return ss.str();
		}
	};

	class BLOB_FILED :public _DeclareMysqlField<String>
	{
	public:
		BLOB_FILED(const String& name, int attr, const String size, const String& default_, const String& check) :_DeclareMysqlField<String>()
		{
			Name = name;
			Attr = attr;
			Type = "BLOB";
			Size = size;
			Default = default_;
			Check = check;
		}
		virtual void FromString(const String& sValue) {
			value = sValue;
		}
		virtual String toEqualExp() const {
			StrStream ss;
			ss << FullName() + _T(" = ");
			ss << _T("'") + Str2Hex(value) + _T("'");
			return ss.str();
		}
		virtual String toString() const {
			StrStream ss;
			ss << _T("'") + Str2Hex(value) + _T("'");
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

#define TOSTR(field) dynamic_cast<_DeclareMysqlField<String>*>(field.get())->value
#define TOINT(field) dynamic_cast<_DeclareMysqlField<int>*>(field.get())->value
#define TODOUBLE(field) dynamic_cast<_DeclareMysqlField<double>*>(field.get())->value
#define TOBOOL(field) dynamic_cast<_DeclareMysqlField<bool>*>(field.get())->value
}
