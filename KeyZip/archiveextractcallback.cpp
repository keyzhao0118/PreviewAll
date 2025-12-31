#include "archiveextractcallback.h"
#include "outstreamwrapper.h"
#include "commonhelper.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

void ArchiveExtractCallBack::init(IInArchive* archive, const QString& entryPath, const QString& destDirPath, const QString& password)
{
	m_archive = archive;
	m_entryPath = QDir::toNativeSeparators(entryPath);
	m_destDirPath = QDir::toNativeSeparators(destDirPath);
	m_password = password;
}

STDMETHODIMP ArchiveExtractCallBack::SetTotal(const UInt64 size)
{
	m_totalSize = static_cast<quint64>(size);
	CommonHelper::LogKeyZipDebugMsg("ArchiveExtractCallBack: Total size to extract: " + QString::number(m_totalSize));
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::SetCompleted(const UInt64* completedSize)
{
	if (completedSize)
	{
		m_completedSize = static_cast<quint64>(*completedSize);
		CommonHelper::LogKeyZipDebugMsg("ArchiveExtractCallBack: Completed size: " + QString::number(m_completedSize));
		bool bIsInterruption = false;
		emit updateProgress(m_completedSize, m_totalSize, bIsInterruption);
		if (bIsInterruption)
			return E_ABORT;
	}
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode)
{
	*outStream = nullptr;
	m_currentIndex = index;
	m_bSkipCurrent = false;

	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;

	PROPVARIANT propPath;	PropVariantInit(&propPath);
	PROPVARIANT propIsDir;	PropVariantInit(&propIsDir);
	m_archive->GetProperty(index, kpidPath, &propPath);
	m_archive->GetProperty(index, kpidIsDir, &propIsDir);

	QString path = QDir::toNativeSeparators(QString::fromWCharArray(propPath.bstrVal));
	if (m_entryPath.isEmpty())
	{
		m_currentFullPath = m_destDirPath + QDir::separator() + path;
	}
	else
	{
		if (!path.startsWith(m_entryPath))
		{
			m_bSkipCurrent = true;
			return S_OK;
		}

		QString relativePath = path.mid(m_entryPath.size());
		if (!relativePath.isEmpty() && !relativePath.startsWith(QDir::separator()))
		{
			m_bSkipCurrent = true;
			return S_OK;
		}

		int pos = m_entryPath.lastIndexOf(QDir::separator()) + 1;
		m_currentFullPath = m_destDirPath + QDir::separator() + m_entryPath.mid(pos);
	}
	m_currentIsDir = propIsDir.boolVal != VARIANT_FALSE;
	
	PropVariantClear(&propPath);
	PropVariantClear(&propIsDir);

	if (m_currentIsDir)
	{
		if (!QDir().mkpath(m_currentFullPath))
			m_bSkipCurrent = true;
		return S_OK;
	}

	const QString parentDir = QFileInfo(m_currentFullPath).absolutePath();
	if (!QDir().mkpath(parentDir))
	{
		m_bSkipCurrent = true;
		return S_OK;
	}

	OutStreamWrapper* outStreamSpec = new OutStreamWrapper(m_currentFullPath);
	CMyComPtr<ISequentialOutStream> sequentialOutStream(outStreamSpec);
	if (!outStreamSpec->isOpen())
	{
		m_bSkipCurrent = true;
		return S_OK;
	}

	*outStream = sequentialOutStream.Detach();
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::PrepareOperation(Int32 askExtractMode)
{
	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::SetOperationResult(Int32 opRes)
{
	// 恢复文件/目录的时间戳与属性（参照 7-Zip 示例做法）
	if (opRes != NArchive::NExtract::NOperationResult::kOK || m_bSkipCurrent)
		return S_OK;

	// 获取时间与属性
	PROPVARIANT propCTime;	PropVariantInit(&propCTime);
	PROPVARIANT propATime;	PropVariantInit(&propATime);
	PROPVARIANT propMTime;	PropVariantInit(&propMTime);
	PROPVARIANT propAttrib;	PropVariantInit(&propAttrib);

	m_archive->GetProperty(m_currentIndex, kpidCTime, &propCTime);
	m_archive->GetProperty(m_currentIndex, kpidATime, &propATime);
	m_archive->GetProperty(m_currentIndex, kpidMTime, &propMTime);
	m_archive->GetProperty(m_currentIndex, kpidAttrib, &propAttrib);

	// 打开句柄用于设置时间戳
	DWORD flags = m_currentIsDir ? FILE_FLAG_BACKUP_SEMANTICS : 0;
	HANDLE h = CreateFileW(reinterpret_cast<LPCWSTR>(m_currentFullPath.utf16()),
		GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, flags, nullptr);
	if (h != INVALID_HANDLE_VALUE)
	{
		const FILETIME* pCT = (propCTime.vt == VT_FILETIME) ? &propCTime.filetime : nullptr;
		const FILETIME* pAT = (propATime.vt == VT_FILETIME) ? &propATime.filetime : nullptr;
		const FILETIME* pMT = (propMTime.vt == VT_FILETIME) ? &propMTime.filetime : nullptr;
		SetFileTime(h, pCT, pAT, pMT);
		CloseHandle(h);
	}

	if (propAttrib.vt == VT_UI4)
	{
		DWORD attrib = propAttrib.ulVal;
		SetFileAttributesW(reinterpret_cast<LPCWSTR>(m_currentFullPath.utf16()), attrib);
	}

	PropVariantClear(&propCTime);
	PropVariantClear(&propATime);
	PropVariantClear(&propMTime);
	PropVariantClear(&propAttrib);

	return S_OK;
}

STDMETHODIMP ArchiveExtractCallBack::CryptoGetTextPassword(BSTR* password)
{
	if (!password)
		return E_INVALIDARG;

	if (m_password.isEmpty())
	{
		bool bCancel = false;
		emit requirePassword(bCancel, m_password);

		if (bCancel || m_password.isEmpty())
			return E_ABORT;
	}

	*password = SysAllocString(reinterpret_cast<const OLECHAR*>(m_password.utf16()));
	return S_OK;
}