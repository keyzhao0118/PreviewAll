#include "archivetreewidget.h"
#include "archivetreewidgetitem.h"
#include "commonhelper.h"
#include <QScrollBar>
#include <QHeaderView>

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	setHeaderLabels({ tr("Name"), tr("Compressed Size"), tr("Original Size"), tr("Type"),tr("Modified Time")});
	setSortingEnabled(true);
	
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
	
	// 添加压缩文件项
	auto archiveItem = addItem(invisibleRootItem(), rootNode);
	
	// 添加顶层项
	for (const ArchiveTreeNode* childNode : rootNode->m_childNodes)
		addItem(archiveItem, childNode);
}

void ArchiveTreeWidget::onItemExpanded(QTreeWidgetItem* parentItem)
{
	if (!parentItem)
		return;

	for (int i = 0; i < parentItem->childCount(); ++i)
		loadChildItems(parentItem->child(i));
}

QTreeWidgetItem* ArchiveTreeWidget::addItem(QTreeWidgetItem* parentItem, const ArchiveTreeNode* node)
{
	if (!parentItem || !node)
		return nullptr;

	auto* item = new ArchiveTreeWidgetItem(parentItem);
	item->setText(ArchiveTreeWidgetItem::Column_Name, node->m_name);
	item->setIcon(ArchiveTreeWidgetItem::Column_Name, CommonHelper::fileIconForName(node->m_name, node->m_bIsDir));
	item->setText(ArchiveTreeWidgetItem::Column_CompressedSize, node->m_bIsDir ? "" : CommonHelper::formatFileSize(node->m_compressedSize));
	item->setText(ArchiveTreeWidgetItem::Column_OriginalSize, node->m_bIsDir ? "" : CommonHelper::formatFileSize(node->m_originalSize));
	item->setText(ArchiveTreeWidgetItem::Column_Type, CommonHelper::fileTypeDisplayName(node->m_name, node->m_bIsDir));
	item->setText(ArchiveTreeWidgetItem::Column_ModifiedTime, node->m_mtime.toString("yyyy/M/d h:mm"));

	item->setTextAlignment(1, Qt::AlignRight);
	item->setTextAlignment(2, Qt::AlignRight);
	
	item->setData(ArchiveTreeWidgetItem::Column_Name, Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(node)));
	item->setData(ArchiveTreeWidgetItem::Column_Name, Qt::UserRole + 1, false);// 标记子项是否已添加，默认未添加
	item->setData(ArchiveTreeWidgetItem::Column_CompressedSize, Qt::UserRole, node->m_bIsDir ? 0 : node->m_compressedSize);
	item->setData(ArchiveTreeWidgetItem::Column_OriginalSize, Qt::UserRole, node->m_bIsDir ? 0 : node->m_originalSize);
	item->setData(ArchiveTreeWidgetItem::Column_Type, Qt::UserRole, node->m_bIsDir);
	item->setData(ArchiveTreeWidgetItem::Column_ModifiedTime, Qt::UserRole, node->m_mtime);

	parentItem->addChild(item);

	return item;
}

void ArchiveTreeWidget::loadChildItems(QTreeWidgetItem* item)
{
	if (!item)
		return;

	const bool bChildItemAdded = item->data(0, Qt::UserRole + 1).toBool();
	if (bChildItemAdded)
		return;

	QVariant nodeVariant = item->data(0, Qt::UserRole);
	quintptr ptr = nodeVariant.value<quintptr>();
	const ArchiveTreeNode* node = reinterpret_cast<const ArchiveTreeNode*>(ptr);
	if (!node)
		return;

	for (const ArchiveTreeNode* childNode : node->m_childNodes)
		addItem(item, childNode);

	item->setData(0, Qt::UserRole + 1, true);// 标记子项已添加
}

