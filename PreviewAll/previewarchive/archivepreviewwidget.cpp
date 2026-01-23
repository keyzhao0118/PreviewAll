#include "archivepreviewwidget.h"
#include "archiveparser.h"
#include "archivetreewidget.h"
#include <QThread>
#include <QMovie>
#include <QPushButton>

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
	if (!m_previewPage)
	{
		craetePreviewPage();
		m_stackedLayout->addWidget(m_previewPage);
	}

	m_stackedLayout->setCurrentWidget(m_previewPage);
}

void ArchivePreviewWidget::createInfoLab()
{
	m_infoLab = new QLabel(this);
	m_infoLab->setWordWrap(true);
	m_infoLab->setAlignment(Qt::AlignCenter);
	QFont infoFont("Microsoft YaHei");
	infoFont.setPointSize(9);
	m_infoLab->setFont(infoFont);
}

void ArchivePreviewWidget::craetePreviewPage()
{
	m_previewPage = new QWidget(this);

	ArchiveTreeWidget* treeWidget = new ArchiveTreeWidget(this);
	const ArchiveTreeNode* rootNode = m_archiveParser->getRootNode();
	treeWidget->refresh(rootNode);

	QLabel* archiveIconLab = new QLabel(this);
	QPixmap archivePix(":/png/archive.png");
	archivePix = archivePix.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	archiveIconLab->setPixmap(archivePix);

	QLabel* statusLab = new QLabel(
		tr("File: %1, Folder: %2").arg(m_archiveParser->getFileCount()).arg(m_archiveParser->getFolderCount()), this);
	
	QPushButton* extractBtn = new QPushButton(this);
	extractBtn->setIcon(QIcon(":/svg/extract.svg"));
	extractBtn->setToolTip(tr("Extract"));

	QWidget* bottomWidget = new QWidget(this);
	QFont statusBarFont("Microsoft YaHei");
	statusBarFont.setPointSize(9);
	bottomWidget->setFont(statusBarFont);
	bottomWidget->setFixedHeight(35);
	QHBoxLayout* bottomLayout = new QHBoxLayout(bottomWidget);
	bottomLayout->setContentsMargins(5, 0, 5, 0);
	bottomLayout->setSpacing(5);

	bottomLayout->addWidget(archiveIconLab);
	bottomLayout->addWidget(statusLab);
	bottomLayout->addStretch();
	bottomLayout->addWidget(extractBtn);

	QVBoxLayout* previewPageLayout = new QVBoxLayout(m_previewPage);
	previewPageLayout->setContentsMargins(0, 0, 0, 0);
	previewPageLayout->setSpacing(0);
	previewPageLayout->addWidget(bottomWidget);
	previewPageLayout->addWidget(treeWidget);
}
