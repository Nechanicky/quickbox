#ifndef QF_QMLWIDGETS_COMBOBOX_H
#define QF_QMLWIDGETS_COMBOBOX_H

#include "qmlwidgetsglobal.h"
#include "idatawidget.h"

#include <qf/core/utils.h>

#include <QComboBox>

namespace qf {
namespace qmlwidgets {

class QFQMLWIDGETS_DECL_EXPORT ComboBox : public QComboBox, public IDataWidget
{
	Q_OBJECT
	Q_PROPERTY(QVariant dataValue READ dataValue WRITE setDataValue NOTIFY dataValueChanged)
	Q_PROPERTY(QString dataId READ dataId WRITE setDataId)
	Q_PROPERTY(bool valueRestrictedToItems READ isValueRestrictedToItems WRITE setValueRestrictedToItems NOTIFY valueRestrictedToItemsChanged)
private:
	typedef QComboBox Super;
public:
	ComboBox(QWidget *parent = nullptr);
	~ComboBox() Q_DECL_OVERRIDE;

	QF_PROPERTY_BOOL_IMPL2(v, V , alueRestrictedToItems, true)
protected:
	QVariant dataValue() Q_DECL_OVERRIDE;
	void setDataValue(const QVariant &val) Q_DECL_OVERRIDE;
	Q_SIGNAL void dataValueChanged(const QVariant &new_val);

	virtual void removeItems();
	virtual void loadItems() {}

	Q_SLOT virtual void onCurrentTextChanged(const QString &txt);
protected:
	bool m_loadingState = false;
};

class QFQMLWIDGETS_DECL_EXPORT ForeignKeyComboBox : public ComboBox
{
	Q_OBJECT
	Q_PROPERTY(QString referencedTable READ referencedTable WRITE setReferencedTable NOTIFY referencedTableChanged)
	Q_PROPERTY(QString referencedField READ referencedField WRITE setReferencedField NOTIFY referencedFieldChanged)
	Q_PROPERTY(QString referencedCaptionField READ referencedCaptionField WRITE setReferencedCaptionField NOTIFY referencedCaptionFieldChanged)
	Q_PROPERTY(QString itemCaptionFormat READ itemCaptionFormat WRITE setItemCaptionFormat NOTIFY itemCaptionFormatChanged)
	Q_PROPERTY(QString queryString READ queryString WRITE setQueryString NOTIFY queryStringChanged)
private:
	typedef ComboBox Super;
public:
	ForeignKeyComboBox(QWidget *parent = nullptr);
	~ForeignKeyComboBox() Q_DECL_OVERRIDE;

	QF_PROPERTY_IMPL(QString, r, R, eferencedTable)
	QF_PROPERTY_IMPL(QString, r, R, eferencedField)
	QF_PROPERTY_IMPL(QString, r, R, eferencedCaptionField)
	QF_PROPERTY_IMPL2(QString, i, I, temCaptionFormat, QStringLiteral("{{captionField}}"))
	QF_PROPERTY_IMPL(QString, q, Q, ueryString)
protected:
	void removeItems() Q_DECL_OVERRIDE;
	void loadItems() Q_DECL_OVERRIDE;
private:
	bool m_itemsLoaded = false;
};

}}

#endif // COMBOBOX_H
