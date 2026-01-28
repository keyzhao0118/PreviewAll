#pragma once

#include <QMainWindow>

class QLineEdit;
class QPushButton;
class PreviewHandlerHost;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);

private slots:
	void chooseFile();

private:
	void setCurrentFile(const QString& path);

	QLineEdit* m_pathEdit = nullptr;
	QPushButton* m_chooseButton = nullptr;
	PreviewHandlerHost* m_previewHost = nullptr;
	QString m_currentFilePath;
};


