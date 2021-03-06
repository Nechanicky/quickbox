#include "reportdocument.h"

#include <qf/core/log.h>
#include <qf/core/string.h>
//#include <qf/core/exception.h>
//#include <qf/core/utils/fileutils.h>

//#include <QPair>
//#include <QFile>
#include <QUrl>

namespace qfc = qf::core;
//namespace qfu = qf::core::utils;

using namespace qf::qmlwidgets::reports;

ReportDocument::ReportDocument(QQmlEngine *engine, QObject *parent) :
	Super(engine, parent)
{
}

void ReportDocument::setFileName(const QString &fname)
{
	qfLogFuncFrame() << fname;
	QString fn = fname;
	if(fn.startsWith(QLatin1String(":/")))
		fn = "qrc" + fn;
	QUrl url(fn);
	qDebug() << "loading url:" << url.toString();
	loadUrl(url);
}
