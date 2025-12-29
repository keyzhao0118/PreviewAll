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

ArchiveTreeNode::~ArchiveTreeNode()
{
	qDeleteAll(m_childNodes);
	m_childNodes.clear();
}

void ArchiveTreeNode::addChild(const QString& name, ArchiveTreeNode* childNode)
{
	if (!childNode)
		return;

	childNode->m_parentNode = this;
	m_childNodes.insert(name, childNode);
}

//---------------------------------------

ArchiveTree::ArchiveTree(const QString& archiveName)
{
	m_rootNode = new ArchiveTreeNode();
	m_rootNode->m_name = archiveName;
}

ArchiveTree::~ArchiveTree()
{
	if (m_rootNode)
		delete m_rootNode;
	m_rootNode = nullptr;
}

void ArchiveTree::addEntry(const QString& path, bool bIsDir, quint64 compressedSize, quint64 originalSize, const QDateTime& mtime)
{
	if (path.isEmpty())
		return;

	QStringList pathParts = QDir::toNativeSeparators(path).split(QDir::separator(), Qt::SkipEmptyParts);
	if (pathParts.isEmpty())
		return;

	ArchiveTreeNode* parentNode = m_rootNode;
	for (int i = 0; i < pathParts.size(); ++i)
	{
		const QString& name = pathParts[i];
		QHash<QString, ArchiveTreeNode*>& childNodes = parentNode->m_childNodes;
		auto it = childNodes.find(name);
		if (it != childNodes.end())
		{
			parentNode = it.value();
			continue;
		}

		bool bIsLastPart = (i == pathParts.size() - 1);
		ArchiveTreeNode* newNode = bIsLastPart ?
			new ArchiveTreeNode(name, bIsDir, compressedSize, originalSize, mtime) :
			new ArchiveTreeNode(name, true, 0, 0, QDateTime());
		
		parentNode->addChild(name, newNode);
		parentNode = newNode;
	}
}

const ArchiveTreeNode* ArchiveTree::getRootNode() const
{
	return m_rootNode;
}

quint64 ArchiveTree::getFileCount() const
{
	std::function<void(const ArchiveTreeNode*, quint64&)> queryFileCount =
		[&](const ArchiveTreeNode* node, quint64& fileCount)
		{
			if (!node)
				return;
			for (auto it = node->m_childNodes.constBegin(); it != node->m_childNodes.constEnd(); ++it)
			{
				const ArchiveTreeNode* childNode = it.value();
				if (childNode->m_bIsDir)
					queryFileCount(childNode, fileCount);
				else
					++fileCount;
			}
		};

	quint64 fileCount = 0;
	queryFileCount(m_rootNode, fileCount);

	return fileCount;
}

quint64 ArchiveTree::getFolderCount() const
{
	std::function<void(const ArchiveTreeNode*, quint64&)> queryFolderCount =
		[&](const ArchiveTreeNode* node, quint64& folderCount)
		{
			if (!node)
				return;
			for (auto it = node->m_childNodes.constBegin(); it != node->m_childNodes.constEnd(); ++it)
			{
				const ArchiveTreeNode* childNode = it.value();
				if (childNode->m_bIsDir)
				{
					++folderCount;
					queryFolderCount(childNode, folderCount);
				}
			}
		};

	quint64 folderCount = 0;
	queryFolderCount(m_rootNode, folderCount);

	return folderCount;
}
