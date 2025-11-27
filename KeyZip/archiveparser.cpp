#include "archiveparser.h"
#include "commonhelper.h"
#include "instreamwrapper.h"
#include "archiveopencallback.h"

#include <QLibrary>
#include <Common/MyInitGuid.h>
#include <7zip/Archive/IArchive.h>
#include <7zip/PropID.h>

// {23170F69-40C1-278A-1000-000110070000}
DEFINE_GUID(CLSID_CFormat7z,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000110010000}
DEFINE_GUID(CLSID_CFormatZip,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000110030000}
DEFINE_GUID(CLSID_CFormatRar,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);

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
	if (createObjectFunc(&CLSID_CFormat7z, &IID_IInArchive, (void**)&archive) != S_OK)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Failed to create 7z archive handler.");
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
	connect(openCallBackSpec, &ArchiveOpenCallBack::requirePassword, this, &ArchiveParser::requirePassword);
	
	HRESULT hr = archive->Open(inStream, nullptr, openCallBack);
	if (hr == E_ABORT)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveParser: Operation aborted by user (e.g., password cancelled).");
		emit parsingFailed();
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

		PROPVARIANT propPath;
		PROPVARIANT propIsDir;
		PROPVARIANT propSize;
		archive->GetProperty(i, kpidPath, &propPath);
		archive->GetProperty(i, kpidIsDir, &propIsDir);
		archive->GetProperty(i, kpidSize, &propSize);

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
	}

	
}


