#pragma once

#include <QWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QStackedLayout>

class ArchiveParser;

class ArchivePreviewWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ArchivePreviewWidget(const QString& filePath, QWidget* parent = nullptr);
	~ArchivePreviewWidget();

	void startParseArchive();

private slots:
	void showLoadingPage();
	void showErrorPage();
	void showEncryptPage();
	void showPreviewPage();

	void onExtractBtnClicked();

private:
	void createInfoLab();
	void craetePreviewPage();

private:
	QString m_filePath;

	QStackedLayout* m_stackedLayout = nullptr;
	QLabel* m_loadingLab = nullptr;
	QLabel* m_infoLab = nullptr;
	QWidget* m_previewPage = nullptr;

	ArchiveParser* m_archiveParser = nullptr;
	QThread* m_parserThread = nullptr;

};