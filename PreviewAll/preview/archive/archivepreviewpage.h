#pragma once

#include "preview/common/previewtask.h"
#include "preview/common/previewpage.h"
#include <QSharedPointer>

class ArchiveParser;
class ArchiveTreeWidget;

class ArchivePreviewPage : public PreviewPage
{
	Q_OBJECT
public:
	explicit ArchivePreviewPage(const QString& filePath, QWidget* parent = nullptr);
	~ArchivePreviewPage() override = default;

	void startPreview() override;

private:
	void showPreviewPage();

	QSharedPointer<ArchiveParser> m_archiveParser;
	PreviewTask m_loadTask;
	ArchiveTreeWidget* m_tree = nullptr;
};
