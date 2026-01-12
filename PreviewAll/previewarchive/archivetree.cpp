#include "archivetree.h"
#include <QDir>

ArchiveTreeNode::ArchiveTreeNode(const QString& name, bool bIsDir)
	: m_name(name)
	, m_bIsDir(bIsDir)
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

ArchiveTree::ArchiveTree(const QString& archivePath)
{
	QFileInfo archiveInfo(archivePath);
	m_rootNode = new ArchiveTreeNode(archiveInfo.fileName(), false);
}

ArchiveTree::~ArchiveTree()
{
	if (m_rootNode)
		delete m_rootNode;
	m_rootNode = nullptr;
}

void ArchiveTree::addEntry(const QString& path, bool bIsDir)
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
		ArchiveTreeNode* newNode = nullptr;
		newNode = new ArchiveTreeNode(name, bIsLastPart?bIsDir:true);

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
