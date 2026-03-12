#pragma once
#include <QLabel>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>

class QPdfDocument;
class QPdfView;

class PdfPreviewWidget : public QWidget
{
	Q_OBJECT
public:
	explicit PdfPreviewWidget(const QString& filePath, QWidget* parent = nullptr);
	~PdfPreviewWidget();

private slots:
	void showLoadingPage();
	void showErrorPage();
	void showPreviewPage(int pageCount);
	void onZoomInClicked();
	void onZoomOutClicked();
	void onFitWidthClicked();

private:
	void initStatusBar();
	void startLoadPdf();

	QString m_filePath;
	QVBoxLayout* m_mainLayout = nullptr;
	QLabel* m_statusLab = nullptr;
	QPushButton* m_zoomInBtn = nullptr;
	QPushButton* m_zoomOutBtn = nullptr;
	QPushButton* m_fitWidthBtn = nullptr;
	QStackedLayout* m_stackedLayout = nullptr;
	QWidget* m_loadingPage = nullptr;
	QWidget* m_errorPage = nullptr;
	QWidget* m_previewPage = nullptr;
	QPdfDocument* m_pdfDocument = nullptr;
	QPdfView* m_pdfView = nullptr;
};
