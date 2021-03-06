#ifndef QF_CORE_UTILS_H
#define QF_CORE_UTILS_H

#include "coreglobal.h"
#include "assert.h"

#include <QVariant>

#define QF_SAFE_DELETE(x) if(x != nullptr) {delete x; x = nullptr;}

#define QF_SARG(s) "'" + QString(s) + "'"
#define QF_IARG(i) "" + QString::number(i) + ""

#define QF_QUOTEME(x) QStringLiteral(#x)

#define QF_PROPERTY_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	private: ptype m_##lower_letter##name_rest = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const ptype &new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT void set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
		} \
	}
#define QF_PROPERTY_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	private: ptype m_##lower_letter##name_rest; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const ptype &new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT void set##upper_letter##name_rest(const ptype &val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
		} \
	}
#define QF_PROPERTY_OBJECT_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	private: ptype m_##lower_letter##name_rest = nullptr; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(ptype new_val); \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT void set##upper_letter##name_rest(ptype val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			if(m_##lower_letter##name_rest != nullptr) \
				if(m_##lower_letter##name_rest->parent() == nullptr) \
					m_##lower_letter##name_rest->setParent(this); \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
		}\
	}
#define QF_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	private: bool m_##lower_letter##name_rest = default_value; \
	public: Q_SIGNAL void lower_letter##name_rest##Changed(const bool &new_val); \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: Q_SLOT void set##upper_letter##name_rest(bool val) { \
		if(m_##lower_letter##name_rest != val) { \
			m_##lower_letter##name_rest = val; \
			emit lower_letter##name_rest##Changed(m_##lower_letter##name_rest); \
		}\
	}
#define QF_PROPERTY_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	QF_PROPERTY_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)

#define QF_OPTION_FIELD_RW(ptype, getter_prefix, setter_prefix, name_rest) \
	public: ptype getter_prefix##name_rest() const {return qvariant_cast<ptype>(value(QF_QUOTEME(getter_prefix##name_rest)));} \
	public: void setter_prefix##name_rest(const ptype &val) {(*this)[QF_QUOTEME(getter_prefix##name_rest)] = val;}
/// for default values other than QVariant()
#define QF_OPTION_FIELD2_RW(ptype, getter_prefix, setter_prefix, name_rest, default_value) \
	public: ptype getter_prefix##name_rest() const {return qvariant_cast<ptype>(value(QF_QUOTEME(getter_prefix##name_rest), default_value));} \
	public: void setter_prefix##name_rest(const ptype &val) {(*this)[QF_QUOTEME(getter_prefix##name_rest)] = val;}

/// for implicitly shared classes properties
#define QF_SHARED_CLASS_FIELD_RW(ptype, getter_prefix, setter_prefix, name_rest) \
	public: ptype getter_prefix##name_rest() const {return d->getter_prefix##name_rest;} \
	public: void setter_prefix##name_rest(const ptype &val) {d->getter_prefix##name_rest = val;}

class QString;

namespace qf {
namespace core {

class QFCORE_DECL_EXPORT Utils
{
public:
	static const QString &nullValueString();
	static void parseFieldName(const QString& full_field_name, QString *pfield_name = NULL, QString *ptable_name = NULL, QString *pdb_name = NULL);
	static QString composeFieldName(const QString &field_name, const QString &table_name = QString(), const QString &db_name = QString());
	/// @returns: True if @a field_name1 ends with @a field_name2. Comparision is case insensitive
	static bool fieldNameEndsWith(const QString &field_name1, const QString &field_name2);
	static bool fieldNameCmp(const QString &fld_name1, const QString &fld_name2);
	static QVariant retypeVariant(const QVariant &_val, QVariant::Type type);

	static bool invokeMethod_B_V(QObject *obj, const char *method_name);

	template <class T>
	static T findParent(const QObject *_o, bool throw_exc = qf::core::Exception::Throw)
	{
		T t = NULL;
		QObject *o = const_cast<QObject*>(_o);
		while(o) {
			o = o->parent();
			if(!o)
				break;
			t = qobject_cast<T>(o);
			if(t)
				break;
		}
		if(!t && throw_exc) {
			QF_EXCEPTION(QString("object 0x%1 has not any parent of requested type.").arg((ulong)_o, 0, 16));
		}
		return t;
	}
};

}}

#endif
