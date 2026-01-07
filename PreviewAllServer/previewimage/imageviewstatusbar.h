#pragma once

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QLayout>

class ImageViewStatusBar  : public QWidget
{
	Q_OBJECT

public:
	ImageViewStatusBar(const QString& imagePath, QWidget* parent);
	~ImageViewStatusBar();

signals:
	void scaleFactorChanged(qreal scaleFactor);
	void adaptiveScale();

public slots:
	void onScaleFactorChanged(qreal scaleFactor);

protected:
	virtual void resizeEvent(QResizeEvent* resizeEvent) override;

private:
	void addResolutionLab();
	void addAdaptiveBtn();
	void addScaleComboBox();
	void addZoomOutBtn();
	void addScaleSlider();
	void addZoomInBtn();

private slots:
	void handleComboBoxChanged(const QString& text);
	void handleSliderChanged(int value);
	void handleZoomOut();
	void handleZoomIn();

private:
	QString m_imagePath;

	QHBoxLayout* m_mainLayout = nullptr;
	QPushButton* m_adaptiveBtn = nullptr;
	QComboBox* m_scaleComboBox = nullptr;
	QSlider* m_scaleSlider = nullptr;
	QPushButton* m_zoomOutBtn = nullptr;
	QPushButton* m_zoomInBtn = nullptr;
	bool m_bIsInternalChange = false;
};

