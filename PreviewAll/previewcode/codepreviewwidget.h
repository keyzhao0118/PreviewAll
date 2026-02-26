#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

class QLabel;
class LineNumberArea;

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
	void highlightCurrentLine();

	int lineNumberAreaWidth() const;
	void lineNumberAreaPaintEvent(QPaintEvent* event);
	void updateLineNumberAreaWidth(int newBlockCount);
	void updateLineNumberArea(const QRect& rect, int dy);

	void zoomIn(int range = 1);
	void zoomOut(int range = 1);

	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	static constexpr int MIN_FONT_SIZE = 6;
	static constexpr int MAX_FONT_SIZE = 48;
	static constexpr qint64 MAX_FILE_SIZE = 1024 * 1024;  // 1 MB
	static constexpr int TAB_STOP_SPACES = 4;

	QString m_filePath;

	QLabel* m_infoLab = nullptr;
	QPlainTextEdit* m_editor = nullptr;
	LineNumberArea* m_lineNumberArea = nullptr;

	KSyntaxHighlighting::Repository* m_repository = nullptr;
	KSyntaxHighlighting::SyntaxHighlighter* m_highlighter = nullptr;

	int m_fontSize = 12;

	friend class LineNumberArea;
};

class LineNumberArea : public QWidget
{
public:
	explicit LineNumberArea(CodePreviewWidget* codeWidget, QPlainTextEdit* editor)
		: QWidget(editor)
		, m_codeWidget(codeWidget)
	{
	}

	QSize sizeHint() const override
	{
		return QSize(m_codeWidget->lineNumberAreaWidth(), 0);
	}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		m_codeWidget->lineNumberAreaPaintEvent(event);
	}

private:
	CodePreviewWidget* m_codeWidget;
};

