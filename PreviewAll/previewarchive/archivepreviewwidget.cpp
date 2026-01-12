#include "archivepreviewwidget.h"
#include "archiveparser.h"
#include "archivetreewidget.h"
#include <QThread>

ArchivePreviewWidget::ArchivePreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
{
	setWindowFlags(Qt::FramelessWindowHint);
	m_stackedLayout = new QStackedLayout(this);
	startParseArchive();
}

ArchivePreviewWidget::~ArchivePreviewWidget()
{
	m_archiveParser->stopParse();
	m_parserThread->quit();
}

void ArchivePreviewWidget::startParseArchive()
{
	showLoadingPage();

	m_parserThread = new QThread;
	m_archiveParser = new ArchiveParser(m_filePath);
	m_archiveParser->moveToThread(m_parserThread);

	connect(m_parserThread, &QThread::started, m_archiveParser, &ArchiveParser::parseArchive);
	connect(m_archiveParser, &ArchiveParser::encryptArchive, this, &ArchivePreviewWidget::showEncryptPage);
	connect(m_archiveParser, &ArchiveParser::parseFailed, this, &ArchivePreviewWidget::showErrorPage);
	connect(m_archiveParser, &ArchiveParser::parseSucceed, this, &ArchivePreviewWidget::showPreviewPage);

	connect(m_parserThread, &QThread::finished, m_parserThread, &QThread::deleteLater);
	connect(m_parserThread, &QThread::finished, m_archiveParser, &ArchiveParser::deleteLater);

	m_parserThread->start();
}

void ArchivePreviewWidget::showLoadingPage()
{
	if (!m_loadingLab)
	{
		m_loadingLab = new QLabel(tr("Loading..."), this);
		m_loadingLab->setAlignment(Qt::AlignCenter);
		m_stackedLayout->addWidget(m_loadingLab);
	}

	m_stackedLayout->setCurrentWidget(m_loadingLab);
}

void ArchivePreviewWidget::showErrorPage()
{
	if (!m_errorLab)
	{
		m_errorLab = new QLabel(tr("Failed to parse archive."), this);
		m_errorLab->setAlignment(Qt::AlignCenter);
		m_stackedLayout->addWidget(m_errorLab);
	}

	m_stackedLayout->setCurrentWidget(m_errorLab);
}

void ArchivePreviewWidget::showEncryptPage()
{
	if (!m_encryptLab)
	{
		m_encryptLab = new QLabel(tr("The archive is encrypted."), this);
		m_encryptLab->setAlignment(Qt::AlignCenter);
		m_stackedLayout->addWidget(m_encryptLab);
	}

	m_stackedLayout->setCurrentWidget(m_encryptLab);
}

void ArchivePreviewWidget::showPreviewPage()
{
	if (!m_treeWidget)
	{
		m_treeWidget = new ArchiveTreeWidget(this);
		const ArchiveTreeNode* rootNode = m_archiveParser->getRootNode();
		m_treeWidget->refresh(rootNode);
		m_stackedLayout->addWidget(m_treeWidget);
	}

	m_stackedLayout->setCurrentWidget(m_treeWidget);
}
