#pragma once

#include <QMainWindow>
#include "archivetree.h"

class ArchiveTreeWidget;
class ArchiveParser;
class ArchiveExtractor;
class KeyCardWidget;
class QLabel;
class QStackedLayout;

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
	
	void clearTreeInfo();

	void initArchiveParser();
	void initArchiveExtractor();

private slots:
	void onOpenTriggered();
	void onNewTriggered();
	void onExtractTriggered();
	void onLocationTriggered();
	void onCloseTriggered();
	void onExitTriggered();
	void onPreviewToggled(bool checked);
	void onAboutTriggered();

	void onCentralStackedChanged(int index);

	void onRequirePassword(bool& bCancel, QString& password);
	void onUpdateProgress(quint64 completed, quint64 total);
	void onEntryFound();
	void onParseFailed();
	void onParseSucceed();

private:
	QAction* m_actOpen = nullptr;
	QAction* m_actNew = nullptr;
	QAction* m_actExtract = nullptr;
	QAction* m_actLocation = nullptr;
	QAction* m_actClose = nullptr;
	QAction* m_actExit = nullptr;

	QAction* m_actPreview = nullptr;

	QAction* m_actAbout = nullptr;

	QStackedLayout* m_centralStackedLayout = nullptr;
	ArchiveTreeWidget* m_treeWidget = nullptr;
	KeyCardWidget* m_previewPanel = nullptr;
	QLabel* m_archiveInfoLab = nullptr;

	QSharedPointer<ArchiveTree> m_archiveTree;
	QSharedPointer<ArchiveParser> m_archiveParser;
	QSharedPointer<ArchiveExtractor> m_archiveExtractor;

	QString m_archivePath;

};