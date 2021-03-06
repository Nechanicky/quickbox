#ifndef QF_CORE_QML_SETTINGS_H
#define QF_CORE_QML_SETTINGS_H

#include <QSettings>

namespace qf {
namespace core {
namespace qml {

class Settings : public QSettings
{
	Q_OBJECT
private:
	typedef QSettings Super;
public:
	explicit Settings(QObject *parent = 0);
	~Settings() Q_DECL_OVERRIDE;
public slots:
	void beginGroup(const QString & prefix) {Super::beginGroup(prefix);}
	int beginReadArray(const QString & prefix) {return Super::beginReadArray(prefix);}
	void beginWriteArray(const QString & prefix, int size = -1) {Super::beginWriteArray(prefix, size);}
	void endArray() {Super::endArray();}
	void endGroup() {Super::endGroup();}
	void setArrayIndex(int i) {Super::setArrayIndex(i);}
	QString group() const {return Super::group();}

	void setValue(const QString & key, const QVariant & value);
	QVariant value(const QString & key, const QVariant & default_value = QVariant()) const;

	void sync() {Super::sync();}
};

}}}

#endif
