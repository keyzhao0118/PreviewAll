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
	Unload();
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
	if (!hwnd || !prc)
		return E_INVALIDARG;

	m_hwndParent = hwnd;
	m_rcParent = *prc;
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
	if (!prc)
		return E_INVALIDARG;

	m_rcParent = *prc;
	resizePreview();
	return S_OK;
}

HRESULT CPreviewAllHandler::DoPreview()
{
	m_hwndPreview = PreviewAllRequester::sendCreateCmd(m_hwndParent, m_filePath);
	if (!m_hwndPreview)
		return E_FAIL;

	resizePreview();
	return S_OK;
}

void CPreviewAllHandler::resizePreview()
{
	if (!m_hwndPreview || !m_hwndParent || !IsWindow(m_hwndPreview))
		return;

	// Explorer calls SetRect repeatedly while dragging the preview splitter.
	// Resize the embedded child before returning so its edge tracks the host.
	// Only resize a preview still attached to the parent supplied by SetWindow.
	if (GetParent(m_hwndPreview) != m_hwndParent)
		return;

	// The preview host thread can retain the primary monitor's DPI context.
	// SetRect already supplies physical host pixels; use a per-monitor context
	// so SetWindowPos does not virtualize them when Explorer is on another screen.
	const DPI_AWARENESS_CONTEXT previousDpiContext =
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	SetWindowPos(m_hwndPreview, nullptr, m_rcParent.left, m_rcParent.top,
		qMax(0L, m_rcParent.right - m_rcParent.left),
		qMax(0L, m_rcParent.bottom - m_rcParent.top),
		SWP_NOZORDER | SWP_NOACTIVATE);
	if (previousDpiContext)
		SetThreadDpiAwarenessContext(previousDpiContext);
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
	if (!phwnd)
		return E_INVALIDARG;

	*phwnd = m_hwndPreview;
	return m_hwndPreview ? S_OK : E_FAIL;
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
