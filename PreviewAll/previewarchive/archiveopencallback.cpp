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

	emit requestPassword(password);
	return S_OK;
}