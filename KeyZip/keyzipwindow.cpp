#include "keyzipwindow.h"
#include "archivetreewidget.h"
#include "keycardwidget.h"
#include "archiveparser.h"
#include "archiveextractor.h"
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
#include <QProcess>

KeyZipWindow::KeyZipWindow(QWidget* parent /*= nullptr*/)
	: QMainWindow(parent)
{
	initMenuAction();
	initCentralWidget();
	initStatusBar();
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
	m_actExtract = new QAction(tr("Extract Archive"), this);
	m_actLocation = new QAction(tr("Open Archive Location"));
	m_actClose = new QAction(tr("Close Archive"), this);
	m_actExit = new QAction(tr("Exit"), this);

	// 视图动作
	m_actPreview = new QAction(tr("Preview Panel"), this);
	m_actPreview->setCheckable(true);

	// 关于动作
	m_actAbout = new QAction(tr("About"), this);

	// 设置快捷键
	m_actOpen->setShortcut(QKeySequence::Open);
	m_actNew->setShortcut(QKeySequence::New);
	m_actExtract->setShortcut(QKeySequence("Ctrl+E"));
	m_actClose->setShortcut(QKeySequence("Ctrl+W"));
	m_actExit->setShortcut(QKeySequence("Ctrl+Q"));

	// 添加到菜单
	fileMenu->addAction(m_actOpen);
	fileMenu->addAction(m_actNew);
	fileMenu->addSeparator();
	fileMenu->addAction(m_actExtract);
	fileMenu->addAction(m_actLocation);
	fileMenu->addAction(m_actClose);
	fileMenu->addSeparator();
	fileMenu->addAction(m_actExit);
	viewMenu->addAction(m_actPreview);
	helpMenu->addAction(m_actAbout);

	// 初始化可用性
	m_actExtract->setEnabled(false);
	m_actLocation->setEnabled(false);
	m_actClose->setEnabled(false);
	m_actPreview->setEnabled(false);

	// 连接动作
	connect(m_actOpen, &QAction::triggered, this, &KeyZipWindow::onOpenTriggered);
	connect(m_actNew, &QAction::triggered, this, &KeyZipWindow::onNewTriggered);
	connect(m_actExtract, &QAction::triggered, this, &KeyZipWindow::onExtractTriggered);
	connect(m_actLocation, &QAction::triggered, this, &KeyZipWindow::onLocationTriggered);
	connect(m_actClose, &QAction::triggered, this, &KeyZipWindow::onCloseTriggered);
	connect(m_actExit, &QAction::triggered, this, &KeyZipWindow::onExitTriggered);
	connect(m_actPreview, &QAction::toggled, this, &KeyZipWindow::onPreviewToggled);
	connect(m_actAbout, &QAction::triggered, this, &KeyZipWindow::onAboutTriggered);
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

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 0);

	m_centralStackedLayout->addWidget(splitter);
	connect(m_centralStackedLayout, &QStackedLayout::currentChanged, this, &KeyZipWindow::onCentralStackedChanged);
}

void KeyZipWindow::initStatusBar()
{
	m_archiveInfoLab = new QLabel(this);
	statusBar()->addPermanentWidget(m_archiveInfoLab);
}

void KeyZipWindow::clearOld()
{
	setWindowTitle("KeyZip");
	if (m_treeWidget)
		m_treeWidget->clear();
	if (m_previewPanel)
		;//
	if (m_archiveInfoLab)
		m_archiveInfoLab->clear();

	m_archivePath.clear();
	
	m_archiveParser.reset(nullptr);
}

void KeyZipWindow::initArchiveParser()
{
	m_archiveParser.reset(new ArchiveParser());
	connect(m_archiveParser.data(), &ArchiveParser::requirePassword, this, &KeyZipWindow::onRequirePassword, Qt::BlockingQueuedConnection);
	connect(m_archiveParser.data(), &ArchiveParser::updateProgress, this, &KeyZipWindow::onUpdateProgress, Qt::BlockingQueuedConnection);
	connect(m_archiveParser.data(), &ArchiveParser::parseFailed, this, &KeyZipWindow::onParseFailed);
	connect(m_archiveParser.data(), &ArchiveParser::parseSucceed, this, &KeyZipWindow::onParseSucceed);
}

void KeyZipWindow::initArchiveExtractor()
{
	m_archiveExtractor.reset(new ArchiveExtractor());

}

void KeyZipWindow::onOpenTriggered()
{
	const QString filePath = QFileDialog::getOpenFileName(this, tr("Open Archive"), QString(), tr("Archives (*.zip *.7z *.rar);;All Files (*.*)"));
	if (filePath.isEmpty())
		return;

	clearOld();
	m_archivePath = filePath;
	m_centralStackedLayout->setCurrentIndex(1);

	initArchiveParser();
	m_archiveParser->parseArchive(m_archivePath);
}

void KeyZipWindow::onNewTriggered()
{
	QMessageBox::information(this, "", tr("Not Implemented Yet"));
}

void KeyZipWindow::onExtractTriggered()
{
	if (m_archivePath.isEmpty())
		return;

	const QString destDir = QFileDialog::getExistingDirectory(this, tr("Select Extraction Directory"), QString());
	if (destDir.isEmpty())
		return;

	if (QMessageBox::question(this, tr("Confirm Extraction"), tr("Extract archive to:\n%1").arg(destDir),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		return;

	initArchiveExtractor();
	m_archiveExtractor->extractArchive(m_archivePath, destDir);
}

void KeyZipWindow::onLocationTriggered()
{
	if (!m_archivePath.isEmpty() && QFile::exists(m_archivePath))
	{
		QString nativePath = QDir::toNativeSeparators(m_archivePath);
		QProcess::startDetached("explorer.exe", { "/select,", nativePath });
	}
}

void KeyZipWindow::onCloseTriggered()
{	
	clearOld();
	m_centralStackedLayout->setCurrentIndex(0);
}

void KeyZipWindow::onExitTriggered()
{
	close();
}

void KeyZipWindow::onPreviewToggled(bool checked)
{
	if (m_previewPanel)
		m_previewPanel->setVisible(checked);
}

void KeyZipWindow::onAboutTriggered()
{
	QMessageBox::information(this, tr("About KeyZip"), tr("KeyZip\nA simple archive manager."));
}

void KeyZipWindow::onCentralStackedChanged(int index)
{
	if (index == 0)// 首页
	{
		m_actExtract->setEnabled(false);
		m_actLocation->setEnabled(false);
		m_actClose->setEnabled(false);
		m_actPreview->setEnabled(false);
	}
	else if (index == 1)// 归档浏览页
	{
		m_actExtract->setEnabled(true);
		m_actLocation->setEnabled(true);
		m_actClose->setEnabled(true);
		m_actPreview->setEnabled(true);
	}
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

void KeyZipWindow::onParseFailed()
{
	clearOld();

	QMessageBox::critical(this, "", tr("Parsing Failed"));

	if (m_centralStackedLayout)
		m_centralStackedLayout->setCurrentIndex(0);
}

void KeyZipWindow::onParseSucceed()
{
	setWindowTitle(m_archivePath + " - KeyZip");
	if (!m_archiveParser)
		return;

	if (m_archiveInfoLab)
	{
		m_archiveInfoLab->setText(tr("File: %1, Folder: %2, Archive file size: %3")
			.arg(m_archiveParser->getFileCount())
			.arg(m_archiveParser->getFolderCount())
			.arg(CommonHelper::formatFileSize(QFileInfo(m_archivePath).size())));
	}
	if (m_treeWidget)
	{
		m_treeWidget->refresh(m_archiveParser->getRootNode());
	}
}
