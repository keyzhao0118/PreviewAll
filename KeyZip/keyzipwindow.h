#pragma once

#include <QMainWindow>

class ArchiveTreeWidget;
class ArchiveParser;
class KeyCardWidget;

class KeyZipWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit KeyZipWindow(QWidget *parent = nullptr);
	~KeyZipWindow();

private:
	void initMenuAction();
	void initCentralWidget();
	void initStatusBar();
	void initArchiveParser();

private slots:
	void onRequirePassword(bool& bCancel, QString& password);
	void onParsingFailed();
	void onEntryFound(const QString& entryPath, bool bIsDir, quint64 entrySize);

private:
	ArchiveTreeWidget* m_treeWidget = nullptr;
	KeyCardWidget* m_previewPanel = nullptr;

	ArchiveParser* m_archiveParser = nullptr;

	QString m_archivePath;

};