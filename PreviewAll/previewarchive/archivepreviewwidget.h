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

private:
	void initStatusBar();

	void createLoadingPage();
	void createErrorPage();
	void createEncryptPage();
	void createPreviewPage();

private:
	QString m_filePath;

	QVBoxLayout* m_mainLayout = nullptr;
	QStackedLayout* m_stackedLayout = nullptr;
	QWidget* m_loadingPage = nullptr;
	QWidget* m_errorPage = nullptr;
	QWidget* m_encryptPage = nullptr;
	QWidget* m_previewPage = nullptr;
	QLabel* m_statusLab = nullptr;

	ArchiveParser* m_archiveParser = nullptr;
	QThread* m_parserThread = nullptr;

};