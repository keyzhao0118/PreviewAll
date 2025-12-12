#include "archiveextractor.h"
#include "commonhelper.h"
#include "instreamwrapper.h"
#include "archiveopencallback.h"
#include "archiveextractcallback.h"

#include <QLibrary>
#include <QFileInfo>

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

	QString suffix = QFileInfo(m_archivePath).suffix().toLower();
	GUID clsid = { 0 };
	if (suffix == "zip")
		clsid = CLSID_CFormatZip;
	else if (suffix == "7z")
		clsid = CLSID_CFormat7z;
	else
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Unsupported archive format: " + suffix);
		emit extractFailed();
		return;
	}

	QLibrary sevenZipLib("7zip.dll");
	if (!sevenZipLib.load())
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to load 7zip.dll.");
		emit extractFailed();
		return;
	}

	CreateObjectFunc createObjectFunc = (CreateObjectFunc)sevenZipLib.resolve("CreateObject");
	if (!createObjectFunc)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to get CreateObject function.");
		emit extractFailed();
		return;
	}

	CMyComPtr<IInArchive> archive;
	if (createObjectFunc(&clsid, &IID_IInArchive, (void**)&archive) != S_OK)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to create 7z archive handler.");
		emit extractFailed();
		return;
	}

	InStreamWrapper* inStreamSpec = new InStreamWrapper(m_archivePath);
	CMyComPtr<IInStream> inStream(inStreamSpec);
	if (!inStreamSpec->isOpen())
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to open archive file: " + m_archivePath);
		emit extractFailed();
		return;
	}

	ArchiveOpenCallBack* openCallBackSpec = new ArchiveOpenCallBack();
	CMyComPtr<IArchiveOpenCallback> openCallBack(openCallBackSpec);
	connect(openCallBackSpec, &ArchiveOpenCallBack::requirePassword, this, &ArchiveExtractor::requirePassword, Qt::DirectConnection);

	HRESULT hrOpen = archive->Open(inStream, nullptr, openCallBack);
	if (hrOpen == E_ABORT)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Operation aborted by user (e.g., password cancelled).");
		return;
	}

	if (hrOpen != S_OK)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractor: Failed to open archive. HRESULT: " + QString::number(hrOpen, 16));
		emit extractFailed();
		return;
	}

	ArchiveExtractCallBack* extractCallBackSpec = new ArchiveExtractCallBack();
	CMyComPtr<IArchiveExtractCallback> extractCallBack(extractCallBackSpec);
	extractCallBackSpec->Init(archive, m_destDirPath);
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
