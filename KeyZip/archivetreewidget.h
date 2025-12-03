#pragma once

#include <QTreeWidget>
#include <QSharedPointer>
//#include "archiveentrynode.h"

class ArchiveTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	explicit ArchiveTreeWidget(QWidget* parent = nullptr);
	~ArchiveTreeWidget();
	
	void clearEntry();

	void addEntry(const QString& entryPath, bool bIsDir, quint64 entrySize);

private:
	//QSharedPointer<ArchiveEntryNode> m_rootNode;
};