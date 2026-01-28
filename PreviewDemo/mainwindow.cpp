#include "mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "previewhandlerhost.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle("PreviewDemo (Windows PreviewHandler + Qt)");

	auto* central = new QWidget(this);
	auto* rootLayout = new QVBoxLayout(central);
	rootLayout->setContentsMargins(10, 10, 10, 10);
	rootLayout->setSpacing(10);

	auto* topRow = new QHBoxLayout();
	m_pathEdit = new QLineEdit(central);
	m_pathEdit->setReadOnly(true);
	m_pathEdit->setPlaceholderText(QStringLiteral("请选择一个文件以显示系统预览..."));

	m_chooseButton = new QPushButton(QStringLiteral("选择文件..."), central);
	connect(m_chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFile);

	topRow->addWidget(m_pathEdit, 1);
	topRow->addWidget(m_chooseButton, 0);

	m_previewHost = new PreviewHandlerHost(central);

	rootLayout->addLayout(topRow);
	rootLayout->addWidget(m_previewHost, 1);
	setCentralWidget(central);

	statusBar()->showMessage(QStringLiteral("就绪：点击“选择文件...”"), 3000);
}

void MainWindow::chooseFile()
{
	const QString filePath = QFileDialog::getOpenFileName(
		this,
		QStringLiteral("选择要预览的文件"),
		m_currentFilePath.isEmpty() ? QString() : m_currentFilePath
	);

	if (filePath.isEmpty())
		return;

	setCurrentFile(filePath);
}

void MainWindow::setCurrentFile(const QString& path)
{
	m_currentFilePath = path;
	m_pathEdit->setText(path);

	QString err;
	if (!m_previewHost->loadFile(path, &err))
	{
		statusBar()->showMessage(QStringLiteral("预览失败"), 5000);
		QMessageBox::warning(this, QStringLiteral("无法预览"),
			QStringLiteral("系统未找到可用的 PreviewHandler，或该处理器初始化失败。\n\n文件：%1\n\n错误：%2")
				.arg(path, err.isEmpty() ? QStringLiteral("(无)") : err));
		return;
	}

	statusBar()->showMessage(QStringLiteral("已加载系统预览"), 3000);
}


