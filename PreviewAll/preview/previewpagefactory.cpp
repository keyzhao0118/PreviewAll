#include "previewpagefactory.h"
#include "archive/archivepreviewpage.h"
#include "image/imagepreviewpage.h"
#include "markdown/markdownpreviewpage.h"

#include <QFileInfo>

const QStringList& PreviewPageFactory::extensions(PreviewFormat format)
{
	static const QStringList images = {
		".png", ".jpg", ".jpeg", ".tif", ".tiff", ".bmp", ".webp", ".ico", ".svg", ".gif"
	};
	static const QStringList archives = { ".zip", ".rar", ".7z" };
	static const QStringList markdown = { ".md", ".markdown" };
	static const QStringList none;
	switch (format)
	{
	case PreviewFormat::Image: return images;
	case PreviewFormat::Archive: return archives;
	case PreviewFormat::Markdown: return markdown;
	}
	return none;
}

PreviewPage* PreviewPageFactory::create(const QString& filePath, QWidget* parent)
{
	const QString suffix = "." + QFileInfo(filePath).suffix();
	if (extensions(PreviewFormat::Image).contains(suffix, Qt::CaseInsensitive))
		return new ImagePreviewPage(filePath, parent);
	if (extensions(PreviewFormat::Archive).contains(suffix, Qt::CaseInsensitive))
		return new ArchivePreviewPage(filePath, parent);
	if (extensions(PreviewFormat::Markdown).contains(suffix, Qt::CaseInsensitive))
		return new MarkdownPreviewPage(filePath, parent);
	return nullptr;
}
