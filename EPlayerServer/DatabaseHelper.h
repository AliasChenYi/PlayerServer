#pragma once
#include <string>
#include <map>
#include <vector>
#include <list>
#include <tchar.h>
#include <memory>

namespace edoyun {
	enum {
		NONE,//无属性
		NOT_NULL,//非空
		DEFAULT = 2,//默认值
		UNIQUE = 4,//唯一
		PRIMARY_KEY = 8,//主键
		CHECK = 16,//约束
		AUTOINCREMENT = 32//自动增长
	};
#ifdef _MBCS
	using String = std::string;
	using StrStream = std::stringstream;
#else
	using String = std::wstring;
	using StrStream = std::wstringstream;
#endif
	using StrMap = std::map<String, String>;
	using StrList = std::list<String>;
	template<typename Value>
	class ValueTool {
		//判断是否存在toString
		template<typename U, String(U::*)()> struct HELP;
		template<typename U> static char Test(HELP<U, &U::toString>*);
		template<typename U> static int Test(...);
		//判断是否存在c_str
		template<typename U, const char* (U::*)()const> struct HELP1;
		template<typename U> static char Test1(HELP1<U, &U::toString>*);
		template<typename U> static int Test1(...);
	public:
		const static bool has_toString = sizeof(Test<Value>(0)) == sizeof(char);
		const static bool has_c_str = sizeof(Test1<Value>(0) == sizeof(char));
		static std::enable_if<has_toString, String>
			toString(const Value& v) {
			return v.toString();
		}

		static std::enable_if<has_c_str, String>
			toString(const Value& v) {
			return _T("\"") + v + _T("\"");
		}

		String toString(const char* v) {
			return _T("\"") + String(v) + _T("\"");
		}

		static std::enable_if<!has_toString && !has_c_str, String>
			toString(const Value& v) {
			StrStream s;
			s << v;
			return s.str();
		}
	};

	using CParamIter = StrList::const_iterator;
	using ParamIter = StrList::iterator;

	struct StrListParam {
		template<typename ... _ARGS_>
		StrListParam(const _ARGS_& ... args) {
			int _[] = { (Append(args), 0)... };
		}
		StrListParam(const StrListParam& slp) {
			lstParams.insert(lstParams.end(), slp.lstParams.begin(), slp.lstParams.end());
		}
		StrListParam& operator=(const StrListParam& slp) {
			if (this != &slp) {
				lstParams.insert(lstParams.end(), slp.lstParams.begin(), slp.lstParams.end());
			}
			return *this;
		}
		CParamIter begin() const { return lstParams.begin(); }
		ParamIter begin() { return lstParams.begin(); }
		CParamIter end() const { return lstParams.end(); }
		ParamIter end() { return lstParams.end(); }
		size_t size() const { return lstParams.size(); }
		StrList lstParams;
	private:
		template<typename TAIL>
		void Append(const TAIL& tail) { lstParams.push_back(tail); }
	};
	using SLParam = struct StrListParam;


#define _V(X) ValueTool::toString((X))


	struct _Field_ {
	public:
		virtual String Create() = 0;
		_Field_() { Attr = NONE; }
		_Field_(const _Field_& field);
		virtual ~_Field_() { Attr = NONE; }
		virtual _Field_& operator=(const _Field_& field);
		virtual void FromString(const String& sValue) = 0;
		virtual String toEqualExp() const = 0;
		virtual String FullName() const = 0;
		virtual String toString() const = 0;
	public:
		String Name;
		String Type;
		String Size;
		unsigned Attr;
		String Default;
		String Check;
	};

	using FieldArray = std::vector<std::shared_ptr<_Field_>>;
	using FieldMap = std::map<String, std::shared_ptr<_Field_>>;
	using FMapIter = FieldMap::iterator;
	using FMapCIter = FieldMap::const_iterator;
	using PField = std::shared_ptr<_Field_>;

	struct _Table_ {
	public:
		_Table_() {}
		_Table_(const _Table_& table) {
			Database = table.Database;
			Name = table.Name;
			Columns = table.Columns;
			Fields = table.Fields;
		}
		virtual String Create() = 0;
		virtual String Insert(const SLParam& Columns, const SLParam& Values) = 0;
		virtual String Drop() = 0;
		virtual String Modify(const SLParam& Columns, const SLParam& Values) = 0;
		virtual String Query() = 0;
		virtual String FullName() const = 0;
		virtual std::shared_ptr<_Table_> Copy() const = 0;
	public:
		String Database;//表所属的库
		String Name;
		FieldArray Columns;
		FieldMap Fields;
	};

	using PTable = std::shared_ptr<_Table_>;
	using TableArray = std::vector<PTable>;
	using TableList = std::list<PTable>;

#define DECLARE_TABLE_CLASS(name, base) \
class name:public base { \
public: \
virtual PTable Copy() const{ return PTable(new name(*this));} \
name():base(){Name = _T(#name);

#define DECLARE_ITEM(name, attr, type, size, Default, check) \
{PField field(new type(_T(#name), attr, _T(size), _T(Default), _T(check))); \
Columns.push_back(field); \
Fields.insert(std::pair<String,PField>(_T(#name),field));}

#define DECLARE_TABLE_CLASS_END() }};

	class DataBaseClient
	{
	public:
		DataBaseClient() {}
		virtual ~DataBaseClient() {}
		virtual int Connect(const StrMap& args) = 0;
		virtual int Exec(const String& sql) = 0;
		virtual int Exec(const String& sql, TableList& result,
			const _Table_& table) = 0;
		virtual int StartTransaction() = 0;
		virtual int CommitTransaction() = 0;
		virtual int RollabckTransaction() = 0;
		virtual bool IsConnected() = 0;
		virtual int Close() = 0;
	};
}

#ifndef Snprintf
#ifndef _UNICODE
#define Snprintf snprintf
#else
#define Snprintf _snwprintf
#endif
#endif