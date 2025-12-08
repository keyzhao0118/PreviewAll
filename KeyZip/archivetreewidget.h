#pragma once

#include <QTreeWidget>
#include "archivetree.h"

class ArchiveTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	explicit ArchiveTreeWidget(QWidget* parent = nullptr);
	~ArchiveTreeWidget() = default;

	void refresh(const QSharedPointer<ArchiveTreeNode>& rootNode);

private slots:
	void onItemExpanded(QTreeWidgetItem* parentItem);

private:
	void addItem(QTreeWidgetItem* parentItem, const QSharedPointer<ArchiveTreeNode>& node);
	void loadChildItems(QTreeWidgetItem* item);
};