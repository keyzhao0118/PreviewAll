#pragma once

#include <QWidget>
#include <QSharedPointer>
#include <QStackedLayout>

class QVBoxLayout;

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

	QSharedPointer<ArchiveParser> m_archiveParser;

};
