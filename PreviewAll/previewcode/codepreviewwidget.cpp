#include "codepreviewwidget.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
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
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_editor = new QPlainTextEdit(this);
	m_editor->setReadOnly(true);
	m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

	QFont font;
	font.setFamily("Consolas");
	font.setPointSize(12);
	font.setFixedPitch(true);

	m_editor->setFont(font);

	layout->addWidget(m_editor);
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

	m_editor->setPlainText(ts.readAll());
}
