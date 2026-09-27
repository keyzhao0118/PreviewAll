#include "imagepreviewpage.h"

#include <QImageReader>
#include <QDebug>
#include <QVBoxLayout>
#include <QtMath>
#include <utility>

namespace
{
	constexpr int imageAllocationLimitMb = 512;
	constexpr qint64 maxDecodedPixels = 64LL * 1024 * 1024;
	constexpr int maxDecodedDimension = 16384;

	QSize constrainedDecodeSize(const QSize& sourceSize)
	{
		if (!sourceSize.isValid())
			return sourceSize;
		qreal scale = qMin(1.0, qreal(maxDecodedDimension) / qMax(sourceSize.width(), sourceSize.height()));
		const qreal pixels = qreal(sourceSize.width()) * sourceSize.height();
		if (pixels > maxDecodedPixels)
			scale = qMin(scale, qSqrt(qreal(maxDecodedPixels) / pixels));
		if (scale >= 1.0)
			return sourceSize;
		return QSize(qMax(1, qFloor(sourceSize.width() * scale)),
			qMax(1, qFloor(sourceSize.height() * scale)));
	}
}

ImagePreviewPage::ImagePreviewPage(const QString& filePath, QWidget* parent)
	: PreviewPage(filePath, parent)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	m_imageViewPort = new ImageViewPortWidget(this);
	layout->addWidget(m_imageViewPort);
}

void ImagePreviewPage::startPreview()
{
	emit loading();
	const QString path = filePath();
	QImageReader::setAllocationLimit(imageAllocationLimitMb);
	struct LoadResult { QImage image; QString error; };
	if (!m_loadTask.start(this, [path](const std::atomic_bool&) {
		LoadResult result;
		QImageReader reader(path);
		reader.setAutoTransform(true);
		const QSize sourceSize = reader.size();
		const QSize decodeSize = constrainedDecodeSize(sourceSize);
		if (decodeSize.isValid() && decodeSize != sourceSize)
			reader.setScaledSize(decodeSize);
		result.image = reader.read();
		if (result.image.isNull())
			result.error = reader.errorString();
		return result;
	}, [this](LoadResult result) {
		if (result.image.isNull())
		{
			qWarning() << "Failed to load image:" << filePath() << result.error;
			emit failed(tr("Failed to load image"));
			return;
		}
		m_imageViewPort->setImage(std::move(result.image));
		emit ready();
	}))
		emit failed(tr("Preview is busy. Please try again."));
}
