#include "connection.h"

#include "catalog.h"

#include "../core/log.h"
#include "../core/utils.h"
#include "../core/assert.h"

#include <QMap>
#include <QVariant>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlField>
#include <QStringList>
#include <QMutableListIterator>
#include <QProcess>

using namespace qf::core::sql;

//=========================================
//              Connection
//=========================================
namespace {

QMap<QString, QStringList> s_primaryIndexCache;
QMap<QString, QString> s_serialFieldNamesCache;

void s_clearCache(const QString &connection_name)
{
	QString prefix = connection_name + '.';
	{
		QMutableMapIterator<QString, QStringList> it(s_primaryIndexCache);
		while(it.hasNext()) {
			it.next();
			if(it.key().startsWith(prefix, Qt::CaseInsensitive))
				it.remove();
		}
	}
	{
		QMutableMapIterator<QString, QString> it(s_serialFieldNamesCache);
		while(it.hasNext()) {
			it.next();
			if(it.key().startsWith(prefix, Qt::CaseInsensitive))
				it.remove();
		}
	}
}

}

Connection::Connection()
	: QSqlDatabase()
{
}

Connection::Connection(const QSqlDatabase& qdb)
	: QSqlDatabase(qdb)
{
}

bool Connection::open()
{
	s_clearCache(connectionName());
	return Super::open();
}

void Connection::close()
{
	Super::close();
}

#if 0
void DbInfo::close()
{
	if(isOpen()) {
		qfLogFuncFrame() << signature();
		QSqlDatabase::close();
	}
}

void DbInfo::open(const DbInfo::ConnectionOptions &options) throw(QFSqlException)
{
	qfLogFuncFrame() << signature() << "driver:" << driver();
	qfDebug() << "\tdriver name:" << driver()->metaObject()->className();
	close();
	if(port() == 0) setPort(defaultPort(driverName()));
	if(!options.isEmpty()) {
		QStringList opt_list;
		foreach(QString key, options.keys()) {
			QString val = options.value(key);
			if(val.isNull()) val = key;
			else val = key + "=" + val;
			opt_list << val;
		}
		setConnectOptions(opt_list.join(";"));
	}
	//qfInfo() << "password:" << password();
	if(!QSqlDatabase::open()) {
		setConnectOptions();
		throw QFSqlException(QString("Error opening database %1").arg(signature())
							 + "\n" + "\n"
							 + lastError().text());
	}
}
#endif
int Connection::defaultPort(const QString &driver_name)
{
	if(driver_name.endsWith("PSQL")) return 5432;
	else if(driver_name.endsWith("MYSQL")) return 3306;
	else if(driver_name.endsWith("IBASE")) return 3050;
	return 0;
}

QString Connection::signature() const
{
	QString s;
	s += this->databaseName();
	s+= QString("[%1]").arg(this->driverName());
	s+= this->userName() + "@";
	s+= this->hostName() + ":";
	s+= QString("%1").arg(this->port());
	//qfInfo() << this->driver()->handle().typeName() << this->driver()->handle().data();
	return s;
}

QString Connection::info(int verbosity) const
{
	QString s;
	if(verbosity == 0) s = signature();
	else {
		s = QString("database '%1' on %2@%3 driver %4");
		s = s.arg(databaseName()).arg(userName()).arg(hostName()).arg(driverName());
	}
	return s;
}

static QString formatValueForSql(const QVariant &val)
{
	QString ret = val.toString();
	switch(val.type()) {
	case QVariant::String:
		ret = "'" + ret + "'";
		break;
	default:
		break;
	}
	return ret;
}

