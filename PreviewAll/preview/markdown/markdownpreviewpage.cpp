#include "markdownpreviewpage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
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
#include <QVector>
#include <QVBoxLayout>

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

MarkdownPreviewPage::MarkdownPreviewPage(const QString& filePath, QWidget* parent)
	: PreviewPage(filePath, parent)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	m_renderedView = new QTextBrowser(this);
	layout->addWidget(m_renderedView);
	m_renderedView->setOpenLinks(false);
	m_renderedView->setFrameShape(QFrame::NoFrame);
}

void MarkdownPreviewPage::startPreview()
{
	emit loading();
	const QString filePath = this->filePath();
	const QString styleSheet = markdownStyleSheet(palette());
	const QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(filePath).absolutePath() + QDir::separator());

	struct LoadResult
	{
		std::shared_ptr<MarkdownDocumentPayload> document;
		LoadError error = LoadError::None;
	};
	if (!m_loadTask.start(this,
		[filePath, styleSheet, baseUrl](const std::atomic_bool& cancelled) {
		LoadResult result;
		QFileInfo fileInfo(filePath);
		QByteArray bytes;
		if (!fileInfo.isFile() || fileInfo.size() > MaxMarkdownFileSize)
		{
			result.error = LoadError::Unavailable;
		}
		else
		{
			QFile file(filePath);
			if (!file.open(QIODevice::ReadOnly))
			{
				result.error = LoadError::ReadFailed;
			}
			else
			{
				bytes = file.readAll();
				if (file.error() != QFileDevice::NoError || bytes.size() > MaxMarkdownFileSize)
					result.error = LoadError::ReadFailed;
			}
		}

		if (cancelled.load(std::memory_order_relaxed))
			return result;

		result.document = std::make_shared<MarkdownDocumentPayload>();
		if (result.error == LoadError::None)
		{
			result.document->document = new QTextDocument;
			result.document->document->setBaseUrl(baseUrl);
			result.document->document->setDefaultStyleSheet(styleSheet);
			result.document->document->setMarkdown(decodeMarkdown(bytes), QTextDocument::MarkdownDialectGitHub);
			constrainDocumentImages(result.document->document);
			wrapDocumentCodeBlocks(result.document->document);
			result.document->document->setDocumentMargin(20);
		}

		QCoreApplication* application = QCoreApplication::instance();
		if (application && result.document->document)
			result.document->document->moveToThread(application->thread());
		return result;
	}, [this](LoadResult result) {
		if (result.error != LoadError::None)
		{
			showLoadError(result.error);
			return;
		}
		result.document->document->setParent(m_renderedView);
		m_renderedView->setDocument(result.document->release());
		if (m_renderedView->document()->isEmpty())
			emit empty(tr("The Markdown file is empty."));
		else
			emit ready();
	}))
		emit failed(tr("Preview is busy. Please try again."));
}

void MarkdownPreviewPage::showLoadError(LoadError error)
{
	if (error == LoadError::Unavailable)
		emit failed(tr("Markdown file is unavailable or larger than 8 MiB."));
	else
		emit failed(tr("Failed to load Markdown file."));
}
