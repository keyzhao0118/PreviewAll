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
#include <QProgressDialog>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>
#include <QLabel>
#include <QProcess>

KeyZipWindow::KeyZipWindow(QWidget* parent /*= nullptr*/)
	: QMainWindow(parent)
{
	initAction();
	initMenu();
	initToolBar();
	initCentralWidget();
	initStatusBar();
	setContextMenuPolicy(Qt::NoContextMenu);
	setWindowIcon(QIcon(":/icons/extract_all.svg"));
	resize(900, 600);
}

KeyZipWindow::~KeyZipWindow()
{
}

void KeyZipWindow::initAction()
{
	// 文件动作
	m_actOpen = new QAction(tr("Open Archive"), this);
	m_actNew = new QAction(tr("New Archive"), this);
	m_actExtractAll = new QAction(tr("Extract All"), this);
	m_actExtractSelect = new QAction(tr("Extract Select"), this);
	m_actLocation = new QAction(tr("Open Archive Location"));
	m_actClose = new QAction(tr("Close Archive"), this);
	m_actExit = new QAction(tr("Exit"), this);

	// 编辑动作
	m_actAdd = new QAction(tr("Add Files"), this);
	m_actDelete = new QAction(tr("Delete Files"), this);

	// 视图动作
	m_actPreview = new QAction(tr("Preview Panel"), this);
	m_actPreview->setCheckable(true);

	// 关于动作
	m_actAbout = new QAction(tr("About"), this);

	// 设置快捷键
	m_actOpen->setShortcut(QKeySequence::Open);
	m_actNew->setShortcut(QKeySequence::New);
	m_actExtractAll->setShortcut(QKeySequence("Ctrl+E"));
	m_actClose->setShortcut(QKeySequence("Ctrl+W"));
	m_actExit->setShortcut(QKeySequence("Ctrl+Q"));

	// 初始化可用性
	m_actExtractAll->setEnabled(false);
	m_actExtractSelect->setEnabled(false);
	m_actLocation->setEnabled(false);
	m_actClose->setEnabled(false);
	m_actAdd->setEnabled(false);
	m_actDelete->setEnabled(false);
	m_actPreview->setEnabled(false);

	// 连接动作
	connect(m_actOpen, &QAction::triggered, this, &KeyZipWindow::onOpenTriggered);
	connect(m_actNew, &QAction::triggered, this, &KeyZipWindow::onNewTriggered);
	connect(m_actExtractAll, &QAction::triggered, this, &KeyZipWindow::onExtractAllTriggered);
	connect(m_actExtractSelect, &QAction::triggered, this, &KeyZipWindow::onExtractSelectTriggered);
	connect(m_actLocation, &QAction::triggered, this, &KeyZipWindow::onLocationTriggered);
	connect(m_actClose, &QAction::triggered, this, &KeyZipWindow::onCloseTriggered);
	connect(m_actExit, &QAction::triggered, this, &KeyZipWindow::onExitTriggered);

	connect(m_actAdd, &QAction::triggered, this, &KeyZipWindow::onAddTriggered);
	connect(m_actDelete, &QAction::triggered, this, &KeyZipWindow::onDeleteTriggered);

	connect(m_actPreview, &QAction::toggled, this, &KeyZipWindow::onPreviewToggled);
	connect(m_actAbout, &QAction::triggered, this, &KeyZipWindow::onAboutTriggered);
}

void KeyZipWindow::initMenu()
{
	// 文件、编辑、视图、帮助菜单
	QMenu* fileMenu = menuBar()->addMenu(tr("&File(F)"));
	QMenu* editMenu = menuBar()->addMenu(tr("&Edit(E)"));
	QMenu* viewMenu = menuBar()->addMenu(tr("&View(V)"));
	QMenu* helpMenu = menuBar()->addMenu(tr("&Help(H)"));

	fileMenu->addAction(m_actOpen);
	fileMenu->addAction(m_actNew);
	fileMenu->addSeparator();
	fileMenu->addAction(m_actExtractAll);
	fileMenu->addAction(m_actExtractSelect);
	fileMenu->addSeparator();
	fileMenu->addAction(m_actLocation);
	fileMenu->addAction(m_actClose);
	fileMenu->addSeparator();
	fileMenu->addAction(m_actExit);

	editMenu->addAction(m_actAdd);
	editMenu->addAction(m_actDelete);

	viewMenu->addAction(m_actPreview);
	helpMenu->addAction(m_actAbout);
}

