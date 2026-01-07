#pragma once

#include <QOpenGLWidget>
#include <QTimeLine>

class ImageViewPortWidget  : public QOpenGLWidget
{
	Q_OBJECT

public:
	ImageViewPortWidget(const QString& imagePath, QWidget *parent);
	~ImageViewPortWidget();

signals:
	void scaleFactorChanged(qreal scaleFactor);

public slots:
	void onScaleFactorChanged(qreal scaleFactor);
	void onAdaptiveScale();

protected:
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void paintGL() override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void loadOriginPixmap();
	void loadGifFramePixmap();
	void resizeToFit();
	void updateScaleFactor();
	void updatePaintBasePos();
	void updatePaintOffset();
	void updateCursor();
	bool canDrag();

	void enqueueZoomOperation(int steps);
	void consumeAccumulateZoomSteps();

private:
	QString m_imagePath;

	QPixmap m_originPixmap;

	QSize m_paintSize;
	QPoint m_paintBasePos;
	QPoint m_paintOffset;

	qreal m_curScaleFactor = 1.0;

	bool m_bDragging = false;
	QPoint m_lastMousePos;

	qreal m_zoomStartScaleFactor = 1.0;
	qreal m_zoomStopScaleFactor = 1.0;
	int m_accumulateZoomSteps = 0;
	QTimeLine m_zoomTimeLine;

	bool m_bLoadFirstGifFrame = false;
	bool m_bIsLoading = true;
};

