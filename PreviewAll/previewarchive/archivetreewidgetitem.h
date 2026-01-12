#pragma once

#include <QTreeWidgetItem>

class ArchiveTreeWidgetItem : public QTreeWidgetItem
{
public:
	enum
	{
		Column_Name = 0,
	};

	enum
	{
		NodeRole = Qt::UserRole,
		IsDirRole = Qt::UserRole + 1,
		IsChildrenLoadedRole = Qt::UserRole + 2,
	};

	using QTreeWidgetItem::QTreeWidgetItem;

	bool operator<(const QTreeWidgetItem& other) const override;
};
