#include "codepreviewwidget.h"
#include "codeeditorwidget.h"
#include "codesearchbar.h"
#include <QVBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QThread>
#include <QPointer>
#include <QTimer>
#include <QSvgRenderer>
#include <QPainter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

CodePreviewWidget::CodePreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
{
	initUi();
	initHighlighter();
	initShortcuts();
	QTimer::singleShot(0, this, &CodePreviewWidget::loadFile);
}

CodePreviewWidget::~CodePreviewWidget()
{
	delete m_textStream;
	delete m_file;
}

void CodePreviewWidget::initUi()
{
	setWindowFlags(Qt::FramelessWindowHint);
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	initStatusBar();
	initSearchBar();
	initTextEditor();

	mainLayout->addWidget(m_statusBar);
	mainLayout->addWidget(m_searchBar);
	mainLayout->addWidget(m_textEditor);
}

void CodePreviewWidget::initStatusBar()
{
	m_statusBar = new QWidget(this);
	m_statusBar->setFixedHeight(50);

	auto statusLayout = new QHBoxLayout(m_statusBar);
	statusLayout->setContentsMargins(10, 0, 10, 0);
	statusLayout->setSpacing(10);

	QLabel* codeIconLab = new QLabel(m_statusBar);
	QSvgRenderer svgRenderer(QString(":/svg/code.svg"));
	QPixmap codePix(30, 30);
	codePix.fill(Qt::transparent);
	QPainter painter(&codePix);
	svgRenderer.render(&painter);
	painter.end();
	codeIconLab->setPixmap(codePix);

	m_infoLab = new QLabel(m_statusBar);

	QPushButton* searchBtn = new QPushButton(m_statusBar);
	searchBtn->setIcon(QIcon(":/svg/search.svg"));
	searchBtn->setIconSize(QSize(30, 30));
	searchBtn->setToolTip(tr("Search"));
	searchBtn->setStyleSheet(
		"QPushButton { border: none; border-radius: 4px; padding: 4px; background: transparent; }"
		"QPushButton:hover { background-color: rgba(128, 128, 128, 50); }"
		"QPushButton:pressed { background-color: rgba(128, 128, 128, 100); }"
	);
	connect(searchBtn, &QPushButton::clicked, this, &CodePreviewWidget::onSearchBtnClicked);

	statusLayout->addWidget(codeIconLab);
	statusLayout->addWidget(m_infoLab);
	statusLayout->addStretch();
	statusLayout->addWidget(searchBtn);

	QFont font("Microsoft YaHei");
	font.setPointSize(9);
	m_statusBar->setFont(font);
}

void CodePreviewWidget::initSearchBar()
{
	m_searchBar = new CodeSearchBar(this);
	m_searchBar->setVisible(false);

	connect(m_searchBar, &CodeSearchBar::searchChanged, this, &CodePreviewWidget::performSearch);
	connect(m_searchBar, &CodeSearchBar::findNext, this, &CodePreviewWidget::findNext);
	connect(m_searchBar, &CodeSearchBar::findPrevious, this, &CodePreviewWidget::findPrevious);
	connect(m_searchBar, &CodeSearchBar::closed, this, [this]() {
		m_searchBar->setVisible(false);
		clearSearchHighlights();
		m_textEditor->setFocus();
	});
}

void CodePreviewWidget::initTextEditor()
{
	m_textEditor = new CodeEditorWidget(this);
	m_textEditor->setFrameShape(QFrame::NoFrame);
	m_textEditor->setReadOnly(true);
	m_textEditor->setLineWrapMode(QPlainTextEdit::NoWrap);

	QFont font("Microsoft YaHei");
	font.setFamily("Consolas");
	font.setPointSize(12);
	m_textEditor->setFont(font);

	connect(m_textEditor->verticalScrollBar(), &QScrollBar::valueChanged,
		this, &CodePreviewWidget::onScrollValueChanged);
}

void CodePreviewWidget::initHighlighter()
{
	m_repository = new KSyntaxHighlighting::Repository;
	const auto def = m_repository->definitionForFileName(m_filePath);
	m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(m_textEditor->document());
	m_highlighter->setDefinition(def);
	m_highlighter->setTheme(m_repository->defaultTheme());
}

void CodePreviewWidget::initShortcuts()
{
	auto* searchShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
	connect(searchShortcut, &QShortcut::activated, this, &CodePreviewWidget::onSearchBtnClicked);
}

