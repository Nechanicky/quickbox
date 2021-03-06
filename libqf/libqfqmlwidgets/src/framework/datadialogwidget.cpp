#include "datadialogwidget.h"
#include "../dialogs/dialog.h"

#include <qf/core/model/datadocument.h>

namespace qfm = qf::core::model;
using namespace qf::qmlwidgets::framework;

DataDialogWidget::DataDialogWidget(QWidget *parent)
	: Super(parent)
{

}

DataDialogWidget::~DataDialogWidget()
{

}

qf::qmlwidgets::DataController *DataDialogWidget::dataController()
{
	return m_dataController;
}

void DataDialogWidget::setDataController(qf::qmlwidgets::DataController *dc)
{
	m_dataController = dc;
}

void DataDialogWidget::load(const QVariant &id, int mode)
{
	if(m_dataController) {
		if(m_dataController->document()) {
			m_dataController->document()->load(id, (qf::core::model::DataDocument::RecordEditMode)mode);
		}
	}
}

bool DataDialogWidget::dialogDoneRequest(int result)
{
	qfLogFuncFrame();
	bool ret = true;
	if(result == qf::qmlwidgets::dialogs::Dialog::ResultAccept) {
		ret = saveData();
	}
	if(ret)
		ret = Super::dialogDoneRequest(result);
	return ret;
}

bool DataDialogWidget::saveData()
{
	bool ret = true;
	DataController *dc = dataController();
	if(dc) {
		qfm::DataDocument *doc = dc->document();
		if(doc) {
			connect(doc, &qfm::DataDocument::saved, this, &DataDialogWidget::dataSaved, (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
			doc->save();
		}
	}
	return ret;
}

