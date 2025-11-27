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
	QString pass;
	emit requirePassword(bCancel, pass);

	if (bCancel || pass.isEmpty())
		return E_ABORT;

	*password = SysAllocString(reinterpret_cast<const OLECHAR*>(pass.utf16()));
	return S_OK;
}