static QVariant sqlite_set_pragma(QSqlQuery &q, const QString &pragma_key, const QVariant &val)
{
	qfLogFuncFrame() << pragma_key << "value:" << val.toString();
	QString qs = "PRAGMA " + pragma_key;
	if(!q.exec(qs)) {
		qfError() << QString("SQL Error\nquery: %1;").arg(qs);
		return QVariant();
	}
	q.next();
	QVariant old_val = q.value(0);
	qfDebug() << "\t oldval:" << old_val.toString();
	//qfDebug() << "\t newval:" << val.toString();
	qs = qs + "=" + formatValueForSql(val);
	//qfInfo() << qs;
	if(!q.exec(qs)) {
		qfError() << QString("SQL Error\nquery: %1;").arg(qs);
		return QVariant();
	}
	return old_val;
}

QStringList Connection::tables(const QString& dbname, QSql::TableType type) const
{
	qfLogFuncFrame() << "dbname:" << dbname << "type" << type;
	QStringList ret;
	if(driverName().endsWith("PSQL")) {
		QSqlQuery q(*this);
		QString s = "SELECT table_name FROM information_schema.tables"
					" WHERE table_schema='%1' AND table_type IN (%2)"
					" ORDER BY table_type, table_name";
		s = s.arg(dbname);
		QStringList table_types;
		if(type & QSql::Tables)
			table_types << "'BASE TABLE'";
		if(type & QSql::Views)
			table_types << "'VIEW'";
		s = s.arg(table_types.join(','));
		q.exec(s);
		while(q.next()) {
			ret << q.value(0).toString();
		}
	}
	else if(driverName().endsWith("IBASE")) {
		QSqlQuery q(*this);
		QString s = "SELECT RDB$RELATION_NAME FROM RDB$RELATIONS"
					" WHERE RDB$SYSTEM_FLAG = %2 AND RDB$VIEW_BLR IS %1 NULL";
		if(type == QSql::Tables)
			s = s.arg("").arg(0);
		else if(type == QSql::Views)
			s = s.arg("NOT").arg(0);
		else if(type == QSql::SystemTables)
			s = s.arg("NOT").arg(1);
		else s = QString();
		if(!s.isEmpty()) {
			//qfDebug() << "\t" << s;
			q.exec(s);
			while(q.next()) {
				ret << q.value(0).toString();
			}
		}
	}
	else if(driverName().endsWith("SQLITE")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		QStringList table_types;
		if(type & QSql::Tables)
			table_types << "'table'";
		if(type & QSql::Views)
			table_types << "'view'";
		if(type == QSql::SystemTables)
			table_types << "'system'";
		QString from;
		if(dbname.isEmpty() || dbname == "main")
			from = "(SELECT * FROM sqlite_master UNION ALL SELECT * FROM sqlite_temp_master)";
		else
			from = dbname + ".sqlite_master";
		QString s;
		QVariant old_short_column_names = sqlite_set_pragma(q, "short_column_names", 0);
		QVariant old_full_column_names = sqlite_set_pragma(q, "full_column_names", 0);
		s = "SELECT name FROM %1 WHERE type IN (%2) ORDER BY type, name";
		s = s.arg(from).arg(table_types.join(','));
		qfDebug() << "\t" << s;
		if(!q.exec(s)) {
			qfError() << QString("Error getting table list for database '%1'\nquery: %2;").arg(dbname).arg(s);
			return QStringList();
		}
		while(q.next()) {
			s = q.value(0).toString();
			ret.append(s);
		}
		sqlite_set_pragma(q, "short_column_names", old_short_column_names);
		sqlite_set_pragma(q, "full_column_names", old_full_column_names);
	}
	else if(driverName().endsWith("MYSQL")) {
		QSqlQuery q(*this);
		QStringList sl = serverVersion();
		int ver = 0;
		if(sl.size() > 0) {
			//qfDebug() << "\tsl[0]:" << sl[0];
			ver = sl[0].toInt();
		}
		//qfDebug() << "\tver:" << ver;
		if(ver <= 4 || driverName().contains("HTTP")) { /// napr. www.wz.cz zakazuje uzivateli information_schema
			if(type == QSql::Tables) {
				/// kvuli verzi 4, ktera nema information_schema
				QString s = "SHOW TABLES FROM %1";
				s = s.arg(dbname);
				q.exec(s);
			}
		}
		else {
			QString s = "SELECT table_name FROM INFORMATION_SCHEMA.TABLES"
						" WHERE table_schema='%1' AND table_type IN (%2)";
			QStringList table_types;
			if(type & QSql::Tables)
				table_types << "'BASE TABLE'";
			if(type & QSql::Views)
				table_types << "'VIEW'";
			if(type == QSql::SystemTables)
				table_types << "'SYSTEM VIEW'";
			s = s.arg(dbname).arg(table_types.join(','));
			qfDebug() << "\t" << s;
			q.exec(s);
		}
		qfDebug() << "\tfound:";
		while(q.next()) {
			QString s = q.value(0).toString();
			qfDebug() << s;
			ret << s;
		}
	}
	return ret;
}

