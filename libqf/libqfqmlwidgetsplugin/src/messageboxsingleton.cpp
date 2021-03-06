#include "messageboxsingleton.h"

#include <QQmlEngine>
#include <QMessageBox>
#include <QApplication>

MessageBoxSingleton::MessageBoxSingleton(QObject *parent) :
	QObject(parent)
{
}

QObject *MessageBoxSingleton::singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
	Q_UNUSED(scriptEngine)
	MessageBoxSingleton *s = new MessageBoxSingleton(engine);
	return s;
}

void MessageBoxSingleton::aboutQt(QWidget *parent, const QString &title)
{
	if(parent == nullptr) {
		parent = QApplication::activeWindow();
	}
	QMessageBox::aboutQt(parent, title);
}

int MessageBoxSingleton::critical(QWidget *parent, const QString &text)
{
	if(parent == nullptr) {
		parent = QApplication::activeWindow();
	}
	return QMessageBox::critical(parent, trUtf8("Error"), text);
}
