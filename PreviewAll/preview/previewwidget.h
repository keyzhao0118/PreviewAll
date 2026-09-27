#pragma once

#include <QString>
#include <QWidget>

class PreviewContentStack;
class PreviewPage;

// The only native window embedded in the Explorer preview host.
class PreviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PreviewWidget(const QString& filePath, QWidget* parent = nullptr);
	~PreviewWidget() override = default;
	void cancelPreview();

private:
	PreviewContentStack* m_content = nullptr;
	PreviewPage* m_page = nullptr;
};