void KeyZipWindow::initToolBar()
{
	m_toolBar = addToolBar(tr("Tool Bar"));
	m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	m_toolBar->addAction(m_actExtractAll);
	m_toolBar->addAction(m_actExtractSelect);
	m_toolBar->addSeparator();
	m_toolBar->addAction(m_actAdd);
	m_toolBar->addAction(m_actDelete);

	m_toolBar->setVisible(false);
	m_toolBar->setMovable(false);

	m_actExtractAll->setIcon(QIcon(":/icons/extract_all.svg"));
	m_actExtractSelect->setIcon(QIcon(":/icons/extract_select.svg"));
	m_actAdd->setIcon(QIcon(":/icons/add.svg"));
	m_actDelete->setIcon(QIcon(":/icons/delete.svg"));

	m_toolBar->setIconSize(QSize(48, 48));
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
	connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, [this]() {
		const bool bHasSelection = !m_treeWidget->selectedItems().isEmpty();
		m_actExtractSelect->setEnabled(bHasSelection);
		m_actDelete->setEnabled(bHasSelection);
	});

	m_previewPanel = new KeyCardWidget(splitter);
	m_previewPanel->setMinimumWidth(300);
	m_previewPanel->setBackgroundColor(Qt::gray);
	m_previewPanel->setVisible(false);//ToDo:使用配置文件控制

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

void KeyZipWindow::startArchiveParser()
{
	m_archiveParser.reset(new ArchiveParser());
	connect(m_archiveParser.data(), &ArchiveParser::requirePassword, this, &KeyZipWindow::onRequirePassword, Qt::BlockingQueuedConnection);
	connect(m_archiveParser.data(), &ArchiveParser::updateProgress, this, &KeyZipWindow::onUpdateParseProgress, Qt::BlockingQueuedConnection);
	connect(m_archiveParser.data(), &ArchiveParser::parseFailed, this, &KeyZipWindow::onParseFailed);
	connect(m_archiveParser.data(), &ArchiveParser::parseSucceed, this, &KeyZipWindow::onParseSucceed);

	m_bParseCanceled = false;
	m_archiveParser->parseArchive(m_archivePath);
}

void KeyZipWindow::startArchiveExtractor(const QString& archivePath, const QString& destDirPath)
{
	m_archiveExtractor.reset(new ArchiveExtractor());
	connect(m_archiveExtractor.data(), &ArchiveExtractor::requirePassword, this, &KeyZipWindow::onRequirePassword, Qt::BlockingQueuedConnection);
	connect(m_archiveExtractor.data(), &ArchiveExtractor::updateProgress, this, &KeyZipWindow::onUpdateExtractProgress, Qt::BlockingQueuedConnection);
	connect(m_archiveExtractor.data(), &ArchiveExtractor::extractFailed, this, &KeyZipWindow::onExtractFailed);
	connect(m_archiveExtractor.data(), &ArchiveExtractor::extractSucceed, this, &KeyZipWindow::onExtractSucceed);
	
	m_bExtractCanceled = false;
	m_archiveExtractor->extractArchive(archivePath, destDirPath);
}

void KeyZipWindow::onOpenTriggered()
{
	const QString filePath = QFileDialog::getOpenFileName(this, tr("Open Archive"), QString(), tr("Archives (*.zip *.7z *.rar);;All Files (*.*)"));
	if (filePath.isEmpty())
		return;

	clearOld();
	m_archivePath = filePath;
	m_centralStackedLayout->setCurrentIndex(1);

	startArchiveParser();
}

void KeyZipWindow::onNewTriggered()
{
	QMessageBox::information(this, "", tr("Not Implemented Yet"));
}

