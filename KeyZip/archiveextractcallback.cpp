#include "archiveextractcallback.h"
#include "outstreamwrapper.h"
#include "commonhelper.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

void ArchiveExtractCallBack::init(IInArchive* archive, const QString& destDirPath)
{
	m_archive = archive;
	m_destDirPath = destDirPath;
}

STDMETHODIMP ArchiveExtractCallBack::SetTotal(const UInt64 size)
{
	m_totalSize = static_cast<quint64>(size);
	qDebug() << "Total size to extract:" << m_totalSize;
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::SetCompleted(const UInt64* completedSize)
{
	if (completedSize)
	{
		m_completedSize = static_cast<quint64>(*completedSize);
		qDebug() << "Completed size:" << m_completedSize;
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
	HRESULT hrPath = m_archive->GetProperty(index, kpidPath, &propPath);
	HRESULT hrIsDir = m_archive->GetProperty(index, kpidIsDir, &propIsDir);
	if (FAILED(hrPath) || propPath.vt != VT_BSTR ||
		FAILED(hrIsDir) || propIsDir.vt != VT_BOOL)
	{
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractCallBack: GetProperty failed at index " + QString::number(index));
		PropVariantClear(&propPath);
		PropVariantClear(&propIsDir);
		return E_FAIL;
	}

	QString path = QString::fromWCharArray(propPath.bstrVal);
	bool bIsDir = propIsDir.boolVal != VARIANT_FALSE;

	PropVariantClear(&propPath);
	PropVariantClear(&propIsDir);

	QString fullPath = QDir::cleanPath(m_destDirPath + QDir::separator() + path);
	if (bIsDir)
	{
		if(!QDir().mkpath(fullPath))
			return E_FAIL;
		else
			return S_OK;
	}

	const QString parentDir = QFileInfo(fullPath).absolutePath();
	if (!QDir().mkpath(parentDir))
		return E_FAIL;

	OutStreamWrapper* outStreamSpec = new OutStreamWrapper(fullPath);
	CMyComPtr<ISequentialOutStream> sequentialOutStream(outStreamSpec);
	if (!outStreamSpec->isOpen())
		return E_FAIL;

	*outStream = sequentialOutStream.Detach();
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