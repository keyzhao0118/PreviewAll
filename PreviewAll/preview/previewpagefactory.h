#pragma once

#include <QString>
#include <QStringList>

class PreviewPage;
class QWidget;

enum class PreviewFormat { Image, Archive, Markdown };

// One catalog drives both format dispatch and extensions in the tray UI.
class PreviewPageFactory
{
public:
	static PreviewPage* create(const QString& filePath, QWidget* parent);
	static const QStringList& extensions(PreviewFormat format);
};
