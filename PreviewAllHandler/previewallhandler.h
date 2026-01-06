#pragma once

#include <shobjidl.h>
#include <QString>

class CPreviewAllHandler :
	public IObjectWithSite,
	public IPreviewHandler,
	public IOleWindow,
	public IInitializeWithFile
{
public:
	CPreviewAllHandler();
	virtual ~CPreviewAllHandler();

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IObjectWithSite
	IFACEMETHODIMP SetSite(IUnknown* punkSite);
	IFACEMETHODIMP GetSite(REFIID riid, void** ppv);

	// IPreviewHandler
	IFACEMETHODIMP SetWindow(HWND hwnd, const RECT* prc);
	IFACEMETHODIMP SetFocus();
	IFACEMETHODIMP QueryFocus(HWND* phwnd);
	IFACEMETHODIMP TranslateAccelerator(MSG* pmsg);
	IFACEMETHODIMP SetRect(const RECT* prc);
	IFACEMETHODIMP DoPreview();
	IFACEMETHODIMP Unload();

	// IOleWindow
	IFACEMETHODIMP GetWindow(HWND* phwnd);
	IFACEMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

	// IInitializeWithFile
	IFACEMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD grfMode);

private:
	long m_cRef = 1;
	HWND m_hwndParent = nullptr;
	RECT m_rcParent = { 0 };
	HWND m_hwndPreview = nullptr;
	IUnknown* m_punkSite = nullptr;
	QString m_filePath;
};
