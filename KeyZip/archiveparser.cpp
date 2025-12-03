#include "archiveparser.h"
#include "commonhelper.h"
#include "instreamwrapper.h"
#include "archiveopencallback.h"

#include <QLibrary>
#include <QFileInfo>
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
	QString suffix = QFileInfo(m_archivePath).suffix().toLower();
	GUID clsid = { 0 };
	if (suffix == "zip")
		clsid = CLSID_CFormatZip;
	else if(suffix == "7z")
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
	for (UInt32 i = 0; i < itemCount; ++i)
	{
		emit updateProgress(i + 1, itemCount);
		if (isInterruptionRequested())
		{
			CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Parsing interrupted.");
			return;
		}

		PROPVARIANT propPath;	PropVariantInit(&propPath);
		PROPVARIANT propIsDir;	PropVariantInit(&propIsDir);
		PROPVARIANT propSize;	PropVariantInit(&propSize);
		HRESULT hrPath = archive->GetProperty(i, kpidPath, &propPath);
		HRESULT hrIsDir = archive->GetProperty(i, kpidIsDir, &propIsDir);
		HRESULT hrSize = archive->GetProperty(i, kpidSize, &propSize);
		if (FAILED(hrPath) || FAILED(hrIsDir) || FAILED(hrSize))
		{
			CommonHelper::LogKeyZipDebugMsg("ArchiveParser: GetProperty failed at index " + QString::number(i)
				+ " hrPath=0x" + QString::number(hrPath, 16)
				+ " hrIsDir=0x" + QString::number(hrIsDir, 16)
				+ " hrSize=0x" + QString::number(hrSize, 16));
			PropVariantClear(&propPath);
			PropVariantClear(&propIsDir);
			PropVariantClear(&propSize);
			continue;
		}

		if (propPath.vt != VT_BSTR || propIsDir.vt != VT_BOOL || propSize.vt != VT_UI8)
		{
			// Invalid property types
			CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Invalid property types for item index " + QString::number(i) + ".");
			continue;
		}

		QString entryPath = QString::fromWCharArray(propPath.bstrVal);
		bool bIsDir = propIsDir.boolVal != VARIANT_FALSE;
		qint64 size = propSize.uhVal.QuadPart;

		emit entryFound(entryPath, bIsDir, size);
		PropVariantClear(&propPath);
		PropVariantClear(&propIsDir);
		PropVariantClear(&propSize);
	}

	
}



