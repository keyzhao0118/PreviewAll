#pragma once

#include <QWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QStackedLayout>

class ArchiveTreeWidget;
class ArchiveParser;

class ArchivePreviewWidget : public QWidget
{
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
	QString m_filePath;

	QStackedLayout* m_stackedLayout = nullptr;
	QLabel* m_loadingLab = nullptr;
	QLabel* m_errorLab = nullptr;
	QLabel* m_encryptLab = nullptr;
	ArchiveTreeWidget* m_treeWidget = nullptr;

	ArchiveParser* m_archiveParser = nullptr;
	QThread* m_parserThread = nullptr;

};