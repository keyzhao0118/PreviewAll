#include "archivetreewidget.h"
#include <QScrollBar>
#include <QHeaderView>

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	setHeaderLabels({ tr("Name"), tr("Compressed Size"), tr("Original Size"), tr("Type"),tr("Modified Time")});
	setSortingEnabled(true);
	
	header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	header()->setSectionsMovable(false);
	
	verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
	horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
}

ArchiveTreeWidget::~ArchiveTreeWidget()
{
}

void ArchiveTreeWidget::clearEntry()
{
	clear();
}

void ArchiveTreeWidget::addEntry(const QString& entryPath, bool bIsDir, quint64 entrySize)
{
	QTreeWidgetItem* parent = invisibleRootItem();
	parent->addChild(new QTreeWidgetItem({ entryPath, QString::number(entrySize) }));
}

