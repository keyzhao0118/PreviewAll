#include "archivetreewidget.h"
#include "commonhelper.h"
#include <QScrollBar>
#include <QHeaderView>

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	setHeaderLabels({ tr("Name"), tr("Compressed Size"), tr("Original Size"), tr("Type"),tr("Modified Time")});
	setSortingEnabled(true);
	
	header()->setSectionsMovable(false);
	
	verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
	horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

	setAnimated(true);

	connect(this, &QTreeWidget::itemExpanded, this, &ArchiveTreeWidget::onItemExpanded);
}

void ArchiveTreeWidget::refresh(const ArchiveTreeNode* rootNode)
{
	if (!rootNode)
		return;

	clear();
	QTreeWidgetItem* rootItem = invisibleRootItem();
	
	// 添加顶层项
	for (const ArchiveTreeNode* childNode : rootNode->m_childNodes)
		addItem(rootItem, childNode);
	
	// 添加顶层项的子项
	for (int i = 0; i < rootItem->childCount(); ++i)
		loadChildItems(rootItem->child(i));
}

void ArchiveTreeWidget::onItemExpanded(QTreeWidgetItem* parentItem)
{
	if (!parentItem)
		return;

	for (int i = 0; i < parentItem->childCount(); ++i)
		loadChildItems(parentItem->child(i));
}

void ArchiveTreeWidget::addItem(QTreeWidgetItem* parentItem, const ArchiveTreeNode* node)
{
	if (!parentItem || !node)
		return;

	auto* item = new QTreeWidgetItem(parentItem);
	item->setText(0, node->m_name);
	item->setIcon(0, CommonHelper::fileIconForName(node->m_name, node->m_bIsDir));
	item->setText(1, node->m_bIsDir ? "" : CommonHelper::formatFileSize(node->m_compressedSize));
	item->setText(2, node->m_bIsDir ? "" : CommonHelper::formatFileSize(node->m_originalSize));
	item->setText(3, CommonHelper::fileTypeDisplayName(node->m_name, node->m_bIsDir));
	item->setText(4, node->m_mtime.toString("yyyy/M/d h:mm"));

	item->setTextAlignment(1, Qt::AlignRight);
	item->setTextAlignment(2, Qt::AlignRight);

	item->setData(0, Qt::UserRole, QVariant::fromValue<const ArchiveTreeNode*>(node));
	item->setData(0, Qt::UserRole + 1, false);// 标记子项是否已添加，默认未添加
	item->setData(1, Qt::UserRole, node->m_bIsDir ? 0 : node->m_compressedSize);
	item->setData(2, Qt::UserRole, node->m_bIsDir ? 0 : node->m_originalSize);
	item->setData(3, Qt::UserRole, node->m_bIsDir);
	item->setData(4, Qt::UserRole, node->m_mtime);

	parentItem->addChild(item);
}

void ArchiveTreeWidget::loadChildItems(QTreeWidgetItem* item)
{
	if (!item)
		return;

	const bool bChildItemAdded = item->data(0, Qt::UserRole + 1).toBool();
	if (bChildItemAdded)
		return;

	auto nodeVariant = item->data(0, Qt::UserRole);
	auto node = nodeVariant.value<const ArchiveTreeNode*>();
	if (!node)
		return;

	for (const ArchiveTreeNode* childNode : node->m_childNodes)
		addItem(item, childNode);

	item->setData(0, Qt::UserRole + 1, true);// 标记子项已添加
}

