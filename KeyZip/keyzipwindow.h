#pragma once

#include <QMainWindow>

class ArchiveTreeWidget;
class ArchiveParser;

class KeyZipWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit KeyZipWindow(QWidget *parent = nullptr);
	~KeyZipWindow();

private slots:
	void onParsingFailed();
	void onEntryFound(const QString& entryPath, bool bIsDir, quint64 entrySize);

private:
	ArchiveTreeWidget* m_treeWidget = nullptr;
	ArchiveParser* m_archiveParser = nullptr;

};