#include "imageviewportwidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QImageReader>
#include <QDebug>
#include <QtMath>
#include <QLayout>
#include <QMovie>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QThread>
#include <utility>

namespace
{
	const qreal s_minScaleFactor = 0.01;
	const qreal s_maxScaleFactor = 8.0;
	const qreal s_zoomStepPerNotch = 1.1;
	constexpr int s_imageAllocationLimitMb = 512;
	constexpr qint64 s_maxDecodedPixels = 64LL * 1024 * 1024;
	constexpr int s_maxDecodedDimension = 16384;

	QSize constrainedDecodeSize(const QSize& sourceSize)
	{
		if (!sourceSize.isValid())
			return sourceSize;

		qreal scale = qMin(
			1.0,
			qreal(s_maxDecodedDimension) / qMax(sourceSize.width(), sourceSize.height()));
		const qreal pixelCount = qreal(sourceSize.width()) * sourceSize.height();
		if (pixelCount > s_maxDecodedPixels)
			scale = qMin(scale, qSqrt(qreal(s_maxDecodedPixels) / pixelCount));

		if (scale >= 1.0)
			return sourceSize;

		return QSize(
			qMax(1, qFloor(sourceSize.width() * scale)),
			qMax(1, qFloor(sourceSize.height() * scale)));
	}
}

ImageViewPortWidget::ImageViewPortWidget(const QString& imagePath, QWidget *parent)
	: QOpenGLWidget(parent)
	, m_imagePath(imagePath)
{
	setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
	QImageReader::setAllocationLimit(s_imageAllocationLimitMb);

	m_zoomTimeLine.setDuration(140);
	m_zoomTimeLine.setUpdateInterval(1000 / 60);
	m_zoomTimeLine.setEasingCurve(QEasingCurve::Linear);
	connect(&m_zoomTimeLine, &QTimeLine::valueChanged, this, [this](qreal x) {
		qreal ratio = m_zoomStopScaleFactor / m_zoomStartScaleFactor;
		m_curScaleFactor = m_zoomStartScaleFactor * qPow(ratio, x);
		updateScaleFactor();
		updatePaintBasePos();
		updatePaintOffset();
		updateCursor();
		update();
	});
	connect(&m_zoomTimeLine, &QTimeLine::finished, this, [this]() {
		consumeAccumulateZoomSteps();
	});

	if (imagePath.endsWith(".gif", Qt::CaseInsensitive))
		loadGif();
	else
		loadImage();
}

ImageViewPortWidget::~ImageViewPortWidget()
{}


void ImageViewPortWidget::onAdaptiveScale()
{
	m_zoomTimeLine.stop();
	m_zoomStartScaleFactor = m_curScaleFactor;

	if (m_image.isNull())
		return;
	m_paintSize = m_image.size().scaled(size(), Qt::KeepAspectRatio);
	m_zoomStopScaleFactor = 1.0 * m_paintSize.width() / m_image.width();
	m_zoomTimeLine.start();
}

void ImageViewPortWidget::resizeEvent(QResizeEvent* event)
{
	QOpenGLWidget::resizeEvent(event);
	resizeToFit();
}

void ImageViewPortWidget::paintGL()
{
	if (auto* f = QOpenGLContext::currentContext()->functions())
	{
		f->glDisable(GL_SCISSOR_TEST);
		const QColor bg = QColor("#ffffff");
		f->glClearColor(bg.redF(), bg.greenF(), bg.blueF(), bg.alphaF());
		f->glClear(GL_COLOR_BUFFER_BIT);
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	if (m_image.isNull())
	{
		painter.drawText(rect(), Qt::AlignCenter, m_bIsLoading ? tr("Loading...") : tr("Loading failed"));
		return;
	}

	QPoint drawPos = m_paintBasePos + m_paintOffset;
	QRect targetRect(drawPos, m_paintSize);
	QRect viewportRect(0, 0, width(), height());
	QRect visibleRect = targetRect.intersected(viewportRect);
	if (!visibleRect.isEmpty())
	{
		QRect sourceRect(
			(visibleRect.left() - drawPos.x()) / m_curScaleFactor,
			(visibleRect.top() - drawPos.y()) / m_curScaleFactor,
			visibleRect.width() / m_curScaleFactor,
			visibleRect.height() / m_curScaleFactor
		);
		painter.drawImage(visibleRect, m_image, sourceRect);
	}
}

void ImageViewPortWidget::wheelEvent(QWheelEvent* event)
{
	if (m_image.isNull())
		return;

	int deltaY = event->angleDelta().y();
	if (deltaY == 0)
	{
		event->ignore();
		return;
	}

	int steps = deltaY / 120;
	enqueueZoomOperation(steps);

	event->accept();
}

void ImageViewPortWidget::mousePressEvent(QMouseEvent* event)
{
	activateWindow();

	if (event->button() == Qt::LeftButton && canDrag())
	{
		m_bDragging = true;
		m_lastMousePos = event->pos();
	}
	updateCursor();
	QOpenGLWidget::mousePressEvent(event);
}

void ImageViewPortWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_bDragging)
	{
		QPoint delta = event->pos() - m_lastMousePos;
		m_lastMousePos = event->pos();
		m_paintOffset += delta;
		updatePaintOffset();
		update();
	}
	QOpenGLWidget::mouseMoveEvent(event);
}

void ImageViewPortWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_bDragging)
	{
		m_bDragging = false;
	}
	updateCursor();
	QOpenGLWidget::mouseReleaseEvent(event);
}

void ImageViewPortWidget::loadImage()
{
	QPointer<ImageViewPortWidget> that(this);
	const QString imagePath = m_imagePath;
	QThread* loadThread = QThread::create([that, imagePath]() {
		if (!that)
			return;

		QImageReader reader(imagePath);
		reader.setAutoTransform(true);
		const QSize sourceSize = reader.size();
		const QSize decodeSize = constrainedDecodeSize(sourceSize);
		if (decodeSize.isValid() && decodeSize != sourceSize)
			reader.setScaledSize(decodeSize);

		QImage image = reader.read();
		const QString error = image.isNull() ? reader.errorString() : QString();
		QMetaObject::invokeMethod(that, [that, image = std::move(image), error]() mutable {
			if (!that)
				return;

			that->m_bIsLoading = false;
			that->m_image = std::move(image);
			if (that->m_image.isNull())
			{
				qWarning() << "Failed to load image:" << that->m_imagePath << error;
				that->update();
				return;
			}

			that->resizeToFit();
		}, Qt::QueuedConnection);
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void ImageViewPortWidget::loadGif()
{
	m_bLoadFirstGifFrame = true;
	auto* gifMovie = new QMovie(m_imagePath, QByteArray(), this);
	connect(gifMovie, &QMovie::frameChanged, this, [this, gifMovie]() {
		m_bIsLoading = false;
		m_image = gifMovie->currentImage();
		if (m_bLoadFirstGifFrame)
		{
			m_bLoadFirstGifFrame = false;
			resizeToFit();
			return;
		}

		updateScaleFactor();
		updatePaintBasePos();
		updatePaintOffset();
		updateCursor();
		update();
	});
	connect(gifMovie, &QMovie::error, this, [this, gifMovie](QImageReader::ImageReaderError) {
		m_bIsLoading = false;
		m_image = QImage();
		qWarning() << "Failed to load GIF:" << m_imagePath << gifMovie->lastErrorString();
		update();
	});
	gifMovie->start();
}

void ImageViewPortWidget::resizeToFit()
{
	if (m_image.isNull())
		return;

	m_paintSize = m_image.size().scaled(size(), Qt::KeepAspectRatio);
	m_curScaleFactor = 1.0 * m_paintSize.width() / m_image.width();
	updateScaleFactor();
	updatePaintBasePos();
	updatePaintOffset();
	updateCursor();
	update();
}

void ImageViewPortWidget::updateScaleFactor()
{
	m_curScaleFactor = std::clamp(
		m_curScaleFactor,
		s_minScaleFactor / devicePixelRatioF(),
		s_maxScaleFactor / devicePixelRatioF());

	if (m_image.isNull())
		return;

	m_paintSize = m_image.size() * m_curScaleFactor;
}

void ImageViewPortWidget::updatePaintBasePos()
{
	m_paintBasePos.setX((size().width() - m_paintSize.width()) / 2);
	m_paintBasePos.setY((size().height() - m_paintSize.height()) / 2);
}

void ImageViewPortWidget::updatePaintOffset()
{
	QPoint paintPos = m_paintBasePos + m_paintOffset;
	if (m_paintSize.width() <= width())
	{
		m_paintOffset.setX(0);
	}
	else
	{
		int maxOffsetX = (m_paintSize.width() - width()) / 2;
		m_paintOffset.setX(std::clamp(m_paintOffset.x(), -maxOffsetX, maxOffsetX));
	}

	if (m_paintSize.height() <= height())
	{
		m_paintOffset.setY(0);
	}
	else
	{
		int maxOffsetY = (m_paintSize.height() - height()) / 2;
		m_paintOffset.setY(std::clamp(m_paintOffset.y(), -maxOffsetY, maxOffsetY));
	}
}

void ImageViewPortWidget::updateCursor()
{
	if (m_paintSize.width() > width() || m_paintSize.height() > height())
	{
		if (m_bDragging)
			setCursor(Qt::ClosedHandCursor);
		else
			setCursor(Qt::OpenHandCursor);
	}
	else
		unsetCursor();
}

bool ImageViewPortWidget::canDrag()
{
	QPoint curPaintPos = m_paintBasePos + m_paintOffset;
	return curPaintPos.x() < 0 || curPaintPos.y() < 0
		|| curPaintPos.x() + m_paintSize.width() > width()
		|| curPaintPos.y() + m_paintSize.height() > height();
}

void ImageViewPortWidget::enqueueZoomOperation(int steps)
{
	if (steps == 0)
		return;

	m_accumulateZoomSteps += steps;
	if (m_zoomTimeLine.state() == QTimeLine::NotRunning)
		consumeAccumulateZoomSteps();
}

void ImageViewPortWidget::consumeAccumulateZoomSteps()
{
	if (m_accumulateZoomSteps == 0)
		return;

	m_zoomTimeLine.stop();
	m_zoomStartScaleFactor = m_curScaleFactor;
	m_zoomStopScaleFactor = m_curScaleFactor * qPow(s_zoomStepPerNotch, m_accumulateZoomSteps);
	m_accumulateZoomSteps = 0;
	m_zoomTimeLine.start();
}

