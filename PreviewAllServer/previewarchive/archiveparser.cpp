#include "archiveparser.h"
#include "archivetree.h"
#include "archiveopencallback.h"
#include "instreamwrapper.h"

#include <7zip/PropID.h>
#include <QElapsedTimer>
#include <QLibrary>
#include <QDebug>

namespace
{

	extern "C" const GUID CLSID_CFormatZip;
	extern "C" const GUID CLSID_CFormat7z;
	extern "C" const GUID CLSID_CFormatRar;
	extern "C" const GUID CLSID_CFormatRar5;
	typedef UINT32(WINAPI* CreateObjectFunc)(const GUID* clsID, const GUID* iid, void** outObject);

	bool isZip(const uint8_t* p)
	{
		return p[0] == 0x50 && p[1] == 0x4B &&
			(p[2] == 0x03 || p[2] == 0x05 || p[2] == 0x07) &&
			(p[3] == 0x04 || p[3] == 0x06 || p[3] == 0x08);
	}

	bool is7z(const uint8_t* p)
	{
		static const uint8_t sig[6] = {
			0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C
		};
		return memcmp(p, sig, 6) == 0;
	}

	bool isRar4(const uint8_t* p)
	{
		static const uint8_t sig[7] = {
			0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00
		};
		return memcmp(p, sig, 7) == 0;
	}

	bool isRar5(const uint8_t* p)
	{
		static const uint8_t sig[8] = {
			0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00
		};
		return memcmp(p, sig, 8) == 0;
	}

	CLSID getAdapatedCLSID(const QString& archivePath)
	{
		QFile file(archivePath);
		if (!file.open(QIODevice::ReadOnly))
			return CLSID_NULL;

		uint8_t header[32] = { 0 };
		const qint64 readSize = file.read(reinterpret_cast<char*>(header), sizeof(header));
		file.close();

		if (readSize < 8)
			return CLSID_NULL;

		if (is7z(header))
			return CLSID_CFormat7z;

		if (isRar5(header))
			return CLSID_CFormatRar5;

		if (isRar4(header))
			return CLSID_CFormatRar;

		if (isZip(header))
			return CLSID_CFormatZip;

		return CLSID_NULL;
	}

}

ArchiveParser::ArchiveParser(const QString& archivePath, QObject* parent /*= nullptr*/)
	: QObject(parent)
	, m_archivePath(archivePath)
{
	m_archiveTree.reset(new ArchiveTree(archivePath));
}

ArchiveParser::~ArchiveParser()
{
}

void ArchiveParser::stopParse()
{
	QMutexLocker locker(&m_mutex);
	m_bStopParse = true;
	m_waitCondition.wakeOne();
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

void ArchiveParser::parseArchive()
{
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();

	ArchiveOpenCallBack* openCallBackSpec = new ArchiveOpenCallBack();
	CMyComPtr<IArchiveOpenCallback> openCallBack(openCallBackSpec);

	CMyComPtr<IInArchive> archive;
	HRESULT hrOpen = tryOpenArchive(m_archivePath, openCallBack, archive);
	if (hrOpen == E_ABORT)
	{
		qDebug() << "ArchiveParser: Abort parse encrypt archive.";
		emit encryptArchive();
		return;
	}
	if (hrOpen != S_OK)
	{
		qDebug() << "ArchiveParser: Failed to open archive.";
		emit parseFailed();
		return;
	}

	if (!processArchive(archive))
	{
		qDebug() << "ArchiveParser: Parsing stopped by user.";
		return;
	}

	emit parseSucceed();
	qDebug() << "ArchiveParser: Parsing completed in " + QString::number(elapsedTimer.elapsed()) + " ms.";
}

HRESULT ArchiveParser::tryOpenArchive(const QString& archivePath, IArchiveOpenCallback* openCallback, CMyComPtr<IInArchive>& outInArchive)
{
	outInArchive.Release();

	QLibrary sevenZipLib("7zip.dll");
	if (!sevenZipLib.load())
		return S_FALSE;

	CreateObjectFunc createObjectFunc = (CreateObjectFunc)sevenZipLib.resolve("CreateObject");
	if (!createObjectFunc)
		return S_FALSE;

	const GUID clsid = getAdapatedCLSID(archivePath);
	if (clsid == CLSID_NULL)
		return S_FALSE;

	CMyComPtr<IInArchive> archive;
	if (createObjectFunc(&clsid, &IID_IInArchive, (void**)&archive) != S_OK)
		return S_FALSE;

	InStreamWrapper* inStreamSpec = new InStreamWrapper(archivePath);
	CMyComPtr<IInStream> inStream(inStreamSpec);
	if (!inStreamSpec->isOpen())
		return S_FALSE;

	HRESULT hr = archive->Open(inStream, nullptr, openCallback);
	if (hr == S_OK)
		outInArchive = archive;

	return hr;
}

bool ArchiveParser::processArchive(CMyComPtr<IInArchive> archive)
{
	UInt32 itemCount = 0;
	archive->GetNumberOfItems(&itemCount);

	QElapsedTimer progressTimer;
	progressTimer.start();

	for (UInt32 i = 0; i < itemCount; ++i)
	{
		if (checkStopParse())
			return false;

		if (progressTimer.elapsed() >= 500)
		{
			emit updateProgress(i, itemCount);
			progressTimer.restart();
		}

		PROPVARIANT propPath;		PropVariantInit(&propPath);
		PROPVARIANT propIsDir;		PropVariantInit(&propIsDir);

		archive->GetProperty(i, kpidPath, &propPath);
		archive->GetProperty(i, kpidIsDir, &propIsDir);

		QString path = QString::fromWCharArray(propPath.bstrVal);
		bool bIsDir = propIsDir.boolVal != VARIANT_FALSE;

		PropVariantClear(&propPath);
		PropVariantClear(&propIsDir);

		if (m_archiveTree)
			m_archiveTree->addEntry(path, bIsDir);
	}

	return true;
}

bool ArchiveParser::checkStopParse()
{
	QMutexLocker locker(&m_mutex);
	return m_bStopParse;
}
