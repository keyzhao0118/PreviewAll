#pragma once

#include <QImage>
#include <QOpenGLWidget>

#include <atomic>
#include <memory>

class ImageViewPortWidget  : public QOpenGLWidget
{
	Q_OBJECT

public:
	ImageViewPortWidget(const QString& imagePath, QWidget *parent);
	~ImageViewPortWidget();

protected:
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void paintGL() override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void loadImage();
	void resizeToFit();
	void updateScaleFactor();
	void updatePaintBasePos();
	void updatePaintOffset();
	void updateCursor();
	bool canDrag();

private:
	QString m_imagePath;
	std::shared_ptr<std::atomic_bool> m_loadCancelled;

	QImage m_image;

	QSize m_paintSize;
	QPoint m_paintBasePos;
	QPoint m_paintOffset;

	qreal m_curScaleFactor = 1.0;

	bool m_bDragging = false;
	QPoint m_lastMousePos;

	bool m_bIsLoading = true;
};

