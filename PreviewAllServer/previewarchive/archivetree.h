#pragma once

#include <QObject>
#include <QHash>

class ArchiveTreeNode
{
public:
	
	ArchiveTreeNode(const QString& name, bool bIsDir);
	~ArchiveTreeNode();

	void addChild(const QString& name, ArchiveTreeNode* childNode);

public:
	QString m_name;
	bool m_bIsDir = false;

	ArchiveTreeNode* m_parentNode = nullptr;
	QHash<QString, ArchiveTreeNode*> m_childNodes;
};

class ArchiveTree
{
public:
	explicit ArchiveTree(const QString& archivePath);
	~ArchiveTree();

	void addEntry(const QString& path, bool bIsDir);

	const ArchiveTreeNode* getRootNode() const;
	quint64 getFileCount() const;
	quint64 getFolderCount() const;

private:
	ArchiveTreeNode* m_rootNode = nullptr;
};