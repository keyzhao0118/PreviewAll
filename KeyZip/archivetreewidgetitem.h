#pragma once

#include <QTreeWidgetItem>

class ArchiveTreeWidgetItem : public QTreeWidgetItem
{
public:
	enum Column
	{
		Column_Name = 0,
		Column_CompressedSize,
		Column_OriginalSize,
		Column_Type,
		Column_ModifiedTime
	};

	using QTreeWidgetItem::QTreeWidgetItem;

	bool operator<(const QTreeWidgetItem& other) const override;
};
