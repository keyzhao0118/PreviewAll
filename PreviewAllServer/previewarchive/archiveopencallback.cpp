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
	return E_ABORT;
}