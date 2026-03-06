#include "codepreviewwidget.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QThread>
#include <QPointer>
#include <QTimer>
#include <QSvgRenderer>
#include <QPainter>
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
	
	initStatusBar();
	initTextEditor();

	mainLayout->addWidget(m_statusBar);
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

void CodePreviewWidget::initTextEditor()
{
	m_textEditor = new QPlainTextEdit(this);
	m_textEditor->setFrameShape(QFrame::NoFrame);
	m_textEditor->setReadOnly(true);
	m_textEditor->setLineWrapMode(QPlainTextEdit::NoWrap);

	QFont font("Microsoft YaHei");
	font.setFamily("Consolas");
	font.setPointSize(12);
	m_textEditor->setFont(font);
}

void CodePreviewWidget::initHighlighter()
{
	m_repository = new KSyntaxHighlighting::Repository;
	const auto def = m_repository->definitionForFileName(m_filePath);
	m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(m_textEditor->document());
	m_highlighter->setDefinition(def);
	m_highlighter->setTheme(m_repository->defaultTheme());
}

void CodePreviewWidget::loadFile()
{
	QPointer<CodePreviewWidget> that(this);
	QThread* loadThread = QThread::create([that]() {
		if (!that)
			return;

		QFile file(that->m_filePath);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream ts(&file);
			const QString content = ts.readAll();

			// 切回 UI 线程：在这里才设置文本内容
			QMetaObject::invokeMethod(that, [that, content]() {
				if (!that)
					return;
				that->m_textEditor->setPlainText(content);
				int lineCount = that->m_textEditor->blockCount();
				int charCount = content.length();
				that->m_infoLab->setText(tr("Line: %1, Char: %2").arg(lineCount).arg(charCount));
				}, Qt::QueuedConnection);
		}
		else
		{
			// 切回 UI 线程：在这里才设置文本内容
			QMetaObject::invokeMethod(that, [that]() {
				if (that)
				{
					that->m_textEditor->setPlainText(QString());
					that->m_infoLab->setText(tr("Failed to load file"));
				}
			}, Qt::QueuedConnection);
		}
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void CodePreviewWidget::onSearchBtnClicked()
{
	// 这里可以实现搜索功能，例如弹出一个搜索框，或者在编辑器上方显示一个搜索栏
}
