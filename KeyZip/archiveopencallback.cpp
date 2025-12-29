#include "archiveopencallback.h"

STDMETHODIMP ArchiveOpenCallBack::SetTotal(const UInt64* files, const UInt64* bytes)
{
	return S_OK;
}

STDMETHODIMP ArchiveOpenCallBack::SetCompleted(const UInt64* files, const UInt64* bytes)
{
	return S_OK;
}

STDMETHODIMP ArchiveOpenCallBack::CryptoGetTextPassword(BSTR* password)
{
	if (!password)
		return E_INVALIDARG;

	bool bCancel = false;
	m_password.clear();
	emit requirePassword(bCancel, m_password);

	if (bCancel || m_password.isEmpty())
		return E_ABORT;

	*password = SysAllocString(reinterpret_cast<const OLECHAR*>(m_password.utf16()));
	return S_OK;
}