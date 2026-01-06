#include "previewallhandler.h"
#include "previewallrequester.h"
#include <shlwapi.h>

namespace
{

	typedef HRESULT(*PFNCREATEINSTANCE)(REFIID riid, void** ppvObject);
	struct CLASS_OBJECT_INIT
	{
		const CLSID* pClsid;
		PFNCREATEINSTANCE pfnCreate;
	};

	const CLSID CLSID_PreviewAllHandler = { 0xA26D5A00, 0xAF3F, 0x47B7, 0xB0, 0x75, 0xA3, 0x28, 0x2D, 0xE9, 0x04, 0xE6 };

	HRESULT CPreviewAllHandler_CreateInstance(REFIID riid, void** ppv)
	{
		*ppv = NULL;

		CPreviewAllHandler* pNew = new (std::nothrow) CPreviewAllHandler();
		HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
		if (SUCCEEDED(hr))
		{
			hr = pNew->QueryInterface(riid, ppv);
			pNew->Release();
		}
		return hr;
	}

	const CLASS_OBJECT_INIT c_rgClassObjectInit[] =
	{
		{ &CLSID_PreviewAllHandler, CPreviewAllHandler_CreateInstance }
	};

	long g_cRefModule = 0;

	void DllAddRef()
	{
		InterlockedIncrement(&g_cRefModule);
	}

	void DllRelease()
	{
		InterlockedDecrement(&g_cRefModule);
	}

	class CClassFactory : public IClassFactory
	{
	public:
		static HRESULT CreateInstance(REFCLSID clsid, const CLASS_OBJECT_INIT* pClassObjectInits, size_t cClassObjectInits, REFIID riid, void** ppv)
		{
			*ppv = NULL;
			HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
			for (size_t i = 0; i < cClassObjectInits; i++)
			{
				if (clsid == *pClassObjectInits[i].pClsid)
				{
					IClassFactory* pClassFactory = new (std::nothrow) CClassFactory(pClassObjectInits[i].pfnCreate);
					hr = pClassFactory ? S_OK : E_OUTOFMEMORY;
					if (SUCCEEDED(hr))
					{
						hr = pClassFactory->QueryInterface(riid, ppv);
						pClassFactory->Release();
					}
					break; // match found
				}
			}
			return hr;
		}

		CClassFactory(PFNCREATEINSTANCE pfnCreate) : _cRef(1), _pfnCreate(pfnCreate)
		{
			DllAddRef();
		}

		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
		{
			static const QITAB qit[] =
			{
				QITABENT(CClassFactory, IClassFactory),
				{ 0 }
			};
			return QISearch(this, qit, riid, ppv);
		}

		IFACEMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&_cRef);
		}

		IFACEMETHODIMP_(ULONG) Release()
		{
			long cRef = InterlockedDecrement(&_cRef);
			if (cRef == 0)
			{
				delete this;
			}
			return cRef;
		}

		// IClassFactory
		IFACEMETHODIMP CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
		{
			return punkOuter ? CLASS_E_NOAGGREGATION : _pfnCreate(riid, ppv);
		}

		IFACEMETHODIMP LockServer(BOOL fLock)
		{
			if (fLock)
			{
				DllAddRef();
			}
			else
			{
				DllRelease();
			}
			return S_OK;
		}

	private:
		~CClassFactory()
		{
			DllRelease();
		}

		long _cRef;
		PFNCREATEINSTANCE _pfnCreate;
	};

}

BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, void*)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);

	return TRUE;
}

STDAPI DllCanUnloadNow()
{
	return (g_cRefModule == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv)
{
	return CClassFactory::CreateInstance(clsid, c_rgClassObjectInit, ARRAYSIZE(c_rgClassObjectInit), riid, ppv);
}