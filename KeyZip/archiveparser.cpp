#include "archiveparser.h"
#include "commonhelper.h"
#include "instreamwrapper.h"
#include "archiveopencallback.h"

#include <QLibrary>
#include <QFileInfo>
#include <QDateTime>
#include <7zip/Archive/IArchive.h>
#include <7zip/PropID.h>

extern "C" const GUID CLSID_CFormatZip;
extern "C" const GUID CLSID_CFormat7z;

typedef UINT32(WINAPI* CreateObjectFunc)(const GUID* clsID, const GUID* iid, void** outObject);

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
	this->start();
}

void ArchiveParser::run()
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
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Unsupported archive format: " + suffix);
		emit parsingFailed();
		return;
	}

	InStreamWrapper* inStreamSpec = new InStreamWrapper(m_archivePath);
	CMyComPtr<IInStream> inStream(inStreamSpec);
	if (!inStreamSpec->isOpen())
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to open archive file: " + m_archivePath);
		emit parsingFailed();
		return;
	}

	ArchiveOpenCallBack* openCallBackSpec = new ArchiveOpenCallBack();
	CMyComPtr<IArchiveOpenCallback> openCallBack(openCallBackSpec);
	connect(openCallBackSpec, &ArchiveOpenCallBack::requirePassword, this, &ArchiveParser::requirePassword, Qt::DirectConnection);

	QLibrary sevenZipLib("7zip.dll");
	if (!sevenZipLib.load())
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to load 7zip.dll.");
		emit parsingFailed();
		return;
	}

	CreateObjectFunc createObjectFunc = (CreateObjectFunc)sevenZipLib.resolve("CreateObject");
	if (!createObjectFunc)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to get CreateObject function.");
		emit parsingFailed();
		return;
	}

	CMyComPtr<IInArchive> archive;
	if (createObjectFunc(&clsid, &IID_IInArchive, (void**)&archive) != S_OK)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to create 7z archive handler.");
		emit parsingFailed();
		return;
	}

	HRESULT hr = archive->Open(inStream, nullptr, openCallBack);
	if (hr == E_ABORT)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Operation aborted by user (e.g., password cancelled).");
		return;
	}

	if (hr != S_OK)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to open archive. HRESULT: " + QString::number(hr, 16));
		emit parsingFailed();
		return;
	}

	UInt32 itemCount = 0;
	archive->GetNumberOfItems(&itemCount);

	QElapsedTimer progressTimer;
	progressTimer.start();

	for (UInt32 i = 0; i < itemCount; ++i)
	{
		// 每500ms发送一次进度，确保首次和最后一次也发送
		if (i == 0 || i == itemCount - 1 || progressTimer.elapsed() >= 500)
		{
			emit updateProgress(i + 1, itemCount);
			progressTimer.restart();
		}

		if (isInterruptionRequested())
		{
			CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Parsing interrupted.");
			return;
		}

		PROPVARIANT propPath;		PropVariantInit(&propPath);
		PROPVARIANT propIsDir;		PropVariantInit(&propIsDir);
		PROPVARIANT propPackSize;	PropVariantInit(&propPackSize);
		PROPVARIANT propSize;		PropVariantInit(&propSize);
		PROPVARIANT propMTime;		PropVariantInit(&propMTime);

		HRESULT hrPath = archive->GetProperty(i, kpidPath, &propPath);
		HRESULT hrIsDir = archive->GetProperty(i, kpidIsDir, &propIsDir);
		HRESULT hrPackSize = archive->GetProperty(i, kpidPackSize, &propPackSize);
		HRESULT hrSize = archive->GetProperty(i, kpidSize, &propSize);
		HRESULT hrMTime = archive->GetProperty(i, kpidMTime, &propMTime);

		if (FAILED(hrPath) || propPath.vt != VT_BSTR ||
			FAILED(hrIsDir) || propIsDir.vt != VT_BOOL ||
			FAILED(hrPackSize) || propPackSize.vt != VT_UI8 ||
			FAILED(hrSize) || propSize.vt != VT_UI8 ||
			FAILED(hrMTime) || propMTime.vt != VT_FILETIME)
		{
			CommonHelper::LogKeyZipDebugMsg("ArchiveParser: GetProperty failed at index " + QString::number(i)
				+ " hrPath=0x" + QString::number(hrPath, 16)
				+ " hrIsDir=0x" + QString::number(hrIsDir, 16)
				+ " hrPackSize=0x" + QString::number(hrPackSize, 16)
				+ " hrSize=0x" + QString::number(hrSize, 16)
				+ " hrMTime=0x" + QString::number(hrMTime, 16));

			PropVariantClear(&propPath);
			PropVariantClear(&propIsDir);
			PropVariantClear(&propPackSize);
			PropVariantClear(&propSize);
			PropVariantClear(&propMTime);

			return;
		}

		QString path = QString::fromWCharArray(propPath.bstrVal);
		bool bIsDir = propIsDir.boolVal != VARIANT_FALSE;
		qint64 packedSize = propPackSize.uhVal.QuadPart;
		qint64 size = propSize.uhVal.QuadPart;
		QDateTime mtime = CommonHelper::fileTimeToDateTime(propMTime.filetime);
		emit entryFound(path, bIsDir, packedSize, size, mtime);

		PropVariantClear(&propPath);
		PropVariantClear(&propIsDir);
		PropVariantClear(&propPackSize);
		PropVariantClear(&propSize);
		PropVariantClear(&propMTime);
	}

	emit parsingSucceed();
	CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Parsing completed in " + QString::number(elapsedTimer.elapsed()) + " ms.");
}



