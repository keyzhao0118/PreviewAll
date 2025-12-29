#include "archiveparser.h"
#include "archivetree.h"
#include "commonhelper.h"
#include "archiveopencallback.h"

#include <QDateTime>
#include <QFileInfo>
#include <7zip/Archive/IArchive.h>
#include <7zip/PropID.h>

ArchiveParser::ArchiveParser(QObject* parent /*= nullptr*/)
	: QThread(parent)
{
}

ArchiveParser::~ArchiveParser()
{
	requestInterruption();
	if (!(this->wait(1000)))
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to stop thread within 1 second.");
}

void ArchiveParser::parseArchive(const QString& archivePath)
{
	m_archivePath = archivePath;
	QString archiveName = QFileInfo(archivePath).fileName();
	m_archiveTree.reset(new ArchiveTree(archiveName));
	this->start();
}

const ArchiveTreeNode* ArchiveParser::getRootNode() const
{
	if (m_archiveTree)
		return m_archiveTree->getRootNode();
	return nullptr;
}

quint64 ArchiveParser::getFileCount() const
{
	if (m_archiveTree)
		return m_archiveTree->getFileCount();
	return 0;
}

quint64 ArchiveParser::getFolderCount() const
{
	if (m_archiveTree)
		return m_archiveTree->getFolderCount();
	return 0;
}

void ArchiveParser::run()
{
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();

	ArchiveOpenCallBack* openCallBackSpec = new ArchiveOpenCallBack();
	CMyComPtr<IArchiveOpenCallback> openCallBack(openCallBackSpec);
	connect(openCallBackSpec, &ArchiveOpenCallBack::requirePassword, this, &ArchiveParser::requirePassword, Qt::DirectConnection);

	CMyComPtr<IInArchive> archive;
	if (!CommonHelper::tryOpenArchive(m_archivePath, openCallBack, archive))
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to open archive: " + m_archivePath);
		emit parseFailed();
		return;
	}

	UInt32 itemCount = 0;
	archive->GetNumberOfItems(&itemCount);

	QElapsedTimer progressTimer;
	progressTimer.start();

	for (UInt32 i = 0; i < itemCount; ++i)
	{
		if (isInterruptionRequested())
		{
			CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Parsing interrupted.");
			return;
		}

		// 每500ms发送一次进度，确保首次和最后一次也发送
		if (i == 0 || i == itemCount - 1 || progressTimer.elapsed() >= 500)
		{
			emit updateProgress(i + 1, itemCount);
			progressTimer.restart();
		}

		PROPVARIANT propPath;		PropVariantInit(&propPath);
		PROPVARIANT propIsDir;		PropVariantInit(&propIsDir);
		PROPVARIANT propPackSize;	PropVariantInit(&propPackSize);
		PROPVARIANT propSize;		PropVariantInit(&propSize);
		PROPVARIANT propMTime;		PropVariantInit(&propMTime);

		archive->GetProperty(i, kpidPath, &propPath);
		archive->GetProperty(i, kpidIsDir, &propIsDir);
		archive->GetProperty(i, kpidPackSize, &propPackSize);
		archive->GetProperty(i, kpidSize, &propSize);
		archive->GetProperty(i, kpidMTime, &propMTime);

		QString path = QString::fromWCharArray(propPath.bstrVal);
		bool bIsDir = propIsDir.boolVal != VARIANT_FALSE;
		quint64 packedSize = propPackSize.uhVal.QuadPart;
		quint64 size = propSize.uhVal.QuadPart;
		QDateTime mtime = CommonHelper::fileTimeToDateTime(propMTime.filetime);
		if (m_archiveTree)
			m_archiveTree->addEntry(path, bIsDir, packedSize, size, mtime);

		PropVariantClear(&propPath);
		PropVariantClear(&propIsDir);
		PropVariantClear(&propPackSize);
		PropVariantClear(&propSize);
		PropVariantClear(&propMTime);
	}

	emit parseSucceed();
	CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Parsing completed in " + QString::number(elapsedTimer.elapsed()) + " ms.");
}



