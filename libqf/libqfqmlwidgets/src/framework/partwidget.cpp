#include "partwidget.h"
#include "plugin.h"
#include "mainwindow.h"
#include "../frame.h"
#include "../dialogs/internal/captionframe.h"

#include <qf/core/log.h>
#include <qf/core/utils.h>

#include <QVBoxLayout>
#include <QIcon>

using namespace qf::qmlwidgets::framework;

PartWidget::PartWidget(QWidget *parent) :
	Super(parent), IPersistentSettings(this)
{
	qfLogFuncFrame();
	m_captionFrame = new qf::qmlwidgets::dialogs::internal::CaptionFrame(this);
	m_centralFrame = new Frame(this);
	m_centralFrame->setLayoutType(Frame::LayoutVertical);
	QBoxLayout *ly = new QVBoxLayout(this);
	ly->setMargin(0);
	ly->addWidget(m_captionFrame);
	qfDebug() << "\t adding:" << m_centralFrame << "to layout:" << ly;
	ly->addWidget(m_centralFrame);
	setLayout(ly);
}

PartWidget::~PartWidget()
{
	qfLogFuncFrame() << this << "title:" << title();
	qDeleteAll(m_attachedObjects);
	m_attachedObjects.clear();
}

void PartWidget::setTitle(QString arg)
{
	if (m_title != arg) {
		m_title = arg;
		emit titleChanged(arg);
	}
}

void PartWidget::setIconSource(QString arg)
{
	if (m_iconSource != arg) {
		m_iconSource = arg;
		emit iconSourceChanged(arg);
	}
}

void PartWidget::setFeatureId(QString id)
{
	if (m_featureId != id) {
		m_featureId = id;
		//setProperty("persistentSettingsId", id);
		setPersistentSettingsId(id);
		//setObjectName(id);
		emit featureIdChanged(id);
	}
}

void PartWidget::updateCaptionFrame()
{
	m_captionFrame->setText(m_title);
	//m_captionFrame->setIconSource(this->iconSource());
	//QString feature_id = featureId();
	QIcon ico = createIcon();
	m_captionFrame->setIcon(ico);
}

QIcon PartWidget::createIcon()
{
	QIcon ico;
	QString feature_id = featureId();
	if(feature_id.isEmpty()) {
		qfWarning() << "featureId property of part widget is empty, default icon will be set.";
	}
	else {
		Plugin *plugin = MainWindow::frameWork()->plugin(feature_id);
		if(!plugin) {
			qfWarning() << "Cannot found plugin for part featureId:" << feature_id << ", default icon will be set.";
		}
		else {
			QString icon_path = iconSource();
			if(icon_path.isEmpty())
				icon_path = "images/feature.png";
			if(!icon_path.startsWith(":/")) {
				icon_path = plugin->manifest()->homeDir() + "/" + icon_path;
			}
			QPixmap pm(icon_path);
			if(pm.isNull())
				qfWarning() << "Cannot load icon on path:" << icon_path;
			else
				ico = QIcon(pm);
		}
	}
	if(ico.isNull())
		ico = QIcon(":/qf/qmlwidgets/images/under-construction.png");
	return ico;
}

QQmlListProperty<QObject> PartWidget::attachedObjects()
{
	return QQmlListProperty<QObject>(this, m_attachedObjects);
}

/*
bool PartWidget::canActivate(bool active_on)
{
	qfLogFuncFrame() << this << active_on;
	Q_UNUSED(active_on);
	return true;
}
*/
QQmlListProperty<QWidget> PartWidget::widgets()
{
	return m_centralFrame->widgets();
}

void PartWidget::loadPersistentSettings()
{
	qfLogFuncFrame();
	QString path = persistentSettingsPath();
	qfDebug() << "\t" << this << path;
	/*
	if(!path.isEmpty()) {
		QSettings settings;
		settings.beginGroup(path);
		QRect geometry = settings.value("geometry").toRect();
		if(geometry.isValid()) {
			this->setGeometry(geometry);
		}
	}
	*/
}

void PartWidget::savePersistentSettings()
{
}

void PartWidget::classBegin()
{
}

void PartWidget::componentComplete()
{
}

