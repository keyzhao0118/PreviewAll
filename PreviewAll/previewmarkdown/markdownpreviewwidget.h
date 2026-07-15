#pragma once

#include <QByteArray>
#include <QString>
#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTextBrowser;

class MarkdownPreviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MarkdownPreviewWidget(const QString& filePath, QWidget* parent = nullptr);

private:
	void initUi();
	void loadFile();
	void setSourceMode(bool sourceMode);
	QString decodeText(const QByteArray& bytes) const;

	QString m_filePath;
	QString m_markdown;
	QPushButton* m_sourceButton = nullptr;
	QStackedWidget* m_viewStack = nullptr;
	QTextBrowser* m_renderedView = nullptr;
	QPlainTextEdit* m_sourceView = nullptr;
};