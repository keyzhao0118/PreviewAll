#include "archivetree.h"
#include <QDir>

ArchiveTreeNode::ArchiveTreeNode(const QString& name, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime)
	: m_name(name)
	, m_bIsDir(bIsDir)
	, m_compressedSize(compressedSize)
	, m_originalSize(originalSize)
	, m_mtime(mtime)
{

}

void ArchiveTreeNode::addChild(const QString& name, const QSharedPointer<ArchiveTreeNode>& childNode)
{
	childNode->m_parentNode = this;
	m_childNodes.insert(name, childNode);
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

	auto parentNode = m_rootNode;
	for (int i = 0; i < pathParts.size(); ++i)
	{
		const QString& name = pathParts[i];
		const auto& childNodes = parentNode->m_childNodes;
		auto it = childNodes.find(name);
		if (it != childNodes.end())
		{
			parentNode = it.value();
			continue;
		}

		QSharedPointer<ArchiveTreeNode> newNode;
		if (i != pathParts.size() - 1)
			newNode.reset(new ArchiveTreeNode(name, true, 0, 0, QDateTime()));
		else
			newNode.reset(new ArchiveTreeNode(name, bIsDir, compressedSize, originalSize, mtime));
		
		parentNode->addChild(name, newNode);
		
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

	std::function<void(const QSharedPointer<ArchiveTreeNode>&, quint64&)> queryFileCount =
		[&](const QSharedPointer<ArchiveTreeNode>& node, quint64& count)
		{
			if (!node)
				return;
			for (auto it = node->m_childNodes.constBegin(); it != node->m_childNodes.constEnd(); ++it)
			{
				const auto& childNode = it.value();
				if (childNode->m_bIsDir)
					queryFileCount(childNode, count);
				else
					++count;
			}
		};

	queryFileCount(m_rootNode, fileCount);

	return fileCount;
}

quint64 ArchiveTree::getFolderCount() const
{
	quint64 folderCount = 0;

	std::function<void(const QSharedPointer<ArchiveTreeNode>&)> queryFolderCount =
		[&](const QSharedPointer<ArchiveTreeNode>& node)
		{
			if (!node)
				return;
			for (auto it = node->m_childNodes.constBegin(); it != node->m_childNodes.constEnd(); ++it)
			{
				const auto& childNode = it.value();
				if (childNode->m_bIsDir)
				{
					++folderCount;
					queryFolderCount(childNode);
				}
			}
		};

	queryFolderCount(m_rootNode);

	return folderCount;
}

void ArchiveTree::clear()
{
	m_rootNode = QSharedPointer<ArchiveTreeNode>::create();
}
