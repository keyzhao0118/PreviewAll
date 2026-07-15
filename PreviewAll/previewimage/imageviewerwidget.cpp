#include "imageviewerwidget.h"
#include <QLayout>

ImageViewerWidget::ImageViewerWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_imageViewPort(new ImageViewPortWidget(filePath, this))
	, m_statusBar(new ImageViewStatusBar(filePath, this))
{
	setWindowFlags(Qt::FramelessWindowHint);
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(m_statusBar);
	mainLayout->addWidget(m_imageViewPort);

	connect(m_statusBar, &ImageViewStatusBar::adaptiveScale, m_imageViewPort, &ImageViewPortWidget::onAdaptiveScale);
}

ImageViewerWidget::~ImageViewerWidget()
{}