QSqlRecord Connection::record(const QString & tablename) const
{
	qfLogFuncFrame() << "tblname:" << tablename;
	QString s = fullTableNameToQtDriverTableName(tablename);
	if(driverName().endsWith("SQLITE")) {
		/// SQLITE neumi schema.tablename v prikazu PRAGMA table_info(...)
		int ix = s.lastIndexOf('.');
		if(ix >= 0) s = s.mid(ix + 1);
	}
	return QSqlDatabase::record(s);
}

QStringList Connection::fields(const QString& tbl_name) const
{
	qfLogFuncFrame() << "tblname:" << tbl_name;
	QString tblname = normalizeDbName(tbl_name);
	//if(tblname[0] == '.') tblname = tblname.mid(1);
	QStringList ret;

	QSqlRecord r = record(tblname);
	if(r.isEmpty()) {
		qfWarning() << QString("table '%1' does not contains fields. (maybe it is in other schema?)").arg(tbl_name);
		//QF_SQL_EXCEPTION(QString("table '%1' does not contains fields. (maybe it is in other schema?)").arg(tbl_name));
	}
	else for(int i=0; i<r.count(); i++) {
		QString s = r.field(i).name().trimmed();
		s = s.section('.', -1);
		qfDebug().nospace() << "\tadding: '" << s << "'";
		ret.append(s);
	}
	return ret;
}

