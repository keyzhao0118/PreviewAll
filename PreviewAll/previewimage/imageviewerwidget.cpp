#include "imageviewerwidget.h"
#include "previewtitlebar.h"
#include <QLayout>

ImageViewerWidget::ImageViewerWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_imageViewPort(new ImageViewPortWidget(filePath, this))
{
	setWindowFlags(Qt::FramelessWindowHint);
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(new PreviewTitleBar(filePath, this));
	mainLayout->addWidget(m_imageViewPort);
}

ImageViewerWidget::~ImageViewerWidget()
{}



