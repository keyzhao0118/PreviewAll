#pragma once

#include <QPlainTextEdit>

class LineNumberArea;

class CodeEditorWidget : public QPlainTextEdit
{
	Q_OBJECT

public:
	explicit CodeEditorWidget(QWidget* parent = nullptr);

	void lineNumberAreaPaintEvent(QPaintEvent* event);
	int lineNumberAreaWidth() const;

protected:
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void updateLineNumberAreaWidth(int newBlockCount);
	void updateLineNumberArea(const QRect& rect, int dy);

private:
	LineNumberArea* m_lineNumberArea = nullptr;
};

class LineNumberArea : public QWidget
{
public:
	explicit LineNumberArea(CodeEditorWidget* editor)
		: QWidget(editor)
		, m_codeEditor(editor)
	{
	}

	QSize sizeHint() const override
	{
		return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
	}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		m_codeEditor->lineNumberAreaPaintEvent(event);
	}

private:
	CodeEditorWidget* m_codeEditor = nullptr;
};
