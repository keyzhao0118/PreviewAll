#include "archivetreewidget.h"
#include "archivetreewidgetitem.h"
#include <QFileIconProvider>
#include <QScrollBar>
#include <QHeaderView>

namespace
{
	QIcon fileIconForName(const QString& name, bool bIsDir)
	{
		static QHash<QString, QIcon> iconCache;

		if (bIsDir)
		{
			if (!iconCache.contains("folder"))
			{
				QFileIconProvider provider;
				QIcon folderIcon = provider.icon(QFileIconProvider::Folder);
				iconCache.insert("folder", folderIcon);
				return folderIcon;
			}
			else
			{
				return iconCache.value("folder");
			}
		}

		const int dot = name.lastIndexOf('.');
		if (dot < 0 || dot == name.length() - 1)
		{
			if (!iconCache.contains("file"))
			{
				QFileIconProvider provider;
				QIcon fileIcon = provider.icon(QFileIconProvider::File);
				iconCache.insert("file", fileIcon);
				return fileIcon;
			}
			else
			{
				return iconCache.value("file");
			}
		}

		const QString ext = name.mid(dot);
		if (!iconCache.contains(ext))
		{
			QFileIconProvider provider;
			QIcon extIcon = provider.icon(QFileInfo(ext));
			iconCache.insert(ext, extIcon);
			return extIcon;
		}
		else
		{
			return iconCache.value(ext);
		}
	}
}

ArchiveTreeWidget::ArchiveTreeWidget(QWidget* parent /*= nullptr*/)
	: QTreeWidget(parent)
{
	header()->setVisible(false);
	setAnimated(true);
	
	verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
	horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

	connect(this, &QTreeWidget::itemExpanded, this, &ArchiveTreeWidget::onItemExpanded);
}

void ArchiveTreeWidget::refresh(const ArchiveTreeNode* rootNode)
{
	clear();

	if (!rootNode)
		return;

	// 添加压缩文件项
	auto archiveItem = addItem(invisibleRootItem(), rootNode);
	
	// 添加顶层项
	for (const ArchiveTreeNode* childNode : rootNode->m_childNodes)
		addItem(archiveItem, childNode);

	archiveItem->setExpanded(true);
}

void ArchiveTreeWidget::onItemExpanded(QTreeWidgetItem* parentItem)
{
	if (!parentItem)
		return;

	for (int i = 0; i < parentItem->childCount(); ++i)
		loadChildItems(parentItem->child(i));

	sortItems(ArchiveTreeWidgetItem::Column_Name, Qt::AscendingOrder);
}

QTreeWidgetItem* ArchiveTreeWidget::addItem(QTreeWidgetItem* parentItem, const ArchiveTreeNode* node)
{
	if (!parentItem || !node)
		return nullptr;

	ArchiveTreeWidgetItem* item = new ArchiveTreeWidgetItem(parentItem);
	item->setText(ArchiveTreeWidgetItem::Column_Name, node->m_name);
	item->setIcon(ArchiveTreeWidgetItem::Column_Name, fileIconForName(node->m_name, node->m_bIsDir));

	item->setData(ArchiveTreeWidgetItem::Column_Name, ArchiveTreeWidgetItem::NodeRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(node)));
	item->setData(ArchiveTreeWidgetItem::Column_Name, ArchiveTreeWidgetItem::IsDirRole, node->m_bIsDir);
	item->setData(ArchiveTreeWidgetItem::Column_Name, ArchiveTreeWidgetItem::IsChildrenLoadedRole, false);// 标记子项是否已添加，默认未添加


	return item;
}

void ArchiveTreeWidget::loadChildItems(QTreeWidgetItem* item)
{
	if (!item)
		return;

	const bool bChildItemAdded = item->data(0, ArchiveTreeWidgetItem::IsChildrenLoadedRole).toBool();
	if (bChildItemAdded)
		return;

	QVariant nodeVariant = item->data(0, ArchiveTreeWidgetItem::NodeRole);
	quintptr ptr = nodeVariant.value<quintptr>();
	const ArchiveTreeNode* node = reinterpret_cast<const ArchiveTreeNode*>(ptr);
	if (!node)
		return;

	for (const ArchiveTreeNode* childNode : node->m_childNodes)
		addItem(item, childNode);

	item->setData(0, ArchiveTreeWidgetItem::IsChildrenLoadedRole, true);// 标记子项已添加
}

