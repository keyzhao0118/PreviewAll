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
	void initHighlighter();
	void loadFile();

private:
	QString m_filePath;

	QLabel* m_infoLab = nullptr;
	QPlainTextEdit* m_editor = nullptr;

	KSyntaxHighlighting::Repository* m_repository = nullptr;
	KSyntaxHighlighting::SyntaxHighlighter* m_highlighter = nullptr;

};

