#include "codepreviewwidget.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QLabel>
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

	QWidget* statusWidget = new QWidget(this);
	statusWidget->setFixedHeight(50);
	auto statusLayout = new QHBoxLayout(statusWidget);
	statusLayout->setContentsMargins(10, 0, 10, 0);
	statusLayout->setSpacing(10);

	QLabel* codeIconLab = new QLabel(this);
	QSvgRenderer svgRenderer(QString(":/svg/code.svg"));
	QPixmap codePix(30, 30);
	codePix.fill(Qt::transparent);
	QPainter painter(&codePix);
	svgRenderer.render(&painter);
	painter.end();
	codeIconLab->setPixmap(codePix);

	m_infoLab = new QLabel(this);

	statusLayout->addWidget(codeIconLab);
	statusLayout->addWidget(m_infoLab);
	statusLayout->addStretch();

	m_editor = new QPlainTextEdit(this);
	m_editor->setFrameShape(QFrame::NoFrame);
	m_editor->setReadOnly(true);
	m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

	QFont font("Microsoft YaHei");
	font.setPointSize(9);
	m_infoLab->setFont(font);

	font.setFamily("Consolas");
	font.setPointSize(12);
	m_editor->setFont(font);

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
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream ts(&file);
			const QString content = ts.readAll();

			// 切回 UI 线程：在这里才设置文本内容
			QMetaObject::invokeMethod(that, [that, content]() {
				if (!that)
					return;
				that->m_editor->setPlainText(content);
				int lineCount = that->m_editor->blockCount();
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
					that->m_editor->setPlainText(QString());
					that->m_infoLab->setText(tr("Failed to load file."));
				}
			}, Qt::QueuedConnection);
		}
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}
