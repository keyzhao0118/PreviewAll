#include "previewallhandler.h"
#include "previewallrequester.h"
#include <shlwapi.h>

namespace
{

	template <class T> void SafeRelease(T** ppT)
	{
		if (*ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}

}

CPreviewAllHandler::CPreviewAllHandler()
{
}

CPreviewAllHandler::~CPreviewAllHandler()
{
	if (m_hwndPreview)
	{
		DestroyWindow(m_hwndPreview);
	}

	SafeRelease(&m_punkSite);
}

// IUnknown
IFACEMETHODIMP CPreviewAllHandler::QueryInterface(REFIID riid, void** ppv)
{
	*ppv = NULL;
	static const QITAB qit[] =
	{
		QITABENT(CPreviewAllHandler, IObjectWithSite),
		QITABENT(CPreviewAllHandler, IOleWindow),
		QITABENT(CPreviewAllHandler, IInitializeWithFile),
		QITABENT(CPreviewAllHandler, IPreviewHandler),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CPreviewAllHandler::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) CPreviewAllHandler::Release()
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if (!cRef)
		delete this;
	return cRef;
}

// IObjectWithSite
HRESULT CPreviewAllHandler::SetSite(IUnknown* punkSite)
{
	SafeRelease(&m_punkSite);
	return punkSite ? punkSite->QueryInterface(&m_punkSite) : S_OK;
}

HRESULT CPreviewAllHandler::GetSite(REFIID riid, void** ppv)
{
	*ppv = NULL;
	return m_punkSite ? m_punkSite->QueryInterface(riid, ppv) : E_FAIL;
}

// IPreviewHandler
HRESULT CPreviewAllHandler::SetWindow(HWND hwnd, const RECT* prc)
{
	if (hwnd && prc)
	{
		m_hwndParent = hwnd;
		m_rcParent = *prc;
	}
	return S_OK;
}

HRESULT CPreviewAllHandler::SetFocus()
{
	HRESULT hr = S_FALSE;
	if (m_hwndPreview)
	{
		::SetFocus(m_hwndPreview);
		hr = S_OK;
	}
	return hr;
}

HRESULT CPreviewAllHandler::QueryFocus(HWND* phwnd)
{
	HRESULT hr = E_INVALIDARG;
	if (phwnd)
	{
		*phwnd = ::GetFocus();
		if (*phwnd)
			hr = S_OK;
		else
			hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}

HRESULT CPreviewAllHandler::TranslateAccelerator(MSG* pmsg)
{
	HRESULT hr = S_FALSE;
	IPreviewHandlerFrame* pFrame = NULL;
	if (m_punkSite && SUCCEEDED(m_punkSite->QueryInterface(&pFrame)))
	{
		hr = pFrame->TranslateAccelerator(pmsg);
		SafeRelease(&pFrame);
	}
	return hr;
}

HRESULT CPreviewAllHandler::SetRect(const RECT* prc)
{
	if (prc && m_hwndPreview)
	{
		m_rcParent = *prc;
		PreviewAllRequester::postResizeCmd(m_hwndPreview, m_rcParent.right - m_rcParent.left, m_rcParent.bottom - m_rcParent.top);
		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT CPreviewAllHandler::DoPreview()
{
	m_hwndPreview = PreviewAllRequester::sendCreateCmd(m_hwndParent, m_filePath);
	if (!m_hwndPreview)
		return E_FAIL;

	PreviewAllRequester::postResizeCmd(m_hwndPreview, m_rcParent.right - m_rcParent.left, m_rcParent.bottom - m_rcParent.top);
	return S_OK;
}

HRESULT CPreviewAllHandler::Unload()
{
	if (m_hwndPreview)
	{
		PreviewAllRequester::postCloseCmd(m_hwndPreview);
		m_hwndPreview = nullptr;
	}
	return S_OK;
}

// IOleWindow methods
HRESULT CPreviewAllHandler::GetWindow(HWND* phwnd)
{
	HRESULT hr = E_INVALIDARG;
	if (phwnd)
	{
		*phwnd = m_hwndParent;
		hr = S_OK;
	}
	return hr;
}

HRESULT CPreviewAllHandler::ContextSensitiveHelp(BOOL)
{
	return E_NOTIMPL;
}

// IInitializeWithFile
HRESULT CPreviewAllHandler::Initialize(LPCWSTR pszFilePath, DWORD grfMode)
{
	if (!pszFilePath)
		return E_INVALIDARG;

	m_filePath = QString::fromStdWString(pszFilePath);
	return S_OK;
}