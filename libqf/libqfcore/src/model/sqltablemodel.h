#ifndef QF_CORE_MODEL_SQLTABLEMODEL_H
#define QF_CORE_MODEL_SQLTABLEMODEL_H

#include "tablemodel.h"
#include "../core/utils.h"
#include "../sql/querybuilder.h"
#include "../sql/query.h"

#include <QMap>
#include <QString>

namespace qf {
namespace core {
namespace sql {
class Connection;
}
namespace model {

class QFCORE_DECL_EXPORT SqlTableModel : public TableModel
{
	Q_OBJECT
	Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
	//Q_PROPERTY(QVariant queryParameters READ queryParameters WRITE setQueryParameters NOTIFY queryParametersChanged)
	Q_PROPERTY(QString connectionName READ connectionName WRITE setConnectionName)
private:
	typedef TableModel Super;
public:
	SqlTableModel(QObject *parent = 0);
	~SqlTableModel() Q_DECL_OVERRIDE;

	QF_PROPERTY_IMPL(QVariant, q, Q, ueryParameters)
public:
	bool reload(const QString &query_str);
	bool reload() Q_DECL_OVERRIDE;
	bool postRow(int row_no, bool throw_exc) Q_DECL_OVERRIDE;
	void revertRow(int row_no) Q_DECL_OVERRIDE;
	int reloadRow(int row_no) Q_DECL_OVERRIDE;
public:
	void setQueryBuilder(const qf::core::sql::QueryBuilder &qb);
	const qf::core::sql::QueryBuilder& queryBuilder() const;

	QString connectionName() const { return m_connectionName; }
	void setConnectionName(QString arg) { m_connectionName = arg; }

	QString query() const { return m_query; }
	void setQuery(QString arg) { if (m_query != arg) { m_query = arg; emit queryChanged(arg); } }
	Q_SIGNAL void queryChanged(QString arg);

	const qf::core::sql::Query& recentlyExecutedQuery() {return m_recentlyExecutedQuery;}

	void addForeignKeyDependency(const QString &master_table_key, const QString &slave_table_key);

protected:
	virtual QString buildQuery();
	virtual QString replaceQueryParameters(const QString query_str);
	qf::core::sql::Connection sqlConnection();
	bool reloadTable(const QString &query_str);
	QSet<QString> tableIds(const utils::Table::FieldList &table_fields);
	void setSqlFlags(qf::core::utils::Table::FieldList &table_fields, const QString &query_str);

	QSet<QString> referencedForeignTables();
	QStringList tableIdsSortedAccordingToForeignKeys();

	bool removeTableRow(int row_no, bool throw_exc = false) Q_DECL_OVERRIDE;
protected:
	qf::core::sql::QueryBuilder m_queryBuilder;
	QString m_query;
	QString m_connectionName;
	qf::core::sql::Query m_recentlyExecutedQuery;
	/// INSERT needs to know dependency of tables in joined queries to insert particular tables in proper order
	QMap<QString, QString> m_foreignKeyDependencies;
};

}}}

#endif // QF_CORE_MODEL_SQLTABLEMODEL_H
