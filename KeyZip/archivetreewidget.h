#pragma once

#include <QTreeWidget>
#include <QSharedPointer>

class ArchiveTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	explicit ArchiveTreeWidget(QWidget* parent = nullptr);
	~ArchiveTreeWidget();

	void addEntry(const QString& entryPath, bool bIsDir, quint64 entrySize);
};