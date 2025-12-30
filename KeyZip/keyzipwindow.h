#pragma once

#include <QMainWindow>

class ArchiveTreeWidget;
class ArchiveParser;
class ArchiveExtractor;
class KeyCardWidget;
class QLabel;
class QStackedLayout;
class QProgressDialog;

class KeyZipWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit KeyZipWindow(QWidget *parent = nullptr);
	~KeyZipWindow();

private:
	void initAction();
	void initMenu();
	void initToolBar();
	void initCentralWidget();
	void initStatusBar();
	
	void clearOld();

	void startArchiveParser();
	void startArchiveExtractor(const QString& archivePath, const QString& destDirPath);

private slots:
	void onOpenTriggered();
	void onNewTriggered();
	void onExtractAllTriggered();
	void onExtractSelectTriggered();
	void onLocationTriggered();
	void onCloseTriggered();
	void onExitTriggered();
	void onAddTriggered();
	void onDeleteTriggered();
	void onPreviewToggled(bool checked);
	void onAboutTriggered();

	void onCentralStackedChanged(int index);

	void onRequirePassword(bool& bCancel, QString& password);
	void onUpdateParseProgress(quint64 completed, quint64 total);
	void onParseFailed();
	void onParseSucceed();

	void onUpdateExtractProgress(quint64 completed, quint64 total);
	void onExtractFailed();
	void onExtractSucceed();

private:
	QAction* m_actOpen = nullptr;
	QAction* m_actNew = nullptr;
	QAction* m_actExtractAll = nullptr;
	QAction* m_actExtractSelect = nullptr;
	QAction* m_actLocation = nullptr;
	QAction* m_actClose = nullptr;
	QAction* m_actExit = nullptr;

	QAction* m_actAdd = nullptr;
	QAction* m_actDelete = nullptr;

	QAction* m_actPreview = nullptr;
	QAction* m_actAbout = nullptr;

	QToolBar* m_toolBar = nullptr;

	QStackedLayout* m_centralStackedLayout = nullptr;
	ArchiveTreeWidget* m_treeWidget = nullptr;
	KeyCardWidget* m_previewPanel = nullptr;
	QLabel* m_archiveInfoLab = nullptr;

	QProgressDialog* m_parseProgressDlg = nullptr;
	bool m_bParseCanceled = false;

	QProgressDialog* m_extractProgressDlg = nullptr;
	bool m_bExtractCanceled = false;

	QSharedPointer<ArchiveParser> m_archiveParser;
	QSharedPointer<ArchiveExtractor> m_archiveExtractor;

	QString m_archivePath;

};