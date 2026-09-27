#pragma once

#include "preview/common/previewpage.h"
#include "preview/common/previewtask.h"
#include "imageviewportwidget.h"

class ImagePreviewPage : public PreviewPage
{
	Q_OBJECT
public:
	explicit ImagePreviewPage(const QString& filePath, QWidget* parent = nullptr);
	~ImagePreviewPage() override = default;
	void startPreview() override;

private:
	PreviewTask m_loadTask;
	ImageViewPortWidget* m_imageViewPort = nullptr;
};
