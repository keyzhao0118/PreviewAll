#pragma once

#include <QSharedPointer>
#include <QDateTime>

class ArchiveTreeNode
{
public:
	
	ArchiveTreeNode(const QString& name, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime);
	ArchiveTreeNode() = default;
	~ArchiveTreeNode();

	void addChild(const QString& name, ArchiveTreeNode* childNode);

public:
	QString m_name;
	bool m_bIsDir = false;
	quint64 m_compressedSize = 0;
	quint64 m_originalSize = 0;
	QDateTime m_mtime;

	ArchiveTreeNode* m_parentNode = nullptr;
	QHash<QString, ArchiveTreeNode*> m_childNodes;
};
Q_DECLARE_METATYPE(const ArchiveTreeNode*)

class ArchiveTree
{
public:
	ArchiveTree();
	~ArchiveTree();

	void addEntry(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime);

	const ArchiveTreeNode* getRootNode() const;
	quint64 getFileCount() const;
	quint64 getFolderCount() const;

	void clear();


private:
	ArchiveTreeNode* m_rootNode = nullptr;
};