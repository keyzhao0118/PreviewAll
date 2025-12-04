#include "archivetreewidget.h"
#include "commonhelper.h"
#include <QScrollBar>
#include <QHeaderView>

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	qRegisterMetaType<QSharedPointer<ArchiveTreeNode>>("QSharedPointer<ArchiveTreeNode>");
	setHeaderLabels({ tr("Name"), tr("Compressed Size"), tr("Original Size"), tr("Type"),tr("Modified Time")});
	setSortingEnabled(true);
	
	header()->setSectionsMovable(false);
	
	verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
	horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

	setAnimated(true);

	connect(this, &ArchiveTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* parentItem) {
		for(int i = 0; i < parentItem->childCount(); ++i)
		{
			auto* childItem = parentItem->child(i);
			auto nodeVariant = childItem->data(0, Qt::UserRole);
			auto node = nodeVariant.value<QSharedPointer<ArchiveTreeNode>>();
			for (const auto& child : node->m_childNodes)
			{
				addNode(childItem, child);
			}
		}
	});
}

void ArchiveTreeWidget::refresh(const QSharedPointer<ArchiveTreeNode>& rootNode)
{
	if (!rootNode)
		return;

	clear();
	QTreeWidgetItem* rootItem = invisibleRootItem();
	
	for (const auto& child : rootNode->m_childNodes)
	{
		auto childItem = addNode(rootItem, child);
		for (const auto& cchild : child->m_childNodes)
			addNode(childItem, cchild);
	}

}

QTreeWidgetItem* ArchiveTreeWidget::addNode(QTreeWidgetItem* parentItem, const QSharedPointer<ArchiveTreeNode>& node)
{
	if (!parentItem || !node)
		return nullptr;

	auto* item = new QTreeWidgetItem(parentItem);
	item->setText(0, node->m_name);
	item->setText(1, CommonHelper::formatFileSize(node->m_compressedSize));
	item->setText(2, CommonHelper::formatFileSize(node->m_originalSize));
	item->setText(3, "XXX");
	item->setText(4, node->m_mtime.toString());

	item->setData(0, Qt::UserRole, QVariant::fromValue<QSharedPointer<ArchiveTreeNode>>(node));
	item->setData(1, Qt::UserRole, QVariant::fromValue<qulonglong>(node->m_compressedSize));
	item->setData(2, Qt::UserRole, QVariant::fromValue<qulonglong>(node->m_originalSize));
	item->setData(4, Qt::UserRole, node->m_mtime);

	parentItem->addChild(item);

	return item;
}

