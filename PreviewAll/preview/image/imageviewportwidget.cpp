#include "imageviewportwidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QDebug>
#include <QtMath>
#include <QLayout>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <utility>

namespace
{
	const qreal s_minScaleFactor = 0.01;
	const qreal s_maxScaleFactor = 8.0;
	const qreal s_zoomStepPerNotch = 1.1;
}

ImageViewPortWidget::ImageViewPortWidget(QWidget *parent)
	: QOpenGLWidget(parent)
{
	setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

void ImageViewPortWidget::setImage(QImage image)
{
	m_image = std::move(image);
	resizeToFit();
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
		return;

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

	const qreal steps = qreal(deltaY) / 120.0;
	m_curScaleFactor *= qPow(s_zoomStepPerNotch, steps);
	updateScaleFactor();
	updatePaintBasePos();
	updatePaintOffset();
	updateCursor();
	update();

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

