#include "archivepreviewwidget.h"
#include "archiveparser.h"
#include "archivetreewidget.h"
#include <QThread>
#include <QLineEdit>
#include <QPushButton>
#include <QMovie>

ArchivePreviewWidget::ArchivePreviewWidget(const QString& filePath, QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
{
	setWindowFlags(Qt::FramelessWindowHint);
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);
	initStatusBar();
	m_stackedLayout = new QStackedLayout();
	m_mainLayout->addLayout(m_stackedLayout);

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
	if (!m_loadingPage)
	{
		createLoadingPage();
		m_stackedLayout->addWidget(m_loadingPage);
	}

	m_stackedLayout->setCurrentWidget(m_loadingPage);
	m_statusLab->setText(tr("Loading"));
}

void ArchivePreviewWidget::showErrorPage()
{
	if (!m_errorPage)
	{
		createErrorPage();
		m_stackedLayout->addWidget(m_errorPage);
	}

	m_stackedLayout->setCurrentWidget(m_errorPage);
	m_statusLab->setText(tr("Error"));
}

void ArchivePreviewWidget::showEncryptPage()
{
	if (!m_encryptPage)
	{
		createEncryptPage();
		m_stackedLayout->addWidget(m_encryptPage);
	}

	m_stackedLayout->setCurrentWidget(m_encryptPage);
	m_statusLab->setText(tr("Enter password"));
}

void ArchivePreviewWidget::showPreviewPage()
{
	if (!m_previewPage)
	{
		createPreviewPage();
		m_stackedLayout->addWidget(m_previewPage);
	}

	m_stackedLayout->setCurrentWidget(m_previewPage);
	m_statusLab->setText(tr("File: %1, Folder: %2").arg(m_archiveParser->getFileCount()).arg(m_archiveParser->getFolderCount()));
}

void ArchivePreviewWidget::initStatusBar()
{
	QLabel* archiveIconLab = new QLabel(this);
	QPixmap archivePix(":/png/archive.png");
	archivePix = archivePix.scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	archiveIconLab->setPixmap(archivePix);

	m_statusLab = new QLabel(this);

	QWidget* statusBar = new QWidget(this);
	QFont statusBarFont("Microsoft YaHei");
	statusBarFont.setPointSize(9);
	statusBar->setFont(statusBarFont);
	statusBar->setFixedHeight(50);
	QHBoxLayout* statusBarLayout = new QHBoxLayout(statusBar);
	statusBarLayout->setContentsMargins(10, 0, 10, 0);
	statusBarLayout->setSpacing(10);

	statusBarLayout->addWidget(archiveIconLab);
	statusBarLayout->addWidget(m_statusLab);
	statusBarLayout->addStretch();

	m_mainLayout->addWidget(statusBar);
}

void ArchivePreviewWidget::createLoadingPage()
{
	m_loadingPage = new QWidget(this);
	m_loadingPage->setStyleSheet("background-color: palette(base);");

	QLabel* loadingLab = new QLabel(m_loadingPage);
	loadingLab->setAlignment(Qt::AlignCenter);

	QMovie* movie = new QMovie(":/gif/loading.gif", QByteArray(), loadingLab);
	movie->setScaledSize(QSize(50, 50));
	loadingLab->setMovie(movie);
	movie->start();

	QVBoxLayout* layout = new QVBoxLayout(m_loadingPage);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch();
	layout->addWidget(loadingLab, 0, Qt::AlignCenter);
	layout->addStretch();
}

void ArchivePreviewWidget::createErrorPage()
{
	m_errorPage = new QWidget(this);
	m_errorPage->setStyleSheet("background-color: palette(base);");

	QLabel* errorLab = new QLabel(m_errorPage);
	errorLab->setText(tr("Failed to load archive"));
	errorLab->setAlignment(Qt::AlignCenter);

	QFont errorFont("Microsoft YaHei");
	errorFont.setPointSize(9);
	errorLab->setFont(errorFont);

	QVBoxLayout* layout = new QVBoxLayout(m_errorPage);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch();
	layout->addWidget(errorLab, 0, Qt::AlignCenter);
	layout->addStretch();
}

void ArchivePreviewWidget::createEncryptPage()
{
	m_encryptPage = new QWidget(this);
	m_encryptPage->setStyleSheet("background-color: palette(base);");

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

void ArchivePreviewWidget::createPreviewPage()
{
	m_previewPage = new QWidget(this);
	m_previewPage->setStyleSheet("background-color: palette(base);");

	ArchiveTreeWidget* treeWidget = new ArchiveTreeWidget(this);
	const ArchiveTreeNode* rootNode = m_archiveParser->getRootNode();
	treeWidget->refresh(rootNode);
	QVBoxLayout* previewPageLayout = new QVBoxLayout(m_previewPage);
	previewPageLayout->setContentsMargins(0, 0, 0, 0);
	previewPageLayout->setSpacing(0);
	previewPageLayout->addWidget(treeWidget);
}