Connection::IndexList Connection::indexes(const QString& tbl_name) const
{
	qfLogFuncFrame() << "tblname:" << tbl_name;
	QString tblname = tbl_name;
	//if(tblname[0] == '.') tblname = tblname.mid(1);
	IndexList ret;
	QString s;
	if(driverName().endsWith("PSQL")) {
		QString s = "SELECT indexname FROM pg_indexes WHERE tablename=" QF_SARG(tbl_name);
		QSqlQuery q1(driver()->createResult());
		q1.setForwardOnly(true);
		q1.exec(s);
		while(q1.next()) {
			QString indexname = q1.value(0).toString();
			IndexInfo df;
			df.name = indexname;
			QString indexview = "SELECT indexes.relname AS indexname, indisprimary AS isprimary, indisunique AS isunique, columns.attname AS colname"
								" FROM pg_index LEFT JOIN pg_class AS indexes ON pg_index.indexrelid = indexes.oid"
								" LEFT JOIN pg_attribute AS columns ON columns.attrelid = pg_index.indrelid"
								" WHERE pg_index.indrelid='%1'::regclass AND columns.attnum = ANY (indkey)";
			indexview = indexview.arg(tblname);
			s  = "SELECT colname, isunique, isprimary FROM (%1) AS t WHERE indexname = '%2'";
			QSqlQuery q(*this);
			q.setForwardOnly(true);
			q.exec(s.arg(indexview).arg(indexname));
			int i = 0;
			while(q.next()) {
				df.fields.append(q.value(0).toString());
				if(i++ == 0) df.unique = q.value(1).toBool();
			}
			ret.append(df);
		}
	}
	else if(driverName().endsWith("MYSQL")) {
		QString s = "SHOW INDEX FROM " + tblname;
		qfDebug() << "\t" << s;
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		q.exec(s);
		IndexInfo df;
		while(q.next()) {
			s = q.value(2).toString();
			qfDebug() << "\tkey_name:" << s;
			if(df.name != s) {
				if(!df.name.isEmpty()) ret.append(df);
				df = IndexInfo();
				df.name = s;
				df.unique = !q.value(1).toBool();
				df.primary = (df.name == "PRIMARY");
				df.fields.append(q.value(4).toString());
			}
			else {
				df.fields.append(q.value(4).toString()); //q.record().indexOf("Column_name")
			}
		}
		if(!df.name.isEmpty()) ret.append(df);
	}
	else if(driverName().endsWith("SQLITE")) {
		/// SQLITE neumi schema.tablename v prikazu PRAGMA table_info(...)
		QString tbl_name = tblname;
		{
			int ix = tbl_name.lastIndexOf('.');
			if(ix >= 0) tbl_name = tbl_name.mid(ix + 1);
		}
		QString s = "SELECT * FROM sqlite_master WHERE type='index' AND sql>'' AND tbl_name='%1'";
		s = s.arg(tbl_name);
		qfDebug() << "\t" << s;
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		q.exec(s);
		while(q.next()) {
			s = q.value(1).toString(); /// name
			qfDebug() << "\tkey_name:" << s;
			IndexInfo df;
			df.name = s;
			s = q.value(4).toString(); /// sql
			df.unique = s.indexOf(" UNIQUE ", Qt::CaseInsensitive) > 0;
			{
				int ix = s.lastIndexOf('(');
				if(ix > 0) {
					s = s.mid(ix+1);
					s = s.mid(0, s.count()-1);
					QStringList sl = s.split(',');
					df.fields = sl;
				}
			}
			if(!df.name.isEmpty()) ret.append(df);
		}
	}
	return ret;
}

QStringList Connection::databases() const
{
	qfLogFuncFrame();
	QStringList sl;
	QSqlQuery q(*this);
	q.setForwardOnly(true);

	if(driverName().endsWith("PSQL")) {
		q.exec(QLatin1String("SELECT datname FROM pg_database "));
		while(q.next()) {
			QString s = q.value(0).toString();
			qfDebug() << "\tfound:" << s;
			if(s.startsWith("template")) continue;
			sl.append(s);
		}
	}
	else {
		sl.append(databaseName());
	}
	qfDebug() << "\tloaded from server:" << sl.join(", ");
	return sl;
}

QStringList Connection::schemas() const
{
	qfLogFuncFrame();
	QStringList ret;
	if(driverName().endsWith("PSQL")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		q.exec("SELECT n.nspname "
			   " FROM pg_catalog.pg_namespace  AS n"
			   " WHERE   (n.nspname NOT LIKE 'pg\\_temp\\_%' OR"
			   " n.nspname = (pg_catalog.current_schemas(true))[1])"
			   " ORDER BY 1");
		QSqlRecord r = q.record();
		while(q.next()) {
			QString s = q.value(0).toString();
			qfLogFuncFrame() << "loading schema" << s;
			ret.append(s);
		}
	}
	else if(driverName().endsWith("SQLITE")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		q.exec(QLatin1String("PRAGMA database_list"));
		QSqlRecord r = q.record();
		while(q.next()) {
			QString s = q.value(r.indexOf("name")).toString();
			ret.append(s);
		}
	}
	else if(driverName().endsWith("MYSQL")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		q.exec(QLatin1String("SHOW DATABASES;"));
		//QSqlRecord r = q.record();
		while(q.next()) {
			QString s = q.value(0).toString();
			ret.append(s);
		}
	}
	else if(driverName().endsWith("IBASE")) {
		ret << "main";
	}
	qfDebug() << "\tloaded from server:" << ret.join(", ");
	return ret;
}

