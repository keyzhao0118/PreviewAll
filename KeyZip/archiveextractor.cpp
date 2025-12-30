#include "archiveextractor.h"
#include "commonhelper.h"
#include "archiveopencallback.h"
#include "archiveextractcallback.h"

extern "C" const GUID CLSID_CFormatZip;
extern "C" const GUID CLSID_CFormat7z;
typedef UINT32(WINAPI* CreateObjectFunc)(const GUID* clsID, const GUID* iid, void** outObject);

ArchiveExtractor::ArchiveExtractor(QObject* parent /*= nullptr*/)
	: QThread(parent)
{
}

ArchiveExtractor::~ArchiveExtractor()
{
	requestInterruption();
	if (!(this->wait(1000)))
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to stop thread within 1 second.");
}

void ArchiveExtractor::extractArchive(const QString& archivePath, const QString& destDirPath)
{
	m_archivePath = archivePath;
	m_destDirPath = destDirPath;
	this->start();
}

void ArchiveExtractor::run()
{
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();

	ArchiveOpenCallBack* openCallBackSpec = new ArchiveOpenCallBack();
	CMyComPtr<IArchiveOpenCallback> openCallBack(openCallBackSpec);
	connect(openCallBackSpec, &ArchiveOpenCallBack::requirePassword, this, &ArchiveExtractor::requirePassword, Qt::DirectConnection);

	CMyComPtr<IInArchive> archive;
	if (!CommonHelper::tryOpenArchive(m_archivePath, openCallBack, archive))
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to open archive: " + m_archivePath);
		emit extractFailed();
		return;
	}

	ArchiveExtractCallBack* extractCallBackSpec = new ArchiveExtractCallBack();
	CMyComPtr<IArchiveExtractCallback> extractCallBack(extractCallBackSpec);
	connect(extractCallBackSpec, &ArchiveExtractCallBack::requirePassword, this, &ArchiveExtractor::requirePassword, Qt::DirectConnection);
	connect(extractCallBackSpec, &ArchiveExtractCallBack::updateProgress, this, &ArchiveExtractor::onUpdateProgress, Qt::DirectConnection);

	extractCallBackSpec->init(archive, m_destDirPath, openCallBackSpec->getPassword());
	HRESULT hrExtract = archive->Extract(nullptr, static_cast<UInt32>(-1), false, extractCallBack);
	if (hrExtract != S_OK)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to extract archive. HRESULT: " + QString::number(hrExtract, 16));
		emit extractFailed();
		return;
	}

	emit extractSucceed();
	CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Extrcting completed in " + QString::number(elapsedTimer.elapsed()) + " ms.");
}

void ArchiveExtractor::onUpdateProgress(quint64 completed, quint64 total, bool& bIsInterruption)
{
	if (isInterruptionRequested())
	{
		bIsInterruption = true;
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Parsing interrupted.");
		return;
	}

	static qint64 lastEmitTime = QDateTime::currentMSecsSinceEpoch();
	quint64 currentTime = QDateTime::currentMSecsSinceEpoch();
	if (currentTime - lastEmitTime >= 500)
	{
		lastEmitTime = currentTime;
		emit updateProgress(completed, total);
	}
}
