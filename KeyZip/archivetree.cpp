#include "archivetree.h"
#include <QDir>

ArchiveTreeNode::ArchiveTreeNode(const QString& formatPath, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime)
	: m_formatPath(formatPath)
	, m_bIsDir(bIsDir)
	, m_compressedSize(compressedSize)
	, m_originalSize(originalSize)
	, m_mtime(mtime)
{
	m_name = formatPath.mid(formatPath.lastIndexOf(QDir::separator()) + 1);
}

void ArchiveTreeNode::setParent(ArchiveTreeNode* parentNode)
{
	m_parentNode = parentNode;
}

void ArchiveTreeNode::addChild(const QSharedPointer<ArchiveTreeNode>& childNode)
{
	childNode->m_parentNode = this;
	m_childNodes.append(childNode);
}

ArchiveTree::ArchiveTree()
{
	m_rootNode = QSharedPointer<ArchiveTreeNode>::create();
}

void ArchiveTree::addEntry(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime)
{
	if (path.isEmpty())
		return;

	QStringList pathParts = QDir::toNativeSeparators(path).split(QDir::separator(), Qt::SkipEmptyParts);
	if (pathParts.isEmpty())
		return;

	QString formatPath;
	auto parentNode = m_rootNode;
	for (int i = 0; i < pathParts.size(); ++i)
	{
		if (i == 0)
			formatPath = pathParts[i];
		else
			formatPath += QDir::separator() + pathParts[i];
		
		auto it = m_index.find(formatPath);
		if (it != m_index.end())
		{
			parentNode = it.value();
			continue;
		}

		QSharedPointer<ArchiveTreeNode> newNode;
		if (i != pathParts.size() - 1)
			newNode.reset(new ArchiveTreeNode(formatPath, true, 0, 0, QDateTime()));
		else
			newNode.reset(new ArchiveTreeNode(formatPath, bIsDir, compressedSize, originalSize, mtime));
		
		m_index.insert(formatPath, newNode);
		newNode->setParent(parentNode.data());
		parentNode->addChild(newNode);
		
		parentNode = newNode;
	}
}

const QSharedPointer<ArchiveTreeNode>& ArchiveTree::getRootNode() const
{
	return m_rootNode;
}

quint64 ArchiveTree::getFileCount() const
{
	quint64 fileCount = 0;
	for (auto it = m_index.constBegin(); it != m_index.constEnd(); ++it)
	{
		if (!it.value()->m_bIsDir)
			++fileCount;
	}
	return fileCount;
}

quint64 ArchiveTree::getFolderCount() const
{
	quint64 folderCount = 0;
	for (auto it = m_index.constBegin(); it != m_index.constEnd(); ++it)
	{
		if (it.value()->m_bIsDir)
			++folderCount;
	}
	return folderCount;
}

void ArchiveTree::clear()
{
	m_index.clear();
	m_rootNode = QSharedPointer<ArchiveTreeNode>::create();
}