bool Connection::isOpen() const
{
	if(!isValid()) return false;
	if(!QSqlDatabase::isOpen()) return false;
	//QSqlDriver *drv = driver();
	//if(!drv) return false;
	//qfLogFuncFrame() << "driver:" << drv;
	//if(!drv->handle().isValid()) return false;
	//if(!QSqlDatabase::isOpen()) return false;
	//if(!drv->isOpen()) return false;
	//qfLogFuncFrame();
	//qfDebug() << "\treturning:" << ret;
	return true;
}
#if 0
QFSql::RelationKind DbInfo::relationKind(const QString& _relname)
{
	QFSql::RelationKind ret = QFSql::UnknownRelation;
	QString relname, dbname;
	qf::core::Utils::parseFieldName(_relname, &relname, &dbname);
	if(driverName().endsWith("PSQL")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		QString s = "SELECT c.relname, c.relkind, n.nspname"
					" FROM pg_class AS c"
					" LEFT JOIN pg_namespace AS n ON c.relnamespace=n.oid"
					" WHERE c.relname = '%1'";
		if(!dbname.isEmpty()) s += " AND n.nspname = '" + dbname + "'";
		if(q.exec(s.arg(relname))) {
			while(q.next()) {
				s = q.value(1).toString();
				if(s == "r") ret = QFSql::TableRelation;
				else if(s == "v") ret = QFSql::ViewRelation;
				else if(s == "i") ret = QFSql::IndexRelation;
				else if(s == "S") ret = QFSql::SequenceRelation;
				break;
			}
		}
	}
	else if(driverName().endsWith("SQLITE")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		QString from;
		if(dbname.isEmpty() || dbname == "main") from = "(SELECT * FROM sqlite_master UNION ALL	SELECT * FROM sqlite_temp_master)";
		else from = dbname + ".sqlite_master";
		QString s = "SELECT type, name, tbl_name FROM %1 WHERE name = '%2'";
		s = s.arg(from).arg(relname);
		QVariant old_short_column_names = sqlite_set_pragma(q, "short_column_names", 0);
		QVariant old_full_column_names = sqlite_set_pragma(q, "full_column_names", 0);
		if(!q.exec(s)) {
			QF_SQL_EXCEPTION(QString("Error getting table list for database '%1'").arg(dbname));
		}
		// For tables, the type field will always be 'table' and the name field will be the name of the table.
		// For indices, type is equal to 'index', name is the name of the index
		//       and tbl_name is the name of the table to which the index belongs.
		else {
			while(q.next()) {
				s = q.value(0).toString();
				if(s == "table") ret = QFSql::TableRelation;
				else if(s == "view") ret = QFSql::ViewRelation;
				else if(s == "index") ret = QFSql::IndexRelation;
				break;
			}
		}
		sqlite_set_pragma(q, "short_column_names", old_short_column_names);
		sqlite_set_pragma(q, "full_column_names", old_full_column_names);
	}
	else if(driverName().endsWith("MYSQL")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		/*
		QString s = "SELECT table_type FROM INFORMATION_SCHEMA.TABLES"
				" WHERE table_schema = '%1' AND table_name = '%2'";
		*/
		/// kvuli verzi 4, ktera nema information_schema
		QString s = "SHOW FULL tables FROM %1 LIKE '%2'";
		s = s.arg(dbname).arg(relname);
		if(!q.exec(s)) {
			QF_SQL_EXCEPTION(QString("Error getting table list for database '%1'\n\n%2").arg(dbname).arg(s));
		}
		else {
			while(q.next()) {
				s = q.value(1).toString();
				if(s == "BASE TABLE") ret = QFSql::TableRelation;
				else if(s == "VIEW") ret = QFSql::ViewRelation;
				else if(s == "SYSTEM VIEW") ret = QFSql::SystemTableRelation;
				break;
			}
		}
		//if(!q.exec("PRAGMA full_column_names=1")) QF_SQL_EXCEPTION(QString("SQL Error\nquery: %1;").arg(s));
	}
	return ret;
}

