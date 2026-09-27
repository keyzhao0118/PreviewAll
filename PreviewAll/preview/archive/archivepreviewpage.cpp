#include "archivepreviewpage.h"
#include "archiveparser.h"
#include "archivetreewidget.h"
#include "archiveparsepool.h"
#include <QVBoxLayout>

ArchivePreviewPage::ArchivePreviewPage(const QString& filePath, QWidget* parent)
	: PreviewPage(filePath, parent)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
}

void ArchivePreviewPage::startPreview()
{
	m_loadTask.cancel();
	emit loading();
	delete m_tree;
	m_tree = nullptr;
	m_archiveParser = QSharedPointer<ArchiveParser>::create(filePath());
	const QSharedPointer<ArchiveParser> parser = m_archiveParser;
	m_loadTask.start(previewArchivePool(), this,
		[parser](const std::atomic_bool&) { return parser->parseArchive(); },
		[this](ArchiveParser::ParseResult result) {
			switch (result)
			{
			case ArchiveParser::ParseResult::Succeeded:
				if (m_archiveParser->getFileCount() == 0 && m_archiveParser->getFolderCount() == 0)
					emit empty(tr("The archive is empty."));
				else
					showPreviewPage();
				break;
			case ArchiveParser::ParseResult::HeaderEncrypted:
				emit failed(tr("Cannot preview an archive with encrypted headers."));
				break;
			case ArchiveParser::ParseResult::Failed:
				emit failed(tr("Failed to load archive"));
				break;
			case ArchiveParser::ParseResult::Cancelled:
				break;
			}
		},
		[parser] { parser->stopParse(); });
}

void ArchivePreviewPage::showPreviewPage()
{
	m_tree = new ArchiveTreeWidget(this);
	layout()->addWidget(m_tree);
	m_tree->refresh(m_archiveParser->getRootNode());
	emit ready();
}
