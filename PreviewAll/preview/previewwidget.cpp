#include "previewwidget.h"
#include "common/previewcontentstack.h"
#include "common/previewtitlebar.h"
#include "previewpagefactory.h"
#include "common/previewpage.h"

#include <QVBoxLayout>

PreviewWidget::PreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(new PreviewTitleBar(filePath, this));
	m_content = new PreviewContentStack(this);
	layout->addWidget(m_content);

	m_page = PreviewPageFactory::create(filePath, m_content);
	if (!m_page)
	{
		m_content->showError(tr("This file type cannot be previewed."));
		return;
	}

	// Register the still-empty page before starting work so unfinished content
	// stays hidden behind the shared loading state.
	m_content->addPage(m_page);
	connect(m_page, &PreviewPage::loading, m_content, &PreviewContentStack::showLoading);
	connect(m_page, &PreviewPage::ready, this, [this] { m_content->showPage(m_page); });
	connect(m_page, &PreviewPage::empty, m_content, &PreviewContentStack::showEmpty);
	connect(m_page, &PreviewPage::failed, m_content, &PreviewContentStack::showError);
	m_page->startPreview();
}
