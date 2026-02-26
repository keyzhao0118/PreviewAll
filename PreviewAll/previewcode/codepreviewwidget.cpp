#include "codepreviewwidget.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QPlainTextEdit>
#include <QThread>
#include <QPointer>
#include <QTimer>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QTextCodec>
#include <QFontMetrics>
#include <QWheelEvent>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

CodePreviewWidget::CodePreviewWidget(const QString& filePath, QWidget* parent)
	:QWidget(parent)
	, m_filePath(filePath)
{
	initUi();
	initHighlighter();
	QTimer::singleShot(0, this, &CodePreviewWidget::loadFile);
}

CodePreviewWidget::~CodePreviewWidget()
{
}

void CodePreviewWidget::initUi()
{
	setWindowFlags(Qt::FramelessWindowHint);
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	QWidget* statusWidget = new QWidget(this);
	statusWidget->setFixedHeight(35);
	auto statusLayout = new QHBoxLayout(statusWidget);
	statusLayout->setContentsMargins(5, 0, 5, 0);
	statusLayout->setSpacing(5);

	QLabel* codeIconLab = new QLabel(this);
	QPixmap codePix(":/png/code.png");
	codePix = codePix.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	codeIconLab->setPixmap(codePix);

	m_infoLab = new QLabel(this);

	statusLayout->addWidget(codeIconLab);
	statusLayout->addWidget(m_infoLab);
	statusLayout->addStretch();

	m_editor = new QPlainTextEdit(this);
	m_editor->setFrameShape(QFrame::NoFrame);
	m_editor->setReadOnly(true);
	m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

	// Font fallback mechanism
	QFont statusFont("Microsoft YaHei");
	statusFont.setPointSize(9);
	m_infoLab->setFont(statusFont);

	QFont editorFont("Consolas");
	editorFont.setStyleHint(QFont::Monospace);
	editorFont.setPointSize(m_fontSize);
	QStringList fallbackFamilies;
	fallbackFamilies << "Source Code Pro" << "Courier New" << "DejaVu Sans Mono" << "Monospace" << "Microsoft YaHei";
	editorFont.setFamilies(QStringList() << "Consolas" << fallbackFamilies);
	m_editor->setFont(editorFont);

	// Tab width setting
	QFontMetrics fm(editorFont);
	m_editor->setTabStopDistance(fm.horizontalAdvance(' ') * TAB_STOP_SPACES);

	// Line number area
	m_lineNumberArea = new LineNumberArea(this, m_editor);

	connect(m_editor, &QPlainTextEdit::blockCountChanged,
		this, &CodePreviewWidget::updateLineNumberAreaWidth);
	connect(m_editor, &QPlainTextEdit::updateRequest,
		this, &CodePreviewWidget::updateLineNumberArea);
	connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
		this, &CodePreviewWidget::highlightCurrentLine);

	updateLineNumberAreaWidth(0);

	m_editor->viewport()->installEventFilter(this);

	mainLayout->addWidget(statusWidget);
	mainLayout->addWidget(m_editor);
}

void CodePreviewWidget::initHighlighter()
{
	m_repository = new KSyntaxHighlighting::Repository;

	// 根据文件名自动识别语法
	const auto def = m_repository->definitionForFileName(m_filePath);

	m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(m_editor->document());
	m_highlighter->setDefinition(def);

	// 主题可选：Dark / Light
	m_highlighter->setTheme(m_repository->defaultTheme());
}

