#pragma once

#include <QByteArray>
#include <QString>
#include "preview/common/previewpage.h"
#include "preview/common/previewtask.h"

class QTextBrowser;

class MarkdownPreviewPage : public PreviewPage
{
	Q_OBJECT

public:
	explicit MarkdownPreviewPage(const QString& filePath, QWidget* parent = nullptr);
	~MarkdownPreviewPage() override = default;
	void startPreview() override;

private:
	enum class LoadError
	{
		None,
		Unavailable,
		ReadFailed,
	};

	void showLoadError(LoadError error);

	PreviewTask m_loadTask;
	QTextBrowser* m_renderedView = nullptr;
};
