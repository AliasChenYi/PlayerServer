#include "DatabaseHelper.h"
#include "LogServer.h"

using namespace edoyun;


edoyun::_Field_::_Field_(const _Field_& field)
{
	Name = field.Name;
	Type = field.Type;
	Size = field.Size;
	Attr = field.Attr;
	Default = field.Default;
	Check = field.Check;
}
_Field_& edoyun::_Field_::operator=(const _Field_& field)
{
	if (this != &field) {
		Name = field.Name;
		Type = field.Type;
		Size = field.Size;
		Attr = field.Attr;
		Default = field.Default;
		Check = field.Check;
	}
	return *this;
}

