#include "codeeditorwidget.h"
#include <QPainter>
#include <QTextBlock>

CodeEditorWidget::CodeEditorWidget(QWidget* parent)
	: QPlainTextEdit(parent)
{
	m_lineNumberArea = new LineNumberArea(this);

	connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditorWidget::updateLineNumberAreaWidth);
	connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditorWidget::updateLineNumberArea);

	updateLineNumberAreaWidth(0);
}

int CodeEditorWidget::lineNumberAreaWidth() const
{
	int digits = 1;
	int max = qMax(1, blockCount());
	while (max >= 10)
	{
		max /= 10;
		++digits;
	}
	digits = qMax(digits, 3);

	int space = 16 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
	return space;
}

void CodeEditorWidget::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
	setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditorWidget::updateLineNumberArea(const QRect& rect, int dy)
{
	if (dy)
		m_lineNumberArea->scroll(0, dy);
	else
		m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

	if (rect.contains(viewport()->rect()))
		updateLineNumberAreaWidth(0);
}

void CodeEditorWidget::resizeEvent(QResizeEvent* event)
{
	QPlainTextEdit::resizeEvent(event);
	QRect cr = contentsRect();
	m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditorWidget::lineNumberAreaPaintEvent(QPaintEvent* event)
{
	QPainter painter(m_lineNumberArea);
	painter.fillRect(event->rect(), palette().window());

	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
	int bottom = top + qRound(blockBoundingRect(block).height());

	QColor lineNumColor = palette().placeholderText().color();

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			QString number = QString::number(blockNumber + 1);
			painter.setPen(lineNumColor);
			painter.drawText(0, top, m_lineNumberArea->width() - 8,
				fontMetrics().height(), Qt::AlignRight | Qt::AlignVCenter, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + qRound(blockBoundingRect(block).height());
		++blockNumber;
	}
}
