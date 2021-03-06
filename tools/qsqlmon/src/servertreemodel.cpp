/********************************************************************
	created:	2005/01/02 14:27
	filename: 	ServerTreeModel.cpp
	author:		Fanda Vacek (fanda.vacek@volny.cz)
*********************************************************************/
#include "servertreemodel.h"
#include "servertreeitem.h"

#include <qf/core/log.h>

#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QModelIndex>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>

//=============================================
//                ServerTreeModel
//=============================================

ServerTreeModel::ServerTreeModel(QObject *parent)
	: Super(parent)
{
}

ServerTreeModel::~ServerTreeModel()
{
	saveSettings();
}

//---------------------------------------------
int ServerTreeModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 4;
}

//---------------------------------------------
QVariant ServerTreeModel::data(const QModelIndex & index, int role) const 
{
	QVariant ret;

	//qDebug("data() - model: %p data: '%p' row: %i col: %i role: %i", index.model(), index.data(), index.row(), index.column(), role);
	if(!index.isValid()) return ret;
	QObject *o = (QObject*)index.internalPointer();
	if(!o) return ret;
	ServerTreeItem *it = qobject_cast<ServerTreeItem*>(o);
	if(!it) {
		qDebug("data is NULL");
		return ret;
	}

	if(role == Qt::DecorationRole) {
		if(index.column() == 0) {
			ret = it->icon(index.column());
		}
	}
	else if(role == Qt::DisplayRole) {
		ret = it->text(index.column());
		//qDebug("data()  RETURN: '%s'", ret.toString().toLatin1().data());
	}
	return ret;
}

//---------------------------------------------
QVariant ServerTreeModel::headerData(int section, Qt::Orientation o, int role) const
{
	QVariant ret;
	if (o == Qt::Horizontal) {
        if(role == Qt::DisplayRole) switch(section) {
			case 0: ret = tr("Description"); break;
			case 1: ret = tr("Host"); break;
			case 2: ret = tr("Database"); break;
			case 3: ret = tr("Driver"); break;
        }
    }
	if(ret.isNull()) ret = QAbstractItemModel::headerData(section, o, role);
	return ret;
}

void ServerTreeModel::loadSettings()
{
	qfLogFuncFrame();
	init();
	QSettings settings;
	QJsonParseError err;
	QByteArray ba = settings.value("connections").toString().toUtf8();
	QVariantList connections_lst;
	if(!ba.isEmpty()) {
		QJsonDocument jsd = QJsonDocument::fromJson(ba, &err);
		if(err.error != QJsonParseError::NoError) {
			qfError() << "Cannot load connections definition from settings:" << err.errorString();
		}
		else {
			connections_lst = jsd.toVariant().toList();
		}
	}
	QSet<int> ids;
	for(auto val : connections_lst) {
		QVariantMap m = val.toMap();
		new Connection(m, m_rootObj);
	}
}

void ServerTreeModel::saveSettings()
{
	qfLogFuncFrame();
	QVariantList connections_lst;
	for(auto c : m_rootObj->findChildren<Connection*>(QString())) {
		qfDebug() << c;
		QVariantMap m = c->params();
		/*
		QMapIterator<QString, QVariant> it(m);
		while(it.hasNext()) {
			it.next();
			qfDebug() << "\t" << it.key() << "->" << it.value().toString();
		}
		*/
		connections_lst << m;
	}
	QSettings settings;
	QJsonDocument jsd = QJsonDocument::fromVariant(connections_lst);
	QString s = QString::fromUtf8(jsd.toJson(QJsonDocument::Compact));
	settings.setValue("connections", s);
}

//=============================================

