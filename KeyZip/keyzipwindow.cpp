#include "keyzipwindow.h"
#include "archivetreewidget.h"
#include "archiveparser.h"
#include <QTreeView>
#include <QMessageBox>

KeyZipWindow::KeyZipWindow(QWidget* parent /*= nullptr*/)
	: QMainWindow(parent)
{
	m_treeWidget = new ArchiveTreeWidget(this);
	setCentralWidget(m_treeWidget);

	m_archiveParser = new ArchiveParser(this);

	connect(m_archiveParser, &ArchiveParser::parsingFailed, this, &KeyZipWindow::onParsingFailed);
	connect(m_archiveParser, &ArchiveParser::entryFound, this, &KeyZipWindow::onEntryFound);

	m_archiveParser->parseArchive("C:\\Users\\keyzhao\\Desktop\\b b.rar");
}

KeyZipWindow::~KeyZipWindow()
{
}

void KeyZipWindow::onParsingFailed(const QString& errorMsg)
{
	QMessageBox::critical(this, tr("Parsing Failed"), errorMsg);
	close();
}

void KeyZipWindow::onEntryFound(const QString& entryPath, bool bIsDir, quint64 entrySize)
{
	m_treeWidget->addEntry(entryPath, bIsDir, entrySize);
}
