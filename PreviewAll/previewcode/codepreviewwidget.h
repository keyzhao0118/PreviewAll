#pragma once

#include <QWidget>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

class QLabel;
class QPlainTextEdit;

class CodePreviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CodePreviewWidget(const QString& filePath, QWidget* parent = nullptr);
	~CodePreviewWidget();

private:
	void initUi();
	void initStatusBar();
	void initTextEditor();
	void initHighlighter();
	void loadFile();

private slots:
	void onSearchBtnClicked();

private:
	QString m_filePath;

	QWidget* m_statusBar = nullptr;
	QLabel* m_infoLab = nullptr;
	QPlainTextEdit* m_textEditor = nullptr;

	KSyntaxHighlighting::Repository* m_repository = nullptr;
	KSyntaxHighlighting::SyntaxHighlighter* m_highlighter = nullptr;

};

