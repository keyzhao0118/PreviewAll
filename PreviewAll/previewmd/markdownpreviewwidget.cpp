#include "markdownpreviewwidget.h"
#include "previewloadpool.h"
#include "previewtitlebar.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QPointer>
#include <QRunnable>
#include <QStringConverter>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QTextLength>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>

#include <memory>
#include <utility>

namespace
{
	constexpr qint64 MaxMarkdownFileSize = 8 * 1024 * 1024;

	struct MarkdownDocumentPayload
	{
		~MarkdownDocumentPayload()
		{
			if (!document)
				return;

			if (document->thread() == QThread::currentThread())
				delete document;
			else
				document->deleteLater();
		}

		QTextDocument* release()
		{
			return std::exchange(document, nullptr);
		}

		QTextDocument* document = nullptr;
	};

	QString decodeMarkdown(const QByteArray& bytes)
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

	QString markdownStyleSheet(const QPalette& palette)
	{
		return QString(
			"body { color: %1; background-color: %2; font-family: 'Segoe UI'; }"
			"pre { background-color: %3; padding: 8px; white-space: pre-wrap; }"
			"code { font-family: 'Consolas'; background-color: %3; }"
			"blockquote { color: %4; border-left: 3px solid %4; margin-left: 0; padding-left: 12px; }"
			"table { border-collapse: collapse; } th, td { border: 1px solid %4; padding: 4px 8px; }"
			"img { max-width: 100%; }")
			.arg(palette.color(QPalette::Text).name(),
				palette.color(QPalette::Base).name(),
				palette.color(QPalette::AlternateBase).name(),
				palette.color(QPalette::PlaceholderText).name());
	}

	void constrainDocumentImages(QTextDocument* document)
	{
		struct ImageFragment
		{
			int position;
			int length;
			QTextImageFormat format;
		};
		QVector<ImageFragment> images;
		for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
		{
			for (auto it = block.begin(); !it.atEnd(); ++it)
			{
				const QTextFragment fragment = it.fragment();
				if (!fragment.isValid() || !fragment.charFormat().isImageFormat())
					continue;

				QTextImageFormat imageFormat = fragment.charFormat().toImageFormat();
				imageFormat.setMaximumWidth(QTextLength(QTextLength::PercentageLength, 100));
				images.append({ fragment.position(), fragment.length(), imageFormat });
			}
		}
		for (const ImageFragment& image : images)
		{
			QTextCursor cursor(document);
			cursor.setPosition(image.position);
			cursor.setPosition(image.position + image.length, QTextCursor::KeepAnchor);
			cursor.setCharFormat(image.format);
		}
	}

	void wrapDocumentCodeBlocks(QTextDocument* document)
	{
		for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
		{
			QTextBlockFormat format = block.blockFormat();
			if (!format.nonBreakableLines())
				continue;
			format.setNonBreakableLines(false);
			QTextCursor cursor(block);
			cursor.setBlockFormat(format);
		}
	}
}

MarkdownPreviewWidget::MarkdownPreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
	, m_loadCancelled(std::make_shared<std::atomic_bool>(false))
{
	setWindowFlags(Qt::FramelessWindowHint);

	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(new PreviewTitleBar(m_filePath, this));

	m_renderedView = new QTextBrowser(this);
	m_renderedView->setOpenLinks(false);
	m_renderedView->setFrameShape(QFrame::NoFrame);
	m_renderedView->setPlainText(tr("Loading..."));
	mainLayout->addWidget(m_renderedView);

	loadFile();
}

MarkdownPreviewWidget::~MarkdownPreviewWidget()
{
	m_loadCancelled->store(true, std::memory_order_relaxed);
}

void MarkdownPreviewWidget::loadFile()
{
	QPointer<MarkdownPreviewWidget> receiver(this);
	const auto cancelled = m_loadCancelled;
	const QString filePath = m_filePath;
	const QString styleSheet = markdownStyleSheet(palette());
	const QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + QDir::separator());

	previewLoadPool().start(QRunnable::create([receiver, cancelled, filePath, styleSheet, baseUrl]() {
		if (cancelled->load(std::memory_order_relaxed))
			return;

		QFileInfo fileInfo(filePath);
		LoadError error = LoadError::None;
		QByteArray bytes;
		if (!fileInfo.isFile() || fileInfo.size() > MaxMarkdownFileSize)
		{
			error = LoadError::Unavailable;
		}
		else
		{
			QFile file(filePath);
			if (!file.open(QIODevice::ReadOnly))
			{
				error = LoadError::ReadFailed;
			}
			else
			{
				bytes = file.readAll();
				if (file.error() != QFileDevice::NoError || bytes.size() > MaxMarkdownFileSize)
					error = LoadError::ReadFailed;
			}
		}

		if (cancelled->load(std::memory_order_relaxed))
			return;

		auto document = std::make_shared<MarkdownDocumentPayload>();
		if (error == LoadError::None)
		{
			document->document = new QTextDocument;
			document->document->setBaseUrl(baseUrl);
			document->document->setDefaultStyleSheet(styleSheet);
			document->document->setMarkdown(decodeMarkdown(bytes), QTextDocument::MarkdownDialectGitHub);
			constrainDocumentImages(document->document);
			wrapDocumentCodeBlocks(document->document);
			document->document->setDocumentMargin(20);
		}

		QCoreApplication* application = QCoreApplication::instance();
		if (!application)
			return;

		if (document->document)
			document->document->moveToThread(application->thread());

		QMetaObject::invokeMethod(application,
			[receiver, cancelled, document, error]() {
				if (!receiver || cancelled->load(std::memory_order_relaxed))
					return;

				if (error != LoadError::None)
				{
					receiver->showLoadError(error);
					return;
				}

				document->document->setParent(receiver->m_renderedView);
				receiver->m_renderedView->setDocument(document->release());
			},
			Qt::QueuedConnection);
	}));
}

void MarkdownPreviewWidget::showLoadError(LoadError error)
{
	if (error == LoadError::Unavailable)
		m_renderedView->setPlainText(tr("Markdown file is unavailable or larger than 8 MiB."));
	else
		m_renderedView->setPlainText(tr("Failed to load Markdown file."));
}
