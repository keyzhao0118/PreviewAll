#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class PreviewTitleBar : public QWidget
{
	Q_OBJECT

public:
	explicit PreviewTitleBar(const QString& filePath, QWidget* parent = nullptr);

private:
	void openWithDefaultApplication();

	QString m_filePath;
	QPushButton* m_openButton = nullptr;
};