QStringList DbInfo::fieldDefsFromCreateTableCommand(const QString &cmd)
{
	qfLogFuncFrame() << cmd;
	QString fs = cmd;
	QStringList sl;
	do {
		int ix;
		if((ix = fs.indexOf('(')) <0)
			break;
		fs = fs.slice(ix);
		if((ix= fs.indexOfMatchingBracket('(', ')', '\'')) <0)
			break;
		//qfDebug() << "\tlen:" << fs.len() << "ix:" << ix;
		fs = fs.slice(1, ix);
		sl = fs.splitBracketed(',', '(', ')', '\'');
	} while(false);
	return sl;
}
#endif

static QString err_msg(QProcess::ProcessError err_no)
{
	switch(err_no) {
	case QProcess::FailedToStart:
		return "The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.";
	case QProcess::Crashed:
		return "The process crashed some time after starting successfully.";
	case QProcess::Timedout:
		return "The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.";
	case QProcess::WriteError:
		return "An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.";
	case QProcess::ReadError:
		return "An error occurred when attempting to read from the process. For example, the process may not be running.";
	default:
		return "An unknown error occurred. This is the default return value of error().";
	}
}

QString Connection::createTableSqlCommand(const QString &tblname)
{
	qfLogFuncFrame();
	if(driverName().endsWith("SQLITE")) {
		QString db, tbl;
		qf::core::Utils::parseFieldName(tblname, &tbl, &db);
		//db = normalizeDbName(db);
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		QString s = "SELECT sql FROM %1.sqlite_master"
					" WHERE type IN ('table', 'view')"
					" AND tbl_name = '%2'";
		s = s.arg(db).arg(tbl);
		//qfDebug() << '\t' << s;
		if(!q.exec(s)) {
			QSqlError err = lastError();
			qfError() << QString("Error getting sql for '%1'").arg(tblname) + "\n" + err.driverText() + "\n" + err.databaseText();
			//QF_SQL_EXCEPTION(s);
		}
		else 	while(q.next()) {
			return q.value(0).toString() + ";";
		}
	}
	else if(driverName().endsWith("PSQL")) {
		QString db, tbl;
		qf::core::Utils::parseFieldName(tblname, &tbl, &db);
		QProcess proc;
		QString prog_name = "pg_dump";
		QStringList params;
		params << "-h";
		params << hostName();
		params << "-p";
		params << QString::number(port());
		params << "-U";
		params << userName();
		params << "-t";
		params << tbl;
		params << databaseName();
		QString s = prog_name + " " + params.join(" ");
		proc.start(prog_name, params);
		qfDebug() << "\tcalling:" << s;
		if (!proc.waitForStarted(5000)) {
			return "calling: " + s + "\n" + err_msg(proc.error());
		}
		//proc.write("Qt rocks!");
		//proc.closeWriteChannel();
		if (!proc.waitForFinished(10000)) {
			return "finished error: " + err_msg(proc.error());
		}
		s = QString();
		if(proc.exitCode() != 0) {
			s = "EXIT CODE: %1\n";
			s = s.arg(proc.exitCode());
			s += QString(proc.readAllStandardError());
			s += "\n";
		}
		QByteArray result = proc.readAll();
		return s + QString(result);
	}
	if(driverName().endsWith("MYSQL")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		QString s = "SHOW CREATE TABLE %1";
		s = s.arg(tblname);
		qfDebug() << '\t' << s;
		if(!q.exec(s)) {
			QSqlError err = lastError();
			//qfDebug() << "\terr is valid:" << err.isValid();
			QString msg  = QString("Error getting sql for '%1'").arg(tblname) + "\n" + s + "\n" + err.driverText() + "\n" + err.databaseText();
			qfError() << msg;
			return QString();
		}
		else if(q.next()) {
			return q.value(1).toString() + ";";
		}
		return QString();
	}
	//return "unsupported for " + driverName();
	return QString();
}