void CodePreviewWidget::loadFile()
{
	QPointer<CodePreviewWidget> that(this);
	QThread* loadThread = QThread::create([that]() {
		if (!that)
			return;

		QFile file(that->m_filePath);
		if (file.open(QIODevice::ReadOnly))
		{
			const qint64 fileSize = file.size();
			const bool truncated = fileSize > MAX_FILE_SIZE;
			const QByteArray rawData = truncated ? file.read(MAX_FILE_SIZE) : file.readAll();

			// File encoding auto-detection
			QTextCodec* codec = nullptr;

			// Check BOM
			if (rawData.startsWith("\xEF\xBB\xBF"))
				codec = QTextCodec::codecForName("UTF-8");
			else if (rawData.startsWith("\xFF\xFE"))
				codec = QTextCodec::codecForName("UTF-16LE");
			else if (rawData.startsWith("\xFE\xFF"))
				codec = QTextCodec::codecForName("UTF-16BE");

			// Try UTF-8 validation
			if (!codec)
			{
				QTextCodec* utf8Codec = QTextCodec::codecForName("UTF-8");
				if (utf8Codec)
				{
					QTextCodec::ConverterState state;
					utf8Codec->toUnicode(rawData.constData(), rawData.size(), &state);
					if (state.invalidChars == 0)
						codec = utf8Codec;
				}
			}

			// Fallback to system locale
			if (!codec)
				codec = QTextCodec::codecForLocale();

			const QString content = codec->toUnicode(rawData);

			// 切回 UI 线程：在这里才设置文本内容
			QMetaObject::invokeMethod(that, [that, content, fileSize, truncated]() {
				if (!that)
					return;
				that->m_editor->setPlainText(content);
				int lineCount = that->m_editor->blockCount();
				int charCount = content.length();
				QString info = tr("Line: %1, Char: %2").arg(lineCount).arg(charCount);
				if (truncated)
					info += tr(" [Truncated: file size %1 bytes exceeds limit]").arg(fileSize);
				that->m_infoLab->setText(info);
				that->highlightCurrentLine();
				}, Qt::QueuedConnection);
		}
		else
		{
			// 切回 UI 线程：在这里才设置文本内容
			QMetaObject::invokeMethod(that, [that]() {
				if (that)
				{
					that->m_editor->setPlainText(QString());
					that->m_infoLab->setText(tr("Failed to load file."));
				}
			}, Qt::QueuedConnection);
		}
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void CodePreviewWidget::highlightCurrentLine()
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	QTextEdit::ExtraSelection selection;
	QColor lineColor = QColor(Qt::yellow).lighter(180);
	selection.format.setBackground(lineColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = m_editor->textCursor();
	selection.cursor.clearSelection();
	extraSelections.append(selection);

	m_editor->setExtraSelections(extraSelections);
}

int CodePreviewWidget::lineNumberAreaWidth() const
{
	int digits = 1;
	int max = qMax(1, m_editor->blockCount());
	while (max >= 10)
	{
		max /= 10;
		++digits;
	}
	digits = qMax(digits, 3);

	int space = 10 + m_editor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
	return space;
}

void CodePreviewWidget::lineNumberAreaPaintEvent(QPaintEvent* event)
{
	QPainter painter(m_lineNumberArea);
	painter.fillRect(event->rect(), QColor(240, 240, 240));

	QTextBlock block = m_editor->firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = qRound(m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top());
	int bottom = top + qRound(m_editor->blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			QString number = QString::number(blockNumber + 1);
			painter.setPen(QColor(130, 130, 130));
			painter.drawText(0, top, m_lineNumberArea->width() - 5,
				m_editor->fontMetrics().height(),
				Qt::AlignRight, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + qRound(m_editor->blockBoundingRect(block).height());
		++blockNumber;
	}
}

void CodePreviewWidget::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
	m_editor->setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodePreviewWidget::updateLineNumberArea(const QRect& rect, int dy)
{
	if (dy)
		m_lineNumberArea->scroll(0, dy);
	else
		m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

	if (rect.contains(m_editor->viewport()->rect()))
		updateLineNumberAreaWidth(0);
}

void CodePreviewWidget::zoomIn(int range)
{
	m_fontSize = qMin(m_fontSize + range, MAX_FONT_SIZE);
	QFont f = m_editor->font();
	f.setPointSize(m_fontSize);
	m_editor->setFont(f);

	QFontMetrics fm(f);
	m_editor->setTabStopDistance(fm.horizontalAdvance(' ') * TAB_STOP_SPACES);
	updateLineNumberAreaWidth(0);
}

void CodePreviewWidget::zoomOut(int range)
{
	m_fontSize = qMax(m_fontSize - range, MIN_FONT_SIZE);
	QFont f = m_editor->font();
	f.setPointSize(m_fontSize);
	m_editor->setFont(f);

	QFontMetrics fm(f);
	m_editor->setTabStopDistance(fm.horizontalAdvance(' ') * TAB_STOP_SPACES);
	updateLineNumberAreaWidth(0);
}

bool CodePreviewWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_editor->viewport() && event->type() == QEvent::Wheel)
	{
		QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
		if (wheelEvent->modifiers() & Qt::ControlModifier)
		{
			if (wheelEvent->angleDelta().y() > 0)
				zoomIn();
			else if (wheelEvent->angleDelta().y() < 0)
				zoomOut();
			return true;
		}
	}
	return QWidget::eventFilter(obj, event);
}
