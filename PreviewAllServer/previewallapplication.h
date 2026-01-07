#pragma once

#include <QApplication>
#include <QLocalServer>
#include <QWidget>

class PreviewAllApplication  : public QApplication
{
	Q_OBJECT

public:
	PreviewAllApplication(int& argc, char** argv);
	~PreviewAllApplication();

	void startWindowManageService();

private:
	HWND handleCreateCmd(HWND hwndParent, const QString& filePath);
	void handleResizeCmd(HWND hwndPreview, int width, int height);
	void handleCloseCmd(HWND hwndPreview);
	QSharedPointer<QWidget> createPreviewWidget(const QString& filePath);

private slots:
	void onNewConnection();
	void onReadyRead();

private:
	QLocalServer* m_previewAllServer = nullptr;
	QHash<HWND, QSharedPointer<QWidget>> m_widgetHash;
};

#ifndef previewAllApp
#define previewAllApp (static_cast<PreviewAllApplication*>(QCoreApplication::instance()))
#endif