QString Connection::dumpTableSqlCommand(const QString &tblname)
{
	if(driverName().endsWith("MYSQL")) {
		QSqlQuery q(*this);
		q.setForwardOnly(true);
		QString s = "SELECT * FROM %1";
		s = s.arg(tblname);
		//qfDebug() << '\t' << s;
		if(!q.exec(s)) {
			QSqlError err = lastError();
			//qfDebug() << "\terr is valid:" << err.isValid();
			QString s  = QString("Error select from '%1'").arg(tblname) + "\n" + err.driverText() + "\n" + err.databaseText();
			qfError() << s;
		}
		else {
			QStringList sl;
			while(q.next()) {
				QSqlRecord rec = q.record();
				QStringList sl2;
				for(int i=0; i<rec.count(); i++) {
					sl2 << driver()->formatValue(rec.field(i));
				}
				sl << "(" + sl2.join(",") + ")";
			}
			QString table;
			qf::core::Utils::parseFieldName(tblname, &table);
			s = "INSERT INTO %1 VALUES\n";
			s = s.arg(table) + sl.join(",\n");
			s += ";";
			return s;
		}
		return QString();
	}
	return "unsupported for " + driverName();
}

QSqlIndex Connection::primaryIndex(const QString& table_id)
{
	qfLogFuncFrame() << "table name:" << table_id;
	QString tblname = table_id;
	QSqlIndex ret;
	if(driverName().endsWith("SQLITE")) {
#ifndef SQLITE_PARSE_CREATE_SQL_SCRIPT
		// TODO: use PRAGMA table_info(table_name)  instead of parsing CREATE command
		// s timhle je problem, protoze pragma neprijma table_name ve tvaru schema.table
		QString tblname, dbname;
		tblname = normalizeTableName(table_id);
		qf::core::Utils::parseFieldName(tblname, &tblname, &dbname);
		QString s = "PRAGMA table_info(table_name)";
		s = s.arg(tblname, dbname);
		QSqlQuery q = exec(s);
		while(q.next()) {
			ret.append(QSqlField(q.value(0).toString()));
		}
#else
		/// parsing create table command
		QString s = createTableSqlCommand(tblname);
		QStringList sl =fieldDefsFromCreateTableCommand(s);
		foreach(QString s1, sl) {
			qfDebug() << "\tfield:" << s1;//PRIMARY KEY
			if(s1.indexOf("primary key", 0, Qt::CaseInsensitive) > 0) {
				QString fs = s1.section(' ', 0, 0, QString::SectionSkipEmpty);
				if(!!fs) {
					QVariant::Type type;
					QString type_name = s1.section(' ', 1, 1, QString::SectionSkipEmpty).toLower();
					if (type_name == QLatin1String("integer") || type_name == QLatin1String("int")) type = QVariant::Int;
					else if(type_name == QLatin1String("double") || type_name == QLatin1String("float") || type_name.startsWith(QLatin1String("numeric"))) type = QVariant::Double;
					else if (type_name == QLatin1String("blob")) type = QVariant::ByteArray;
					else type = QVariant::String;

					ret.append(QSqlField(fs.trimmed(), type));
				}
			}
		}
#endif
	}
	else {
		tblname = fullTableNameToQtDriverTableName(tblname);
		ret = QSqlDatabase::primaryIndex(tblname);
	}
	//for(int i=0; i<ret.count(); i++) {
	//	qfDebug() << "\t" << ret.field(i).name();
	//}
	return ret;
}

QStringList Connection::primaryIndexFieldNames(const QString &table_id)
{
	QF_ASSERT(isValid(),
			  QString("Connection '%1' is not valid!").arg(connectionName()),
			  return QStringList());
	QString pk_key = connectionName() + '.' + table_id;
	QStringList ret;
	if(!s_primaryIndexCache.contains(pk_key)) {
		QSqlIndex sql_ix = primaryIndex(table_id);
		for(int i=0; i<sql_ix.count(); i++) {
			QString fld_name = sql_ix.fieldName(i);
			qf::core::Utils::parseFieldName(fld_name, &fld_name);
			ret << fld_name;
		}
		s_primaryIndexCache[pk_key] = ret;
	}
	else {
		ret = s_primaryIndexCache.value(pk_key);
	}
	return ret;
}

