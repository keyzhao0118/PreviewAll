#pragma once

#include <QImage>
#include <QOpenGLWidget>

class ImageViewPortWidget  : public QOpenGLWidget
{
	Q_OBJECT

public:
	explicit ImageViewPortWidget(QWidget *parent);
	~ImageViewPortWidget() override = default;
	void setImage(QImage image);

protected:
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void paintGL() override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void resizeToFit();
	void updateScaleFactor();
	void updatePaintBasePos();
	void updatePaintOffset();
	void updateCursor();
	bool canDrag();

private:
	QImage m_image;

	QSize m_paintSize;
	QPoint m_paintBasePos;
	QPoint m_paintOffset;

	qreal m_curScaleFactor = 1.0;

	bool m_bDragging = false;
	QPoint m_lastMousePos;

};

