#pragma once

#include <QMainWindow>
#include "archivetree.h"

class ArchiveTreeWidget;
class ArchiveParser;
class KeyCardWidget;
class QLabel;

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

	void clear();

private slots:
	void onRequirePassword(bool& bCancel, QString& password);
	void onUpdateProgress(quint64 completed, quint64 total);
	void onEntryFound(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime);
	void onParsingFailed();
	void onParsingSucceed();

private:
	ArchiveTreeWidget* m_treeWidget = nullptr;
	KeyCardWidget* m_previewPanel = nullptr;
	QLabel* m_archiveInfoLab = nullptr;

	ArchiveParser* m_archiveParser = nullptr;
	QSharedPointer<ArchiveTree> m_archiveTree;

	QString m_archivePath;

};