QString Connection::serialFieldName(const QString &table_id)
{
	qfLogFuncFrame() << "table_id:" << table_id;
	QF_ASSERT(isValid(),
			  QString("Connection '%1' is not valid!").arg(connectionName()),
			  return QString());
	QString pk_key = connectionName() + '.' + table_id;
	QString ret;
	if(!s_serialFieldNamesCache.contains(pk_key)) {
		qf::core::sql::FieldInfoList fldlst;
		fldlst.load(*this, table_id);
		QMapIterator<QString, qf::core::sql::FieldInfo> it(fldlst);
		while(it.hasNext()) {
			it.next();
			qf::core::sql::FieldInfo fi = it.value();
			qfDebug() << "checking:" << it.key() << "name:" << fi.name();
			if(fi.isAutoIncrement()) {
				qfDebug() << "\t found auto increment field:" << fi.name();
				ret = it.key();
				break;
			}
		}
		s_serialFieldNamesCache[pk_key] = ret;
	}
	else {
		ret = s_serialFieldNamesCache.value(pk_key);
	}
	return ret;
}

QString Connection::normalizeFieldName(const QString &n) const
{
	QString db, tbl, fld;
	qf::core::Utils::parseFieldName(n, &fld, &tbl, &db);
	if(!tbl.isEmpty()) {
		db = normalizeDbName(db);
		return qf::core::Utils::composeFieldName(fld, tbl, db);
	}
	return fld;
}

QString Connection::normalizeTableName(const QString &n) const
{
	QString db, tbl;
	qf::core::Utils::parseFieldName(n, &tbl, &db);
	db = normalizeDbName(db);
	return qf::core::Utils::composeFieldName(tbl, db);
}

QString Connection::normalizeDbName(const QString &n) const
{
	/*
	QString db = n;
	if(!db)
		db = currentSchema();
	return qf::core::Utils::composeFieldName(db);
	*/
	return n;
}

/*
QString DbInfo::currentSchema() const
{
	QString ret;
	if(driverName().endsWith("MYSQL")) {
		QSqlQuery q = exec("SELECT DATABASE()");
		if(lastError().isValid()) {
			QF_SQL_EXCEPTION("Error getting curent schema");
		}
		else {
			q.next();
			ret = q.value(0).toString();
		}
	}
	else if(driverName().endsWith("SQLITE")) {
		ret = "main";
	}
	else {
		ret = "main";
	}
	return ret;
}
*/
void Connection::setCurrentSchema(const QString &schema_name)
{
	if(driverName().endsWith("MYSQL")) {
		QSqlQuery q(*this);
		if(!q.exec("USE " + schema_name)) {
			qfError() << "Error setting curent schema to" << schema_name << "\n" << lastError().text();
		}
	}
	else if(driverName().endsWith("PSQL")) {
		QSqlQuery q(*this);
		if(!q.exec("SET SCHEMA " QF_SARG(schema_name))) {
			qfError() << "Error setting curent schema to" << schema_name << "\n" << lastError().text();
		}
	}
}

QStringList Connection::serverVersion() const
{
	QSqlQuery q = exec("SHOW variables LIKE 'version'");
	QStringList sl;
	if(q.next()) {
		sl = q.value(1).toString().split('.');
	}
	return sl;
}

QString Connection::fullTableNameToQtDriverTableName(const QString &full_table_name) const
{
	QString ret = full_table_name;
	if(driverName().endsWith("IBASE")) {
		/// IBASE nema zadny schema
		int ix = ret.lastIndexOf('.');
		if(ix >= 0)
			ret = ret.mid(ix + 1);
	}
	return ret;
}


