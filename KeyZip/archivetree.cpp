#include "archivetree.h"
#include <QDir>
#include <QStack>

ArchiveTreeNode::ArchiveTreeNode(const QString& formatPath, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime)
	: m_formatPath(formatPath)
	, m_name(QFileInfo(formatPath).fileName())
	, m_bIsDir(bIsDir)
	, m_compressedSize(compressedSize)
	, m_originalSize(originalSize)
	, m_mtime(mtime)
{

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

	QStack<QString> pathStack;
	QString formatPath = pathParts.first();
	pathStack.push(formatPath);
	for (int i = 1; i < pathParts.size(); ++i)
	{
		formatPath += QDir::separator() + pathParts[i];
		pathStack.push(formatPath);
	}

	QString curFormatPath = pathStack.pop();
	if (m_index.contains(curFormatPath))
	{
		QSharedPointer<ArchiveTreeNode> existingNode = m_index.value(curFormatPath);
		existingNode->m_bIsDir = bIsDir;
		existingNode->m_compressedSize = compressedSize;
		existingNode->m_originalSize = originalSize;
		existingNode->m_mtime = mtime;
		return;
	}
	QSharedPointer<ArchiveTreeNode> curNode = QSharedPointer<ArchiveTreeNode>::create(curFormatPath, bIsDir, compressedSize, originalSize, mtime);
	m_index.insert(curFormatPath, curNode);

	while (!pathStack.isEmpty())
	{
		curFormatPath = pathStack.pop();
		if (m_index.contains(curFormatPath))
		{
			QSharedPointer<ArchiveTreeNode> parentNode = m_index.value(curFormatPath);
			curNode->setParent(parentNode.data());
			parentNode->addChild(curNode);
			return;
		}
		else
		{
			QSharedPointer<ArchiveTreeNode> parentNode = QSharedPointer<ArchiveTreeNode>::create(curFormatPath, true, 0, 0, QDateTime());
			m_index.insert(curFormatPath, parentNode);
			curNode->setParent(parentNode.data());
			parentNode->addChild(curNode);
			curNode = parentNode;
		}
	}

	curNode->setParent(m_rootNode.data());
	m_rootNode->addChild(curNode);
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
