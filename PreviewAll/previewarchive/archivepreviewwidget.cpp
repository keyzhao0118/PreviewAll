#include "archivepreviewwidget.h"
#include "archiveparser.h"
#include "archivetreewidget.h"
#include <QThread>
#include <QMovie>

ArchivePreviewWidget::ArchivePreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAutoFillBackground(true);
	QPalette pal = palette();
	pal.setColor(QPalette::Window, Qt::white);
	setPalette(pal);

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
		m_loadingLab = new QLabel(this);
		m_loadingLab->setAlignment(Qt::AlignCenter);
		auto* movie = new QMovie(":/gif/loading.gif");
		m_loadingLab->setMovie(movie);
		movie->start();
		m_stackedLayout->addWidget(m_loadingLab);
	}

	m_stackedLayout->setCurrentWidget(m_loadingLab);
}

void ArchivePreviewWidget::showErrorPage()
{
	if (!m_infoLab)
	{
		createInfoLab();
		m_stackedLayout->addWidget(m_infoLab);
	}

	m_infoLab->setText(tr("Failed to parse archive."));
	m_stackedLayout->setCurrentWidget(m_infoLab);
}

void ArchivePreviewWidget::showEncryptPage()
{
	if (!m_infoLab)
	{
		createInfoLab();
		m_stackedLayout->addWidget(m_infoLab);
	}

	m_infoLab->setText(tr("The archive is encrypted and cannot be previewed."));
	m_stackedLayout->setCurrentWidget(m_infoLab);
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

void ArchivePreviewWidget::createInfoLab()
{
	m_infoLab = new QLabel(this);
	m_infoLab->setWordWrap(true);
	m_infoLab->setAlignment(Qt::AlignCenter);
	m_infoLab->setStyleSheet(R"(
			QLabel
			{
				font-family: "Microsoft YaHei";
				font-size: 12pt;
				color: #808080;
			}
	)");
}
