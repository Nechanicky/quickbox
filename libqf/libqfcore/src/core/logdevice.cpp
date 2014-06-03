#include "logdevice.h"

#include <QByteArray>
#include <QString>
#include <QDateTime>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

using namespace qf::core;

namespace {

int environment_treshold()
{
	const QByteArray ba = qgetenv("QF_LOG_TRESHOLD");
	if(ba.isEmpty()) return -1;
	QString s = QString::fromLatin1(ba.data());
	bool ok;
	int ret =  s.toInt(&ok);
	if(!ok) return -1;
	return ret;
}

QList< LogDevice* >& logDevices();

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	Log::Level level = Log::LOG_DEB;
	switch(type) {
	case QtDebugMsg: level = Log::LOG_DEB; break;
	case QtWarningMsg:
		if(QLatin1String(context.category) == Log::categoryDebugName) level = Log::LOG_DEB;
		else if(QLatin1String(context.category) == Log::categoryInfoName) level = Log::LOG_INFO;
		else if(QLatin1String(context.category) == Log::categoryWarningName) level = Log::LOG_WARN;
		else if(QLatin1String(context.category) == Log::categoryErrorName) level = Log::LOG_ERR;
		else if(QLatin1String(context.category) == Log::categoryFatalName) level = Log::LOG_FATAL;
		else level = Log::LOG_WARN;
		break;
	case QtCriticalMsg: level = Log::LOG_ERR; break;
	case QtFatalMsg: level = Log::LOG_FATAL; break;
	}
	for(auto log_device : logDevices()) {
		if(log_device->checkLogPermisions(LogDevice::domainFromContext(context), level)) {
			log_device->log(level, context, msg);
		}
	}
}

QList< LogDevice* >& logDevices()
{
	static bool first_run = true;
	if(first_run) {
		qInstallMessageHandler(myMessageOutput);
	}
	static QList< LogDevice* > log_devices;
	return log_devices;
}

}

//=========================================================
// LogDevice
//=========================================================
int LogDevice::environmentLogTreshold = environment_treshold();
int LogDevice::commandLineLogTreshold = -1;

LogDevice::LogDevice(int argc, char *argv[])
	: m_logTreshold(Log::LOG_INFO), m_count(0), m_isPrettyDomain(false)
{
	setDomainTresholds(argc, argv);
}

LogDevice::~LogDevice()
{
	logDevices().removeOne(this);
}

QString LogDevice::domainFromContext(const QMessageLogContext &context)
{
	return QString::fromUtf8(context.file);
}

void LogDevice::install(LogDevice *dev)
{
	logDevices() << dev;
}

int LogDevice::setLogTreshold(Log::Level level)
{
	int old = m_logTreshold;
	m_logTreshold = level;
	return old;
}

void LogDevice::setDomainTresholds(int argc, char *argv[])
{
	m_domainTresholds.clear();
	QStringList tresholds;
	for(int i=1; i<argc; i++) {
		QString s = argv[i];
		if(s.startsWith("-d")) {
			//printf("1 %s\n", qPrintable(s));
			s = s.mid(2);
			//printf("2 %s\n", qPrintable(s));
			tresholds << s;
		}
	}
	for(QString dom_tres : tresholds) {
		int ix = dom_tres.indexOf(':');
		QString domain = dom_tres;
		//printf("domainTreshold %s\n", qPrintable(dom_tres));
		Log::Level level = Log::LOG_DEB;
		if(ix > 0) {
			domain = dom_tres.mid(0, ix);
			QString s = dom_tres.mid(ix + 1).toLower();
			if(s == "DEB") level = Log::LOG_DEB;
			else if(s == "INFO") level = Log::LOG_INFO;
			else if(s == "WARN") level = Log::LOG_WARN;
			else if(s == "ERR") level = Log::LOG_ERR;
		}
		if(domain.isEmpty()) {
			setLogTreshold(level);
		}
		else {
			//printf("add domainTreshold %s %d \n", qPrintable(domain), level);
			m_domainTresholds[domain] = level;
		}
	}
}

int LogDevice::logTreshold()
{
	if(commandLineLogTreshold >= 0) return commandLineLogTreshold;
	if(environmentLogTreshold >= 0) return environmentLogTreshold;
	return m_logTreshold;
}

