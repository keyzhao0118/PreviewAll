#include "markdownpreviewwidget.h"
#include "previewtoolbarstyle.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringConverter>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
	constexpr qint64 MaxMarkdownFileSize = 8 * 1024 * 1024;
}

MarkdownPreviewWidget::MarkdownPreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
{
	setWindowFlags(Qt::FramelessWindowHint);
	initUi();
	loadFile();
}

void MarkdownPreviewWidget::initUi()
{
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	auto* toolBar = new QWidget(this);
	auto* toolBarLayout = new QHBoxLayout(toolBar);
	PreviewToolbarStyle::apply(toolBar, toolBarLayout);

	auto* typeIcon = new QLabel(toolBar);
	typeIcon->setFixedSize(PreviewToolbarStyle::ContentIconSize, PreviewToolbarStyle::ContentIconSize);
	typeIcon->setPixmap(QIcon(":/svg/code.svg").pixmap(
		PreviewToolbarStyle::ContentIconSize, PreviewToolbarStyle::ContentIconSize));

	auto* fileLabel = new QLabel(QFileInfo(m_filePath).fileName(), toolBar);
	fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	m_sourceButton = new QPushButton(toolBar);
	m_sourceButton->setIcon(QIcon(":/svg/code.svg"));
	m_sourceButton->setToolTip(tr("Show source"));
	m_sourceButton->setCheckable(true);
	PreviewToolbarStyle::applyButton(m_sourceButton);

	toolBarLayout->addWidget(typeIcon);
	toolBarLayout->addWidget(fileLabel);
	toolBarLayout->addStretch();
	toolBarLayout->addWidget(m_sourceButton);
	mainLayout->addWidget(toolBar);

	m_viewStack = new QStackedWidget(this);
	m_renderedView = new QTextBrowser(m_viewStack);
	m_renderedView->setOpenLinks(false);
	m_renderedView->setFrameShape(QFrame::NoFrame);

	m_sourceView = new QPlainTextEdit(m_viewStack);
	m_sourceView->setReadOnly(true);
	m_sourceView->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_sourceView->setFrameShape(QFrame::NoFrame);
	m_sourceView->setFont(QFont("Consolas", 10));

	m_viewStack->addWidget(m_renderedView);
	m_viewStack->addWidget(m_sourceView);
	mainLayout->addWidget(m_viewStack);

	connect(m_sourceButton, &QPushButton::toggled, this, &MarkdownPreviewWidget::setSourceMode);
}

void MarkdownPreviewWidget::loadFile()
{
	QFileInfo fileInfo(m_filePath);
	if (!fileInfo.isFile() || fileInfo.size() > MaxMarkdownFileSize)
	{
		m_renderedView->setPlainText(tr("Markdown file is unavailable or larger than 8 MiB."));
		m_sourceButton->setEnabled(false);
		return;
	}

	QFile file(m_filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		m_renderedView->setPlainText(tr("Failed to load Markdown file."));
		m_sourceButton->setEnabled(false);
		return;
	}

	m_markdown = decodeText(file.readAll());
	m_sourceView->setPlainText(m_markdown);

	const QString basePath = fileInfo.absolutePath() + QDir::separator();
	m_renderedView->document()->setBaseUrl(QUrl::fromLocalFile(basePath));
	const QPalette currentPalette = palette();
	m_renderedView->document()->setDefaultStyleSheet(QString(
		"body { color: %1; background-color: %2; font-family: 'Segoe UI'; }"
		"pre { background-color: %3; padding: 8px; white-space: pre-wrap; }"
		"code { font-family: 'Consolas'; background-color: %3; }"
		"blockquote { color: %4; border-left: 3px solid %4; margin-left: 0; padding-left: 12px; }"
		"table { border-collapse: collapse; } th, td { border: 1px solid %4; padding: 4px 8px; }"
		"img { max-width: 100%; }")
		.arg(currentPalette.color(QPalette::Text).name(),
			currentPalette.color(QPalette::Base).name(),
			currentPalette.color(QPalette::AlternateBase).name(),
			currentPalette.color(QPalette::PlaceholderText).name()));
	m_renderedView->document()->setMarkdown(m_markdown, QTextDocument::MarkdownDialectGitHub);

	QTextFrameFormat frameFormat = m_renderedView->document()->rootFrame()->frameFormat();
	frameFormat.setMargin(20);
	m_renderedView->document()->rootFrame()->setFrameFormat(frameFormat);
}

void MarkdownPreviewWidget::setSourceMode(bool sourceMode)
{
	m_viewStack->setCurrentWidget(sourceMode ? static_cast<QWidget*>(m_sourceView) : static_cast<QWidget*>(m_renderedView));
	m_sourceButton->setToolTip(sourceMode ? tr("Show rendered Markdown") : tr("Show source"));
}

QString MarkdownPreviewWidget::decodeText(const QByteArray& bytes) const
{
	if (bytes.startsWith("\xEF\xBB\xBF"))
		return QString::fromUtf8(bytes.sliced(3));

	if (bytes.startsWith("\xFF\xFE"))
	{
		QStringDecoder decoder(QStringConverter::Utf16LE);
		return decoder(bytes.sliced(2));
	}

	if (bytes.startsWith("\xFE\xFF"))
	{
		QStringDecoder decoder(QStringConverter::Utf16BE);
		return decoder(bytes.sliced(2));
	}

	QStringDecoder utf8Decoder(QStringConverter::Utf8);
	const QString utf8Text = utf8Decoder(bytes);
	return utf8Decoder.hasError() ? QString::fromLocal8Bit(bytes) : utf8Text;
}