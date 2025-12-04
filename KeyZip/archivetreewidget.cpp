#include "archivetreewidget.h"
#include "commonhelper.h"
#include <QScrollBar>
#include <QHeaderView>

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	setHeaderLabels({ tr("Name"), tr("Compressed Size"), tr("Original Size"), tr("Type"),tr("Modified Time")});
	setSortingEnabled(true);
	
	//header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	header()->setSectionsMovable(false);
	
	verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
	horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

	setAnimated(true);
}

void ArchiveTreeWidget::refresh(const QSharedPointer<ArchiveTreeNode>& rootNode)
{
	if (!rootNode)
		return;

	clear();
	QTreeWidgetItem* rootItem = invisibleRootItem();

	std::function<void(QTreeWidgetItem*, const QSharedPointer<ArchiveTreeNode>&)> addNode =
		[&](QTreeWidgetItem* parentItem, const QSharedPointer<ArchiveTreeNode>& node) {
			if (!parentItem || !node)
				return;

			auto* item = new QTreeWidgetItem(parentItem);
			item->setText(0, node->m_name);
			item->setText(1, CommonHelper::formatFileSize(node->m_compressedSize));
			item->setText(2, CommonHelper::formatFileSize(node->m_originalSize));
			item->setText(3, "XXX");
			item->setText(4, node->m_mtime.toString());

			// 原始数值用于排序（避免按文本排序）
			item->setData(1, Qt::UserRole, QVariant::fromValue<qulonglong>(node->m_compressedSize));
			item->setData(2, Qt::UserRole, QVariant::fromValue<qulonglong>(node->m_originalSize));
			item->setData(4, Qt::UserRole, node->m_mtime);

			parentItem->addChild(item);

			for (const auto& child : node->m_childNodes)
				addNode(item, child);
		};
	
	for (const auto& child : rootNode->m_childNodes)
		addNode(rootItem, child);

}

