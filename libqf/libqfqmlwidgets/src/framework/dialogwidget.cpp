#include "dialogwidget.h"

#include "../action.h"
#include "../dialogs/dialog.h"

#include <qf/core/exception.h>

using namespace qf::qmlwidgets::framework;

DialogWidget::DialogWidget(QWidget *parent) :
	Super(parent), IPersistentSettings(this)
{
}

DialogWidget::~DialogWidget()
{
}

bool DialogWidget::dialogDoneRequest(int result)
{
	qfLogFuncFrame();
	Q_UNUSED(result);
	return true;
}

QVariant DialogWidget::dialogDoneRequest_qml(const QVariant &result)
{
	return dialogDoneRequest(result.toBool());
}

void DialogWidget::settleDownInDialogNative(qf::qmlwidgets::dialogs::Dialog *dlg)
{
	settleDownInDialog(dlg);
}

void DialogWidget::settleDownInDialog(qf::qmlwidgets::dialogs::Dialog *dlg)
{
	Q_UNUSED(dlg);
}

qf::qmlwidgets::Action* DialogWidget::action(const QString &name, bool throw_exc)
{
	qfLogFuncFrame() << "name:" << name;
	qf::qmlwidgets::Action *ret = actions().value(name);
	if(!ret && throw_exc)
		QF_EXCEPTION(tr("Cannot find action name '%1'").arg(name));
	return ret;
}

DialogWidget::ActionMap DialogWidget::actions()
{
	qfLogFuncFrame();
	return ActionMap();
}
