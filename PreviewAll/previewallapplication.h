#pragma once

#include <QApplication>
#include <QHash>
#include <QLocalServer>
#include <QSharedPointer>
#include <QWidget>
#include <QWindow>
#include <Windows.h>

class PreviewAllApplication  : public QApplication
{
	Q_OBJECT

public:
	PreviewAllApplication(int& argc, char** argv);
	~PreviewAllApplication();

	void initTranslations();
	void startWindowManageService();

private:
	HWND handleCreateCmd(HWND hwndParent, const QString& filePath);
	void handleCloseCmd(HWND hwndPreview);
	QSharedPointer<QWidget> createPreviewWidget(const QString& filePath);

private slots:
	void onNewConnection();
	void onReadyRead();

private:
	struct EmbeddedPreview
	{
		// The widget is destroyed before its foreign QWindow parent.
		QSharedPointer<QWindow> hostWindow;
		QSharedPointer<QWidget> widget;
	};

	QLocalServer* m_previewAllServer = nullptr;
	QHash<HWND, EmbeddedPreview> m_previews;
};

#ifndef previewAllApp
#define previewAllApp (static_cast<PreviewAllApplication*>(QCoreApplication::instance()))
#endif
