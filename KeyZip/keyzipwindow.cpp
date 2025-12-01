#include "keyzipwindow.h"
#include "archivetreewidget.h"
#include "archiveparser.h"
#include <QTreeView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

KeyZipWindow::KeyZipWindow(QWidget* parent /*= nullptr*/)
	: QMainWindow(parent)
{
	m_treeWidget = new ArchiveTreeWidget(this);
	setCentralWidget(m_treeWidget);

	m_archiveParser = new ArchiveParser(this);

	connect(m_archiveParser, &ArchiveParser::requirePassword, this, &KeyZipWindow::onRequirePassword, Qt::BlockingQueuedConnection);
	connect(m_archiveParser, &ArchiveParser::parsingFailed, this, &KeyZipWindow::onParsingFailed);
	connect(m_archiveParser, &ArchiveParser::entryFound, this, &KeyZipWindow::onEntryFound);

	QString archivePath = QFileDialog::getOpenFileName(this);
	if (archivePath.isEmpty())
	{
		close();
		return;
	}

	m_archiveParser->parseArchive(archivePath);
}

KeyZipWindow::~KeyZipWindow()
{
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
	close();
}

void KeyZipWindow::onEntryFound(const QString& entryPath, bool bIsDir, quint64 entrySize)
{
	m_treeWidget->addEntry(entryPath, bIsDir, entrySize);
}
