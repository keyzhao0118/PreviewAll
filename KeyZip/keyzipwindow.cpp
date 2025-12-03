#include "keyzipwindow.h"
#include "archivetreewidget.h"
#include "keycardwidget.h"
#include "keyslideswitch.h"
#include "archiveparser.h"
#include "commonhelper.h"
#include <QHBoxLayout>
#include <QTreeView>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>

KeyZipWindow::KeyZipWindow(QWidget* parent /*= nullptr*/)
	: QMainWindow(parent)
{
	initMenuAction();
	initCentralWidget();
	initStatusBar();
	initArchiveParser();
	resize(1200, 600);
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
	QAction* actOpen = new QAction(tr("Open Archive"), this);
	QAction* actNew = new QAction(tr("New Archive"), this);
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
	actPreview->setChecked(true);
	actStatusBar->setChecked(true);

	// 关于动作
	QAction* actAbout = new QAction(tr("About"), this);

	// 设置快捷键
	actOpen->setShortcut(QKeySequence::Open);
	actNew->setShortcut(QKeySequence::New);
	actClose->setShortcut(QKeySequence("Ctrl+W"));
	actExit->setShortcut(QKeySequence("Ctrl+Q"));

	// 添加到菜单
	fileMenu->addAction(actOpen);
	fileMenu->addAction(actNew);
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
	connect(actOpen, &QAction::triggered, this, [this]() {
		const QString filePath = QFileDialog::getOpenFileName(this, tr("Open Archive"), QString(), tr("Archives (*.zip *.7z *.rar);;All Files (*.*)"));
		if (filePath.isEmpty())
			return;
		m_archivePath = filePath;

		if (m_archiveParser)
		{
			m_archiveParser->requestInterruption();
			if (!m_archiveParser->wait(1000))
			{
				CommonHelper::LogKeyZipDebugMsg("KeyZipWindow: Failed to stop ArchiveParser thread within 1 second.");
				return;
			}

			if (m_treeWidget)
				m_treeWidget->clearEntry();

			m_archiveParser->parseArchive(m_archivePath);
		}
	});

	connect(actNew, &QAction::triggered, this, [this]() {
		QMessageBox::information(this, "", tr("Not Implemented Yet"));
	});

	connect(actClose, &QAction::triggered, this, [this]() {
		m_archivePath.clear();

		if (m_archiveParser)
		{
			m_archiveParser->requestInterruption();
			if (!m_archiveParser->wait(1000))
			{
				CommonHelper::LogKeyZipDebugMsg("KeyZipWindow: Failed to stop ArchiveParser thread within 1 second.");
				return;
			}
		}

		if (m_treeWidget)
			m_treeWidget->clearEntry();
	});

	connect(actExit, &QAction::triggered, this, [this]() {
		close();
	});

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
	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setChildrenCollapsible(false);
	splitter->setOpaqueResize(true);
	splitter->setHandleWidth(6);

	m_treeWidget = new ArchiveTreeWidget(splitter);
	m_treeWidget->setMinimumWidth(500);

	m_previewPanel = new KeyCardWidget(splitter);
	m_previewPanel->setMinimumWidth(300);
	m_previewPanel->setBackgroundColor(Qt::red);

	splitter->addWidget(m_treeWidget);
	splitter->addWidget(m_previewPanel);

	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes({ 800,400 });

	setCentralWidget(splitter);
}

void KeyZipWindow::initStatusBar()
{
	statusBar()->setVisible(true);
}

void KeyZipWindow::initArchiveParser()
{
	m_archiveParser = new ArchiveParser(this);
	connect(m_archiveParser, &ArchiveParser::requirePassword, this, &KeyZipWindow::onRequirePassword, Qt::BlockingQueuedConnection);
	connect(m_archiveParser, &ArchiveParser::parsingFailed, this, &KeyZipWindow::onParsingFailed);
	connect(m_archiveParser, &ArchiveParser::entryFound, this, &KeyZipWindow::onEntryFound);
}

void KeyZipWindow::onRequirePassword(bool& bCancel, QString& password)
{
	bool ok = false;
	password = QInputDialog::getText(this, tr("Password Required"), tr("Enter Password:"), QLineEdit::Password, "", &ok);
	if (!ok)
		bCancel = true;
}

void KeyZipWindow::onParsingFailed()
{
	QMessageBox::critical(this, "", tr("Parsing Failed"));
}

void KeyZipWindow::onEntryFound(const QString& entryPath, bool bIsDir, quint64 entrySize)
{
	m_treeWidget->addEntry(entryPath, bIsDir, entrySize);
}
