#include "archiveextractcallback.h"
#include "outstreamwrapper.h"
#include "commonhelper.h"
#include <QDir>

void ArchiveExtractCallBack::Init(IInArchive* archive, const QString& destDirPath)
{
	m_archive = archive;
	m_destDirPath = destDirPath;
}

STDMETHODIMP ArchiveExtractCallBack::SetTotal(const UInt64 size)
{
	m_totalSize = static_cast<quint64>(size);
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::SetCompleted(const UInt64* completedSize)
{
	if (completedSize)
	{
		m_completedSize = static_cast<quint64>(*completedSize);
		emit updateProgress(m_completedSize, m_totalSize);
	}
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
{
	*outStream = nullptr;

	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;

	PROPVARIANT propPath;		PropVariantInit(&propPath);
	PROPVARIANT propIsDir;		PropVariantInit(&propIsDir);
	PROPVARIANT propSize;		PropVariantInit(&propSize);
	PROPVARIANT propMTime;		PropVariantInit(&propMTime);

	HRESULT hrPath = m_archive->GetProperty(index, kpidPath, &propPath);
	HRESULT hrIsDir = m_archive->GetProperty(index, kpidIsDir, &propIsDir);
	HRESULT hrSize = m_archive->GetProperty(index, kpidSize, &propSize);
	HRESULT hrMTime = m_archive->GetProperty(index, kpidMTime, &propMTime);
	if (FAILED(hrPath) || propPath.vt != VT_BSTR ||
		FAILED(hrIsDir) || propIsDir.vt != VT_BOOL ||
		FAILED(hrSize) || propSize.vt != VT_UI8 ||
		FAILED(hrMTime) || propMTime.vt != VT_FILETIME)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractCallBack: GetProperty failed at index " + QString::number(index));
		PropVariantClear(&propPath);
		PropVariantClear(&propIsDir);
		PropVariantClear(&propSize);
		PropVariantClear(&propMTime);
		return E_FAIL;
	}

	QString path = QString::fromWCharArray(propPath.bstrVal);
	bool bIsDir = propIsDir.boolVal != VARIANT_FALSE;
	quint64 size = propSize.uhVal.QuadPart;
	QDateTime mtime = CommonHelper::fileTimeToDateTime(propMTime.filetime);

	PropVariantClear(&propPath);
	PropVariantClear(&propIsDir);
	PropVariantClear(&propSize);
	PropVariantClear(&propMTime);

	QString fullPath = QDir::cleanPath(m_destDirPath + QDir::separator() + path);
	if (bIsDir)
	{
		
	}

	
	OutStreamWrapper* outStreamSpec = new OutStreamWrapper(fullPath);

	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::PrepareOperation(Int32 askExtractMode)
{
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::SetOperationResult(Int32 opRes)
{
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::CryptoGetTextPassword(BSTR* password)
{
	if (!password)
		return E_INVALIDARG;

	bool bCancel = false;
	QString pass;
	emit requirePassword(bCancel, pass);

	if (bCancel || pass.isEmpty())
		return E_ABORT;

	*password = SysAllocString(reinterpret_cast<const OLECHAR*>(pass.utf16()));
	return S_OK;
}