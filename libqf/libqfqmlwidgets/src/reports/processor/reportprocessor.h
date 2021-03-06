
//
// Author: Frantisek Vacek <fanda.vacek@volny.cz>, (C) 2014
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef QF_QMLWIDGETS_REPORTS_REPORTPROCESSOR_H
#define QF_QMLWIDGETS_REPORTS_REPORTPROCESSOR_H

#include "../../qmlwidgetsglobal.h"
#include "reportitem.h"
#include "reportdocument.h"
//#include "reportprocessorcontext.h"

#include <qf/core/utils/treetable.h>
//#include <qf/core/utils/searchdirs.h>
#include <qf/core/assert.h>

#include <QObject>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QPointer>

class QPrinter;
class QQmlEngine;

namespace qf {
namespace qmlwidgets {
namespace reports {

class ReportItem;
class ReportItemBand;
class ReportItemMetaPaint;
class ReportItemMetaPaintReport;

//! TODO: write class documentation.
class  QFQMLWIDGETS_DECL_EXPORT ReportProcessor : public QObject
{
	Q_OBJECT
public:
	enum ProcessorMode {SinglePage = 1, FirstPage, AllPages};
	typedef QMap<QString, ReportItem::Image> ImageMap;
public:
	ReportProcessor(QPaintDevice *paint_device, QObject *parent = NULL);
	~ReportProcessor() Q_DECL_OVERRIDE;
public:
	/// vymaze vsechna data vznikla predchozimi kompilacemi
	void reset();
	//--void setReport(const ReportDocument &doc);
	void setReport(const QString &rep_file_name);
	QUrl reportUrl() const;
	//--ReportDocument report() {return fReport;}
	//--ReportDocument& reportRef() {return fReport;}
	void setData(const QString &key, const QVariant& data);
	QVariant data(const QString &key) const {return m_data.value(key);}

	void addImage(const QString key, const ReportItem::Image &img) {m_imageMap[key] = img;}
	const ImageMap& images() const {return m_imageMap;}

	bool isDesignMode() const {return m_designMode;}
	void setDesignMode(bool b) {m_designMode = b;}
public:
	void setPaintDevice(QPaintDevice *pd) {m_paintDevice = pd;}
	QPaintDevice* paintDevice() {
		QF_ASSERT_EX(m_paintDevice, "paintDevice cannot be null");
		return m_paintDevice;
	}
	//! Vrati QFontMetricsF pro \a font a \a paintDevice() .
	//! Pokud je paintDevice NULL, vrati fontMetrics pro screen.
	QFontMetricsF fontMetrics(const QFont &font);
	/*--
	static bool isProcessible(const QDomElement &el);
	//! vytvori item pro element a nastavi nektere deefaultni hodnoty atributu, postara se taky o atribut copyAttributesFrom.
	//! Pro vytvoreni kontkretniho itemu pak vola funkci createItem()
	ReportItem* createProcessibleItem(const QDomElement &el, ReportItem *parent);
	virtual ReportItem* createItem(ReportItem *parent, const QDomElement &el);
	--*/
	//! cislo stranky, ktera se zrovna zpracovava, pocitaji se od 0.
	int processedPageNo() const {return m_processedPageNo;}
	/*--
		static void appendSearchDir(const QString &path) {f_searchDirs.appendDir(path);}
		static void setSearchDirs(const QStringList &sl) {f_searchDirs.setDirs(sl);}
		/// oddelovac je kvuli windows "::"
		static void setSearchDirs(const QString &s) {f_searchDirs.setDirs(s);}
	--*/
	//--qf::core::utils::SearchDirs* searchDirs(bool throw_exc = true);
	/// does not take ownership os \a sd
	//--void setSearchDirs(qf::core::utils::SearchDirs *sd) {f_searchDirs = sd;}
protected:
	virtual ReportItem::PrintResult processPage(ReportItemMetaPaint *out);
	/// return NULL if such a page does not exist.
	ReportItemMetaPaintFrame *getPage(int page_no);
public:
	virtual void process(ProcessorMode mode = AllPages);
	void print(QPrinter &printer, const QVariantMap &options);

	int pageCount();

	ReportItemMetaPaintReport* processorOutput() {return m_processorOutput;}
	ReportItemReport* documentInstanceRoot();
public:
	/// vlozi do el_body report ve formatu HTML
	virtual void processHtml(QDomElement &el_body);

	void dump();
protected:
	void fixTableTags(QDomElement &el);
	QDomElement removeRedundantDivs(QDomElement &el);
	QDomElement fixLayoutHtml(QDomElement &el);
signals:
	//! emitovan vzdy, kdyz procesor dokonci dalsi stranku.
	void pageProcessed();
public slots:
	//! prelozi dalsi stranku reportu (takhle delam multithreading, protoze QFont musi bezet v GUI threadu)
	void processSinglePage() {process(SinglePage);}
public:
	/// Every QnlEngine created by ReportProcessor will have this import paths
	static QStringList& qmlEngineImportPaths();
protected:
	QQmlEngine* qmlEngine(bool throw_exc = true);
private:	
	ImageMap m_imageMap;

	QQmlEngine *m_qmlEngine = nullptr;
	ReportDocument *m_reportDocumentComponent = nullptr;
	QVariantMap m_data;
	QPaintDevice *m_paintDevice = nullptr;
	//! Tree of instantiated QML report objects.
	ReportItemReport *m_documentInstanceRoot = nullptr;
	//ReportItem *f_processedItemsRoot;
	//! Root of ReportItemMetaPaint objects tree generated by \a printMetaPaint() method.
	ReportItemMetaPaintReport *m_processorOutput = nullptr;

	int m_processedPageNo;
	//--qf::core::utils::SearchDirs *f_searchDirs;
	ReportItem::PrintResult m_singlePageProcessResult;

	//! designated for QML Reports GUI designer functionality.
	bool m_designMode;
};

}}}

#endif // QF_QMLWIDGETS_REPORTS_REPORTPROCESSOR_H