bool LogDevice::checkLogPermisions(const QString &_domain, int _level)
{
	bool ret = false;
	do {
		if(_level < 0) break;
		if(_level == QtFatalMsg) {ret = true; break;}
		if(_level == QtDebugMsg) {
#ifndef QT_DEBUG
			break;
#endif
		}
		bool domain_level_found = false;
		QMapIterator<QString, int> it(m_domainTresholds);
		while (it.hasNext()) {
			it.next();
			if(_domain.indexOf(it.key(), 0, Qt::CaseInsensitive) >= 0) {
				//printf("found %s %d \n", qPrintable(_domain), it.value());
				domain_level_found = true;
				if(_level >= it.value()) ret = true;
				break;
			}
		}
		if(!domain_level_found) {
			if(_level >= logTreshold()) ret = true;
		}
	} while(false);
	//printf("%s %d \n", qPrintable(_domain), ret);
	return ret;
}

void LogDevice::setPrettyDomain(bool b)
{
	m_isPrettyDomain = b;
}

bool LogDevice::isPrettyDomain() const
{
	return m_isPrettyDomain;
}

QString LogDevice::prettyDomain(const QString &domain)
{
	if(!isPrettyDomain())
		return domain;
	static QRegExp rx("(\\\\|\\/)");
	return domain.section(rx, -1);
}

//=========================================================
// FileLogDevice
//=========================================================

FileLogDevice::FileLogDevice(int argc, char *argv[])
	: Super(argc, argv)
{
	m_file = stderr;
}

FileLogDevice::~FileLogDevice()
{
	if(m_file != stderr) {
		::fclose(m_file);
	}
}

FileLogDevice *FileLogDevice::install(int argc, char *argv[])
{
	FileLogDevice *ret = new FileLogDevice(argc, argv);
	Super::install(ret);
	return ret;
}

void FileLogDevice::setFile(const QString &path_to_file)
{
	FILE *f = ::fopen(qPrintable(path_to_file), "w");
	if(f) {
		m_file = f;
	}
	else {
		fprintf(stderr, "Cannot open log file '%s' for writing\n", qPrintable(path_to_file));
	}
}

void FileLogDevice::log(Log::Level level, const QMessageLogContext &context, const QString &msg)
{
	if(!m_file) return;
#ifdef Q_OS_UNIX
	bool is_tty = ::isatty(fileno(m_file));
	enum TerminalColor {Black = 0, Red, Green, Yellow, Blue, Magenta, Cyan, White};
	enum TerminalAttr {AttrReset = 0, AttrBright, AttrDim, AttrUnderline, AttrBlink, AttrReverse = 7, AttrHidden};
#endif

#ifdef Q_OS_UNIX
	if(is_tty) {
		TerminalColor fg, bg = Black;
		TerminalAttr attr = AttrReset;
		switch(level) {
		case Log::LOG_INFO: fg = Cyan; break;
		case Log::LOG_WARN: fg = Magenta; attr = AttrBright; break;
		case Log::LOG_ERR:
		case Log::LOG_FATAL: fg = Red; attr = AttrBright; break;
		case Log::LOG_DEB:
		default: fg = White; break;
		}
		QString s;
		std::fprintf(m_file, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
	}
#endif
	std::fprintf(m_file, "%4i", ++m_count);
#ifdef Q_OS_UNIX
	if(!is_tty) {
		/// add timestamp if log goes to file
		QString s = QDateTime::currentDateTime().toString(Qt::ISODate);
		QByteArray ba = s.toLatin1();
		std::fprintf(m_file, " %s", ba.constData());
	}
#endif

	std::fprintf(m_file, "<%s>", Log::levelName(level));
	QString domain = prettyDomain(domainFromContext(context));
	if(!domain.isEmpty()) {
		std::fprintf(m_file, "[%s] ", qPrintable(domain));
	}
	std::fprintf(m_file, "%s", qPrintable(msg));
#ifdef Q_OS_UNIX
	if(is_tty) {
		QString s;
		s.sprintf("%c[%d;%d;%dm", 0x1B, AttrReset, White + 30, Black + 40);
		QByteArray ba = s.toLatin1();
		std::fprintf(m_file, "%s", ba.constData());
	}
#endif
#ifdef Q_OS_WIN
	std::fprintf(m_file, "\r\n");
#else
	std::fprintf(m_file, "\n");
#endif
	std::fflush(m_file);

	if(level == Log::LOG_FATAL)
		std::terminate();
}
