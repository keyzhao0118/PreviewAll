#include "archivetreewidgetitem.h"
#include <QDateTime>

bool ArchiveTreeWidgetItem::operator<(const QTreeWidgetItem& other) const
{
	const auto* rhs = static_cast<const ArchiveTreeWidgetItem*>(&other);
	bool thisIsDir = data(Column_Type, Qt::UserRole).toBool();
	bool otherIsDir = rhs->data(Column_Type, Qt::UserRole).toBool();
	if (thisIsDir != otherIsDir)
		return thisIsDir;

	int column = treeWidget()->sortColumn();
	switch (column)
	{
	case Column_CompressedSize:
		return data(Column_CompressedSize, Qt::UserRole).toLongLong()
			< rhs->data(Column_CompressedSize, Qt::UserRole).toLongLong();
	case Column_OriginalSize:
		return data(Column_OriginalSize, Qt::UserRole).toLongLong()
			< rhs->data(Column_OriginalSize, Qt::UserRole).toLongLong();
	case Column_ModifiedTime:
		return data(Column_ModifiedTime, Qt::UserRole).toDateTime()
			< rhs->data(Column_ModifiedTime, Qt::UserRole).toDateTime();
	default:
		return QTreeWidgetItem::operator<(other);
	}
}
