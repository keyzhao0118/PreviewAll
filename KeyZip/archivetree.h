#pragma once

#include <QSharedPointer>
#include <QDateTime>

class ArchiveTreeNode
{
public:
	
	ArchiveTreeNode(const QString& formatPath, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime);
	ArchiveTreeNode() = default;
	~ArchiveTreeNode() = default;

	void setParent(ArchiveTreeNode* parentNode);
	void addChild(const QSharedPointer<ArchiveTreeNode>& childNode);

public:
	QString m_formatPath;
	QString m_name;
	bool m_bIsDir = false;
	quint64 m_compressedSize = 0;
	quint64 m_originalSize = 0;
	QDateTime m_mtime;

	ArchiveTreeNode* m_parentNode = nullptr;
	QList<QSharedPointer<ArchiveTreeNode>> m_childNodes;
};
Q_DECLARE_METATYPE(QSharedPointer<ArchiveTreeNode>)

class ArchiveTree
{
public:
	ArchiveTree();
	~ArchiveTree() = default;

	void addEntry(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime);

	const QSharedPointer<ArchiveTreeNode>& getRootNode() const;
	quint64 getFileCount() const;
	quint64 getFolderCount() const;

	void clear();


private:
	QSharedPointer<ArchiveTreeNode> m_rootNode;
	QHash<QString, QSharedPointer<ArchiveTreeNode>> m_index;
};