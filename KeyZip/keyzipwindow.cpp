#include "keyzipwindow.h"
#include "archivetreewidget.h"
#include "keycardwidget.h"
#include "keyslideswitch.h"
#include "archiveparser.h"
#include "commonhelper.h"
#include <QLayout>
#include <QStackedLayout>
#include <QPushButton>
#include <QTreeView>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>
#include <QLabel>

KeyZipWindow::KeyZipWindow(QWidget* parent /*= nullptr*/)
	: QMainWindow(parent)
	, m_archiveTree(QSharedPointer<ArchiveTree>::create())
{
	initMenuAction();
	initCentralWidget();
	initStatusBar();
	initArchiveParser();
	resize(900, 600);
}

KeyZipWindow::~KeyZipWindow()
{
}

void KeyZipWindow::initMenuAction()
{
	// 文件、编辑、帮助菜单
	QMenu* fileMenu = menuBar()->addMenu(tr("&File(F)"));
	QMenu* editMenu = menuBar()->addMenu(tr("&Edit(E)"));
	QMenu* viewMenu = menuBar()->addMenu(tr("&View(V)"));
	QMenu* helpMenu = menuBar()->addMenu(tr("&Help(H)"));

	// 文件动作
	m_actOpen = new QAction(tr("Open Archive"), this);
	m_actNew = new QAction(tr("New Archive"), this);
	QAction* actClose = new QAction(tr("Close Archive"), this);
	QAction* actExit = new QAction(tr("Exit"), this);

	// 编辑动作
	QAction* actExtractAll = new QAction(tr("Extract All"), this);
	QAction* actExtractSelected = new QAction(tr("Extract Selected"), this);
	QAction* actOpenSelected = new QAction(tr("Open Selected"), this);

	// 视图动作
	QAction* actPreview = new QAction(tr("Preview Panel"), this);
	QAction* actStatusBar = new QAction(tr("Status Bar"), this);
	actPreview->setCheckable(true);
	actStatusBar->setCheckable(true);
	actPreview->setChecked(false);
	actStatusBar->setChecked(true);

	// 关于动作
	QAction* actAbout = new QAction(tr("About"), this);

	// 设置快捷键
	m_actOpen->setShortcut(QKeySequence::Open);
	m_actNew->setShortcut(QKeySequence::New);
	actClose->setShortcut(QKeySequence("Ctrl+W"));
	actExit->setShortcut(QKeySequence("Ctrl+Q"));

	// 添加到菜单
	fileMenu->addAction(m_actOpen);
	fileMenu->addAction(m_actNew);
	fileMenu->addSeparator();
	fileMenu->addAction(actClose);
	fileMenu->addSeparator();
	fileMenu->addAction(actExit);
	editMenu->addAction(actExtractAll);
	editMenu->addAction(actExtractSelected);
	editMenu->addAction(actOpenSelected);
	viewMenu->addAction(actPreview);
	viewMenu->addAction(actStatusBar);
	helpMenu->addAction(actAbout);

	// 连接动作
	connect(m_actOpen, &QAction::triggered, this, [this]() {
		if (!m_archiveParser)
			return;
		m_archiveParser->requestInterruption();
		if (!m_archiveParser->wait(1000))
		{
			CommonHelper::LogKeyZipDebugMsg("KeyZipWindow: Failed to stop ArchiveParser thread within 1 second.");
			return;
		}

		const QString filePath = QFileDialog::getOpenFileName(this, tr("Open Archive"), QString(), tr("Archives (*.zip *.7z *.rar);;All Files (*.*)"));
		if (filePath.isEmpty())
			return;
		clearTreeInfo();
		if (m_centralStackedLayout && m_centralStackedLayout->count() >= 2)
		{
			m_centralStackedLayout->setCurrentIndex(1);
			m_archivePath = filePath;
			m_archiveParser->parseArchive(m_archivePath);
		}
	});

	connect(m_actNew, &QAction::triggered, this, [this]() {
		QMessageBox::information(this, "", tr("Not Implemented Yet"));
	});

	connect(actClose, &QAction::triggered, this, [this]() {
		if (m_archiveParser)
		{
			m_archiveParser->requestInterruption();
			if (!m_archiveParser->wait(1000))
				CommonHelper::LogKeyZipDebugMsg("KeyZipWindow: Failed to stop ArchiveParser thread within 1 second.");
		}
		clearTreeInfo();
		if(m_centralStackedLayout && m_centralStackedLayout->count() >= 1)
			m_centralStackedLayout->setCurrentIndex(0);
	});

	connect(actExit, &QAction::triggered, this, &KeyZipWindow::close);

	connect(actExtractAll, &QAction::triggered, this, [this]() {
		QMessageBox::information(this, "", tr("Not Implemented Yet"));
	});

	connect(actExtractSelected, &QAction::triggered, this, [this]() {
		QMessageBox::information(this, "", tr("Not Implemented Yet"));
	});

	connect(actOpenSelected, &QAction::triggered, this, [this]() {
		QMessageBox::information(this, "", tr("Not Implemented Yet"));
	});

	connect(actPreview, &QAction::triggered, this, [this](bool checked) {
		if (m_previewPanel)
			m_previewPanel->setVisible(checked);
	});

	connect(actStatusBar, &QAction::triggered, this, [this](bool checked) {
		statusBar()->setVisible(checked);
	});

	connect(actAbout, &QAction::triggered, this, [this]() {
		QMessageBox::about(this, tr("About"), tr("KeyZip\nSimple archive viewer."));
	});
}

