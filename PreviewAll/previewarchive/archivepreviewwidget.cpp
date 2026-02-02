#include "archivepreviewwidget.h"
#include "archiveparser.h"
#include "archivetreewidget.h"
#include <QThread>
#include <QLineEdit>
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
	connect(m_archiveParser, &ArchiveParser::requestPassword, this, &ArchivePreviewWidget::showEncryptPage);
	connect(m_archiveParser, &ArchiveParser::parseFailed, this, &ArchivePreviewWidget::showErrorPage);
	connect(m_archiveParser, &ArchiveParser::parseSucceed, this, &ArchivePreviewWidget::showPreviewPage);

	connect(m_parserThread, &QThread::finished, m_parserThread, &QThread::deleteLater);
	connect(m_parserThread, &QThread::finished, m_archiveParser, &ArchiveParser::deleteLater);

	m_parserThread->start();
}

void ArchivePreviewWidget::showLoadingPage()
{
	if (!m_infoLab)
	{
		createInfoLab();
		m_stackedLayout->addWidget(m_infoLab);
	}

	m_infoLab->setText(tr("Loading archive."));
	m_stackedLayout->setCurrentWidget(m_infoLab);
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
	if (!m_encryptPage)
	{
		createEncryptPage();
		m_stackedLayout->addWidget(m_encryptPage);
	}

	m_stackedLayout->setCurrentWidget(m_encryptPage);
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

void ArchivePreviewWidget::createEncryptPage()
{
	m_encryptPage = new QWidget(this);

	QLineEdit* passwordEdit = new QLineEdit(m_encryptPage);
	passwordEdit->setPlaceholderText(tr("Enter password"));
	passwordEdit->setEchoMode(QLineEdit::Password);

	QPushButton* okBtn = new QPushButton(tr("OK"), m_encryptPage);
	connect(okBtn, &QPushButton::clicked, this, [this, passwordEdit]() {
		showLoadingPage();
		QString password = passwordEdit->text();
		m_archiveParser->setPassword(password);
	});

	QVBoxLayout* encryptLayout = new QVBoxLayout(m_encryptPage);
	encryptLayout->setContentsMargins(50, 50, 50, 50);
	encryptLayout->setSpacing(10);
	encryptLayout->addWidget(passwordEdit);
	encryptLayout->addWidget(okBtn);
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

	QVBoxLayout* previewPageLayout = new QVBoxLayout(m_previewPage);
	previewPageLayout->setContentsMargins(0, 0, 0, 0);
	previewPageLayout->setSpacing(0);
	previewPageLayout->addWidget(bottomWidget);
	previewPageLayout->addWidget(treeWidget);
}
