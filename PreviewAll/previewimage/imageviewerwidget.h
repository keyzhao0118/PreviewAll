#pragma once

#include <QWidget>
#include "imageviewportwidget.h"

class ImageViewerWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ImageViewerWidget(const QString& filePath, QWidget* parent = nullptr);
	~ImageViewerWidget();

private:
	ImageViewPortWidget* m_imageViewPort = nullptr;
};