void KeyZipWindow::initCentralWidget()
{
	QWidget* centralWidget = new QWidget(this);
	m_centralStackedLayout = new QStackedLayout(centralWidget);
	setCentralWidget(centralWidget);

	QWidget* homePage = new QWidget(centralWidget);
	QHBoxLayout* homeLayout = new QHBoxLayout(homePage);
	QPushButton* openBtn = new QPushButton(tr("Open Archive"), homePage);
	QPushButton* newBtn = new QPushButton(tr("New Archive"), homePage);
	openBtn->setFixedSize(150, 150);
	newBtn->setFixedSize(150, 150);
	connect(openBtn, &QPushButton::clicked, m_actOpen, &QAction::trigger);
	connect(newBtn, &QPushButton::clicked, m_actNew, &QAction::trigger);
	homeLayout->addStretch();
	homeLayout->addWidget(openBtn);
	homeLayout->addWidget(newBtn);
	homeLayout->addStretch();
	m_centralStackedLayout->addWidget(homePage);

	QSplitter* splitter = new QSplitter(Qt::Horizontal, centralWidget);
	splitter->setChildrenCollapsible(false);
	splitter->setOpaqueResize(true);
	splitter->setHandleWidth(6);

	m_treeWidget = new ArchiveTreeWidget(splitter);
	m_treeWidget->setMinimumWidth(500);

	m_previewPanel = new KeyCardWidget(splitter);
	m_previewPanel->setMinimumWidth(300);
	m_previewPanel->setBackgroundColor(Qt::gray);
	m_previewPanel->setVisible(false);

	splitter->addWidget(m_treeWidget);
	splitter->addWidget(m_previewPanel);

	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes({ 800,400 });

	m_centralStackedLayout->addWidget(splitter);
}

void KeyZipWindow::initStatusBar()
{
	statusBar()->setVisible(true);
	m_archiveInfoLab = new QLabel(this);
	statusBar()->addPermanentWidget(m_archiveInfoLab);
}

void KeyZipWindow::initArchiveParser()
{
	m_archiveParser = new ArchiveParser(this);
	connect(m_archiveParser, &ArchiveParser::requirePassword, this, &KeyZipWindow::onRequirePassword, Qt::BlockingQueuedConnection);
	connect(m_archiveParser, &ArchiveParser::updateProgress, this, &KeyZipWindow::onUpdateProgress, Qt::BlockingQueuedConnection);
	connect(m_archiveParser, &ArchiveParser::entryFound, this, &KeyZipWindow::onEntryFound, Qt::DirectConnection);
	connect(m_archiveParser, &ArchiveParser::parsingFailed, this, &KeyZipWindow::onParsingFailed);
	connect(m_archiveParser, &ArchiveParser::parsingSucceed, this, &KeyZipWindow::onParsingSucceed);
}

void KeyZipWindow::clearTreeInfo()
{
	if (m_treeWidget)
		m_treeWidget->clear();
	if (m_previewPanel)
		;//
	if (m_archiveInfoLab)
		m_archiveInfoLab->clear();
	if (m_archiveTree)
		m_archiveTree->clear();

	m_archivePath.clear();
	setWindowTitle("KeyZip");
}

void KeyZipWindow::onRequirePassword(bool& bCancel, QString& password)
{
	bool ok = false;
	password = QInputDialog::getText(this, tr("Password Required"), tr("Enter Password:"), QLineEdit::Password, "", &ok);
	if (!ok)
		bCancel = true;
}

void KeyZipWindow::onUpdateProgress(quint64 completed, quint64 total)
{
	if (m_archiveInfoLab)
		m_archiveInfoLab->setText(tr("Parsing Archive: %1 / %2").arg(completed).arg(total));
}

void KeyZipWindow::onEntryFound()
{
	if (m_archiveTree && m_archiveParser)
		m_archiveTree->addEntry(m_archiveParser->getEntryCache());
}

void KeyZipWindow::onParsingFailed()
{
	QMessageBox::critical(this, "", tr("Parsing Failed"));
}

void KeyZipWindow::onParsingSucceed()
{
	if (m_archiveInfoLab && m_treeWidget && m_archiveTree)
	{
		setWindowTitle(m_archivePath + " - KeyZip");
		m_archiveInfoLab->setText(tr("File: %1, Folder: %2, Archive file size: %3")
			.arg(m_archiveTree->getFileCount())
			.arg(m_archiveTree->getFolderCount())
			.arg(CommonHelper::formatFileSize(QFileInfo(m_archivePath).size())));

		m_treeWidget->refresh(m_archiveTree->getRootNode());
	}
}
