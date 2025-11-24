#include "archivetreewidget.h"

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	setColumnCount(2);
	setHeaderLabels({ tr("Name"), tr("Size") });
	setSortingEnabled(true);
}

ArchiveTreeWidget::~ArchiveTreeWidget()
{
}

void ArchiveTreeWidget::addEntry(const QString& entryPath, bool bIsDir, quint64 entrySize)
{
	QTreeWidgetItem* parent = invisibleRootItem();
	parent->addChild(new QTreeWidgetItem({ entryPath, QString::number(entrySize) }));
}