void CodePreviewWidget::loadFile()
{
	QPointer<CodePreviewWidget> that(this);
	QThread* loadThread = QThread::create([that]() {
		if (!that)
			return;

		QFileInfo fi(that->m_filePath);
		bool chunked = fi.size() > FileSizeThreshold;

		QFile file(that->m_filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QMetaObject::invokeMethod(that, [that]() {
				if (!that)
					return;
				that->m_textEditor->setPlainText(QString());
				that->m_infoLab->setText(tr("Failed to load file"));
			}, Qt::QueuedConnection);
			return;
		}

		if (!chunked)
		{
			QTextStream ts(&file);
			const QString content = ts.readAll();
			QMetaObject::invokeMethod(that, [that, content]() {
				if (!that)
					return;
				that->m_isFullyLoaded = true;
				that->m_textEditor->setPlainText(content);
				that->updateInfoLabel();
			}, Qt::QueuedConnection);
		}
		else
		{
			// 分块加载：先读取首个 chunk
			QTextStream ts(&file);
			QString chunk;
			int lineCount = 0;
			while (!ts.atEnd() && lineCount < ChunkLineCount)
			{
				chunk += ts.readLine() + QLatin1Char('\n');
				++lineCount;
			}
			bool atEnd = ts.atEnd();

			// 统计总行数（快速扫描）
			qint64 savedPos = ts.pos();
			int totalLines = lineCount;
			if (!atEnd)
			{
				while (!ts.atEnd())
				{
					ts.readLine();
					++totalLines;
				}
			}

			QMetaObject::invokeMethod(that, [that, chunk, lineCount, totalLines, atEnd]() {
				if (!that)
					return;

				that->m_totalLineCount = totalLines;
				that->m_textEditor->setPlainText(chunk);

				if (atEnd)
				{
					that->m_isFullyLoaded = true;
				}
				else
				{
					// 保持文件打开用于后续分块读取
					that->m_file = new QFile(that->m_filePath);
					that->m_file->open(QIODevice::ReadOnly | QIODevice::Text);
					that->m_textStream = new QTextStream(that->m_file);
					// 跳过已经读取的行
					for (int i = 0; i < lineCount; ++i)
						that->m_textStream->readLine();
				}
				that->updateInfoLabel();
			}, Qt::QueuedConnection);
		}
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void CodePreviewWidget::loadNextChunk()
{
	if (m_isFullyLoaded || m_isLoadingChunk || !m_textStream)
		return;

	m_isLoadingChunk = true;

	QPointer<CodePreviewWidget> that(this);

	QThread* loadThread = QThread::create([that]() {
		if (!that || !that->m_textStream)
			return;

		QString chunk;
		int lineCount = 0;
		while (!that->m_textStream->atEnd() && lineCount < ChunkLineCount)
		{
			chunk += that->m_textStream->readLine() + QLatin1Char('\n');
			++lineCount;
		}
		bool atEnd = that->m_textStream->atEnd();

		QMetaObject::invokeMethod(that, [that, chunk, atEnd]() {
			if (!that)
				return;

			if (atEnd)
			{
				that->m_isFullyLoaded = true;
				delete that->m_textStream;
				that->m_textStream = nullptr;
				delete that->m_file;
				that->m_file = nullptr;
			}

			// 追加文本
			QTextCursor cursor(that->m_textEditor->document());
			cursor.movePosition(QTextCursor::End);
			cursor.insertText(chunk);

			that->m_isLoadingChunk = false;
			that->updateInfoLabel();
		}, Qt::QueuedConnection);
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void CodePreviewWidget::updateInfoLabel()
{
	int loadedLines = m_textEditor->blockCount();
	int charCount = m_textEditor->document()->characterCount() - 1;

	if (m_isFullyLoaded)
	{
		m_infoLab->setText(tr("Line: %1, Char: %2").arg(loadedLines).arg(charCount));
	}
	else
	{
		m_infoLab->setText(tr("Line: %1/%2, Char: %3")
			.arg(loadedLines).arg(m_totalLineCount).arg(charCount));
	}
}

void CodePreviewWidget::onScrollValueChanged(int value)
{
	if (m_isFullyLoaded || m_isLoadingChunk)
		return;

	QScrollBar* sb = m_textEditor->verticalScrollBar();
	int maximum = sb->maximum();
	if (maximum > 0 && value >= maximum * 0.9)
		loadNextChunk();
}

void CodePreviewWidget::onSearchBtnClicked()
{
	bool visible = !m_searchBar->isVisible();
	m_searchBar->setVisible(visible);

	if (visible)
	{
		// 搜索时自动加载剩余内容
		if (!m_isFullyLoaded && m_textStream)
		{
			QPointer<CodePreviewWidget> that(this);
			QThread* loadAllThread = QThread::create([that]() {
				if (!that || !that->m_textStream)
					return;

				QString remaining;
				while (!that->m_textStream->atEnd())
					remaining += that->m_textStream->readLine() + QLatin1Char('\n');

				QMetaObject::invokeMethod(that, [that, remaining]() {
					if (!that)
						return;
					that->m_isFullyLoaded = true;
					delete that->m_textStream;
					that->m_textStream = nullptr;
					delete that->m_file;
					that->m_file = nullptr;

					QTextCursor cursor(that->m_textEditor->document());
					cursor.movePosition(QTextCursor::End);
					cursor.insertText(remaining);
					that->updateInfoLabel();

					// 触发重新搜索
					that->performSearch(that->m_searchBar->searchText());
				}, Qt::QueuedConnection);
			});
			connect(loadAllThread, &QThread::finished, loadAllThread, &QObject::deleteLater);
			loadAllThread->start();
		}

		m_searchBar->focusInput();
	}
	else
	{
		clearSearchHighlights();
		m_textEditor->setFocus();
	}
}

// --- Search logic ---

void CodePreviewWidget::performSearch(const QString& text)
{
	m_matchPositions.clear();
	m_currentMatchIndex = -1;

	if (text.isEmpty())
	{
		clearSearchHighlights();
		m_searchBar->setMatchInfo(0, 0);
		return;
	}

	QTextDocument::FindFlags flags;
	if (m_searchBar->isCaseSensitive())
		flags |= QTextDocument::FindCaseSensitively;

	QTextDocument* doc = m_textEditor->document();
	QTextCursor cursor(doc);
	while (true)
	{
		cursor = doc->find(text, cursor, flags);
		if (cursor.isNull())
			break;
		m_matchPositions.append(cursor);
	}

	highlightAllMatches();

	if (!m_matchPositions.isEmpty())
	{
		m_currentMatchIndex = 0;
		navigateToMatch(0);
	}

	m_searchBar->setMatchInfo(
		m_matchPositions.isEmpty() ? 0 : 1,
		m_matchPositions.size()
	);
}

void CodePreviewWidget::highlightAllMatches()
{
	QList<QTextEdit::ExtraSelection> selections;

	QColor matchColor(255, 255, 0, 80);     // 黄色高亮
	QColor currentColor(255, 165, 0, 120);   // 橙色当前项

	for (int i = 0; i < m_matchPositions.size(); ++i)
	{
		QTextEdit::ExtraSelection sel;
		sel.cursor = m_matchPositions[i];
		sel.format.setBackground(i == m_currentMatchIndex ? currentColor : matchColor);
		selections.append(sel);
	}

	m_textEditor->setExtraSelections(selections);
}

void CodePreviewWidget::findNext()
{
	if (m_matchPositions.isEmpty())
		return;

	m_currentMatchIndex = (m_currentMatchIndex + 1) % m_matchPositions.size();
	navigateToMatch(m_currentMatchIndex);
	highlightAllMatches();
	m_searchBar->setMatchInfo(m_currentMatchIndex + 1, m_matchPositions.size());
}

void CodePreviewWidget::findPrevious()
{
	if (m_matchPositions.isEmpty())
		return;

	m_currentMatchIndex = (m_currentMatchIndex - 1 + m_matchPositions.size()) % m_matchPositions.size();
	navigateToMatch(m_currentMatchIndex);
	highlightAllMatches();
	m_searchBar->setMatchInfo(m_currentMatchIndex + 1, m_matchPositions.size());
}

void CodePreviewWidget::clearSearchHighlights()
{
	m_matchPositions.clear();
	m_currentMatchIndex = -1;
	m_textEditor->setExtraSelections({});
}

void CodePreviewWidget::navigateToMatch(int index)
{
	if (index < 0 || index >= m_matchPositions.size())
		return;

	QTextCursor cursor = m_matchPositions[index];
	m_textEditor->setTextCursor(cursor);
	m_textEditor->centerCursor();
}