void KeyZipWindow::onExtractAllTriggered()
{
	if (m_archivePath.isEmpty())
		return;

	const QString destDirPath = QFileDialog::getExistingDirectory(this, tr("Select Extraction Directory"), QString());
	if (destDirPath.isEmpty())
		return;

	if (QMessageBox::question(this, tr("Confirm Extraction"), tr("Extract archive to:\n%1").arg(destDirPath),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		return;

	startArchiveExtractor(m_archivePath, destDirPath);
}

void KeyZipWindow::onExtractSelectTriggered()
{
	QMessageBox::information(this, "", tr("Not Implemented Yet"));
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

void KeyZipWindow::onAddTriggered()
{
	QMessageBox::information(this, "", tr("Not Implemented Yet"));
}

void KeyZipWindow::onDeleteTriggered()
{
	QMessageBox::information(this, "", tr("Not Implemented Yet"));
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
		m_toolBar->setVisible(false);
		m_actExtractAll->setEnabled(false);
		m_actExtractSelect->setEnabled(false);
		m_actLocation->setEnabled(false);
		m_actClose->setEnabled(false);
		m_actAdd->setEnabled(false);
		m_actDelete->setEnabled(false);
		m_actPreview->setEnabled(false);
	}
}

void KeyZipWindow::onRequirePassword(bool& bCancel, QString& password)
{
	bool ok = false;
	password = QInputDialog::getText(this, tr("Password Required"), tr("Enter Password:"), QLineEdit::Password, "", &ok);
	if (!ok)
		bCancel = true;
}

void KeyZipWindow::onUpdateParseProgress(quint64 completed, quint64 total)
{
	if (!m_parseProgressDlg)
	{
		m_parseProgressDlg = new QProgressDialog(tr("Parsing..."), tr("Cancel"), 0, 100, this);
		m_parseProgressDlg->setWindowModality(Qt::WindowModal);
		connect(m_parseProgressDlg, &QProgressDialog::canceled, this, [this]() {
			if (m_archiveParser)
				m_archiveParser->requestInterruption();
			m_bParseCanceled = true;
		});
	}

	if (m_bParseCanceled)
		return;

	if (m_parseProgressDlg->isHidden())
		m_parseProgressDlg->show();
	m_parseProgressDlg->setValue(static_cast<int>((completed * 100) / total));
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

	m_toolBar->setVisible(true);
	m_actExtractAll->setEnabled(true);
	m_actLocation->setEnabled(true);
	m_actClose->setEnabled(true);
	m_actAdd->setEnabled(true);
	m_actPreview->setEnabled(true);

	m_treeWidget->refresh(m_archiveParser->getRootNode());

	m_archiveInfoLab->setText(tr("File: %1, Folder: %2, Archive file size: %3")
		.arg(m_archiveParser->getFileCount())
		.arg(m_archiveParser->getFolderCount())
		.arg(CommonHelper::formatFileSize(QFileInfo(m_archivePath).size())));
}

void KeyZipWindow::onUpdateExtractProgress(quint64 completed, quint64 total)
{
	if (!m_extractProgressDlg)
	{
		m_extractProgressDlg = new QProgressDialog(tr("Parsing..."), tr("Cancel"), 0, 100, this);
		m_extractProgressDlg->setWindowModality(Qt::WindowModal);
		connect(m_extractProgressDlg, &QProgressDialog::canceled, this, [this]() {
			if (m_archiveExtractor)
				m_archiveExtractor->requestInterruption();
			m_bExtractCanceled = true;
		});
	}

	if (m_bExtractCanceled)
		return;

	if (m_extractProgressDlg->isHidden())
		m_extractProgressDlg->show();
	m_extractProgressDlg->setValue(static_cast<int>((completed * 100) / total));
}

void KeyZipWindow::onExtractFailed()
{
	QMessageBox::critical(this, "", tr("Extraction Failed"));
}

void KeyZipWindow::onExtractSucceed()
{
	QMessageBox::critical(this, "", tr("Extraction Succeed"));
}
