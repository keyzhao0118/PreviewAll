#include "imageviewportwidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QImageReader>
#include <QtMath>
#include <QLayout>
#include <QMovie>
#include <QBuffer>
#include <QFile>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QThread>

namespace
{
	const qreal s_minScaleFactor = 0.01;
	const qreal s_maxScaleFactor = 8.0;
	const qreal s_zoomStepPerNotch = 1.1;
}

ImageViewPortWidget::ImageViewPortWidget(const QString& imagePath, QWidget *parent)
	: QOpenGLWidget(parent)
	, m_imagePath(imagePath)
{
	setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

	//缩放时的过渡动画
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
		emit scaleFactorChanged(m_curScaleFactor);
		consumeAccumulateZoomSteps();
	});

	if (imagePath.endsWith(".gif", Qt::CaseInsensitive))
		loadGifFramePixmap();
	else
		loadOriginPixmap();
}

ImageViewPortWidget::~ImageViewPortWidget()
{}

void ImageViewPortWidget::onScaleFactorChanged(qreal scaleFactor)
{
	m_zoomTimeLine.stop();
	m_zoomStartScaleFactor = m_curScaleFactor;
	m_zoomStopScaleFactor = scaleFactor;
	m_zoomTimeLine.start();
}

void ImageViewPortWidget::onAdaptiveScale()
{
	m_zoomTimeLine.stop();
	m_zoomStartScaleFactor = m_curScaleFactor;

	if (m_originPixmap.isNull())
		return;
	m_paintSize = m_originPixmap.size().scaled(size(), Qt::KeepAspectRatio);
	m_zoomStopScaleFactor = 1.0 * m_paintSize.width() / m_originPixmap.width();
	m_zoomTimeLine.start();
}

void ImageViewPortWidget::resizeEvent(QResizeEvent* event)
{
	QOpenGLWidget::resizeEvent(event);
	resizeToFit();
}

void ImageViewPortWidget::paintGL()
{
	//清屏（避免上一帧内容残留）
	if (auto* f = QOpenGLContext::currentContext()->functions())
	{
		f->glDisable(GL_SCISSOR_TEST); // 防止部分区域剪裁导致清屏不完整
		const QColor bg = palette().window().color();
		f->glClearColor(bg.redF(), bg.greenF(), bg.blueF(), bg.alphaF());
		f->glClear(GL_COLOR_BUFFER_BIT);
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	if (m_originPixmap.isNull())
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
		painter.drawPixmap(visibleRect, m_originPixmap, sourceRect);
	}
}

void ImageViewPortWidget::wheelEvent(QWheelEvent* event)
{
	if (m_originPixmap.isNull())
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

void ImageViewPortWidget::loadOriginPixmap()
{
	QPointer<ImageViewPortWidget> that(this);
	QThread* loadThread = QThread::create([that]() {
		if (!that)
			return;

		QImageReader reader(that->m_imagePath);
		if (reader.canRead())
		{
			QImage originImage = reader.read();
			QPixmap originPixmap = QPixmap::fromImage(originImage);

			// 切回 UI 线程：在这里才刷新视图
			QMetaObject::invokeMethod(that, [that, originPixmap]() {
				if (that)
				{
					that->m_originPixmap = originPixmap;
					that->resizeToFit();
				}
			}, Qt::QueuedConnection);
		}

		that->m_bIsLoading = false;
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void ImageViewPortWidget::loadGifFramePixmap()
{
	QPointer<ImageViewPortWidget> that(this);

	QThread* loadThread = QThread::create([that]() {
		if (!that)
			return;

		QFile gifFile(that->m_imagePath);
		if (gifFile.open(QIODevice::ReadOnly))
		{
			QByteArray gifData = gifFile.readAll();

			// 切回 UI 线程：在这里才刷新视图
			QMetaObject::invokeMethod(that, [that, gifData]() {
				if (!that)
					return;

				QBuffer* gifBuffer = new QBuffer(that);
				gifBuffer->setData(gifData);
				gifBuffer->open(QIODevice::ReadOnly);
				QMovie* gifMovie = new QMovie(gifBuffer, QByteArray(), that);
				if (!gifMovie->isValid())
					return;

				connect(gifMovie, &QMovie::frameChanged, that, [that, gifMovie]() {
					if (!that)
						return;

					that->m_originPixmap = gifMovie->currentPixmap();
					if (that->m_bLoadFirstGifFrame)
					{
						that->m_bLoadFirstGifFrame = false;
						that->resizeToFit();
						return;
					}
					that->updateScaleFactor();
					that->updatePaintBasePos();
					that->updatePaintOffset();
					that->updateCursor();
					that->update();
				});

				gifMovie->start();
			}, Qt::QueuedConnection);
		}

		that->m_bIsLoading = false;
	});

	connect(loadThread, &QThread::finished, loadThread, &QObject::deleteLater);
	loadThread->start();
}

void ImageViewPortWidget::resizeToFit()
{
	if (m_originPixmap.isNull())
		return;

	m_paintSize = m_originPixmap.size().scaled(size(), Qt::KeepAspectRatio);
	m_curScaleFactor = 1.0 * m_paintSize.width() / m_originPixmap.width();
	updateScaleFactor();
	updatePaintBasePos();
	updatePaintOffset();
	updateCursor();
	update();
	emit scaleFactorChanged(m_curScaleFactor);
}

void ImageViewPortWidget::updateScaleFactor()
{
	m_curScaleFactor = std::clamp(
		m_curScaleFactor,
		s_minScaleFactor / devicePixelRatioF(),
		s_maxScaleFactor / devicePixelRatioF());

	if (m_originPixmap.isNull())
		return;

	m_paintSize = m_originPixmap.size() * m_curScaleFactor;
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

