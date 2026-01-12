#include "archivetreewidgetitem.h"

bool ArchiveTreeWidgetItem::operator<(const QTreeWidgetItem& other) const
{
	const auto* rhs = static_cast<const ArchiveTreeWidgetItem*>(&other);
	bool thisIsDir = data(Column_Name, IsDirRole).toBool();
	bool otherIsDir = rhs->data(Column_Name, IsDirRole).toBool();
	if (thisIsDir != otherIsDir)
		return thisIsDir;

	int column = treeWidget()->sortColumn();
	if (column == Column_Name)
		return text(column).toLower() < rhs->text(column).toLower();
	else
		return QTreeWidgetItem::operator<(other);
}
