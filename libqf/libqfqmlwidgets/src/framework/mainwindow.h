#ifndef QF_QMLWIDGETS_FRAMEWORK_MAINWINDOW_H
#define QF_QMLWIDGETS_FRAMEWORK_MAINWINDOW_H

#include "../qmlwidgetsglobal.h"
#include "ipersistentsettings.h"

#include <QMainWindow>
#include <QMap>

namespace qf {
namespace core {
class Settings;
}
}
namespace qf {
namespace qmlwidgets {

class MenuBar;
class StatusBar;

namespace framework {

class Application;
class PluginLoader;
class DockWidget;
class PartWidget;
class CentralWidget;
class Plugin;

class QFQMLWIDGETS_DECL_EXPORT MainWindow : public QMainWindow, public IPersistentSettings
{
	Q_OBJECT
	Q_PROPERTY(qf::qmlwidgets::MenuBar* menuBar READ menuBar)
	Q_PROPERTY(qf::qmlwidgets::StatusBar* statusBar READ statusBar)
	Q_PROPERTY(QString persistentSettingsId READ persistentSettingsId WRITE setPersistentSettingsId)
private:
	typedef QMainWindow Super;
public:
	typedef QMap<QString, QObject*> PluginMap;
public:
	explicit MainWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow() Q_DECL_OVERRIDE;
public:
	CentralWidget* centralWidget();
	void setCentralWidget(CentralWidget *widget);
public:
	void loadPlugins();
	/// framework API
	Q_INVOKABLE void setPersistentSettingDomains(const QString &organization_domain, const QString &organization_name, const QString &application_name = QString());

	MenuBar* menuBar();

	StatusBar* statusBar();
	Q_INVOKABLE void setStatusBar(qf::qmlwidgets::StatusBar *sbar);

	Q_INVOKABLE void addDockWidget(Qt::DockWidgetArea area, qf::qmlwidgets::framework::DockWidget *dockwidget);
	Q_INVOKABLE void addPartWidget(qf::qmlwidgets::framework::PartWidget *widget, const QString &feature_id = QString());

	Q_INVOKABLE qf::qmlwidgets::framework::Plugin* plugin(const QString &feature_id);
	Q_INVOKABLE qf::qmlwidgets::framework::Plugin* pluginForObject(QObject *qml_object);

	Q_SLOT void loadPersistentSettings();

	Q_SLOT void showProgress(const QString &msg, int completed, int total);
	Q_SIGNAL void progress(const QString &msg, int completed, int total);

	/// emitted by plugin loader when all plugins are loaded
	Q_SIGNAL void pluginsLoaded();
	Q_SIGNAL void aboutToClose();
#ifdef GET_RESOURCE_IN_FRAMEWORK
	Q_INVOKABLE int getResource(const QUrl &url, bool show_progress = true);
	Q_SIGNAL void getResourceContentFinished(int request_id, bool ok, const QString &content);
#endif
	static MainWindow* frameWork();

	//Q_INVOKABLE QObject* obj_testing();
protected:
	void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
private:
	Q_SLOT void savePersistentSettings();
	Q_SLOT void whenPluginsLoaded();
	//void setupSettingsPersistence();
private:
	PluginLoader *m_pluginLoader;
	qf::qmlwidgets::StatusBar* m_statusBar;
	static MainWindow *self;
};

}}}

#endif // QF_QMLWIDGETS_FRAMEWORK_MAINWINDOW_H
