#include "codepreviewwidget.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QPlainTextEdit>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

CodePreviewWidget::CodePreviewWidget(const QString& filePath, QWidget* parent)
	:QWidget(parent)
	, m_filePath(filePath)
{
	initUi();
	initHighlighter();
	loadFile();
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
	QFile file(m_filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream ts(&file);
	ts.setCodec("UTF-8");

	const QString content = ts.readAll();
	m_editor->setPlainText(content);
	
	int lineCount = m_editor->blockCount();
	int charCount = content.length();

	m_infoLab->setText(tr("Line: %1, Char: %2").arg(lineCount).arg(charCount));
}
