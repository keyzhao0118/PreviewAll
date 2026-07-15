#pragma once

#include <QWidget>

class ImageViewStatusBar : public QWidget
{
	Q_OBJECT

public:
	explicit ImageViewStatusBar(const QString& imagePath, QWidget* parent);

signals:
	void adaptiveScale();
};