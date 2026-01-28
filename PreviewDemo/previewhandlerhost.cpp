#include "previewhandlerhost.h"

#include <QFileInfo>
#include <QString>

#define NOMINMAX
#include <Windows.h>

#include <shlwapi.h>
#include <shobjidl.h>
#include <wrl/client.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

using Microsoft::WRL::ComPtr;

namespace {
	constexpr wchar_t kPreviewHandlerGuidStr[] = L"{8895b1c6-b41f-4c1c-a562-0d564250836f}";

	static QString hresultToString(HRESULT hr)
	{
		return QString("0x%1").arg(QString::number(static_cast<qulonglong>(hr), 16));
	}

	static bool regReadDefaultString(HKEY root, const std::wstring& subKey, std::wstring* out)
	{
		DWORD type = 0;
		DWORD cb = 0;
		LSTATUS st = RegGetValueW(root, subKey.c_str(), nullptr, RRF_RT_REG_SZ, &type, nullptr, &cb);
		if (st != ERROR_SUCCESS || cb < sizeof(wchar_t))
			return false;

		std::wstring buf;
		buf.resize(cb / sizeof(wchar_t));
		st = RegGetValueW(root, subKey.c_str(), nullptr, RRF_RT_REG_SZ, &type, buf.data(), &cb);
		if (st != ERROR_SUCCESS)
			return false;

		// Ensure NUL-terminated string; std::wstring may include trailing NUL.
		if (!buf.empty() && buf.back() == L'\0')
			buf.pop_back();
		*out = buf;
		return !out->empty();
	}

	static bool tryReadHandlerClsidFromSubkey(const std::wstring& baseKey, std::wstring* outClsidStr)
	{
		const std::wstring key = baseKey + L"\\shellex\\" + kPreviewHandlerGuidStr;
		return regReadDefaultString(HKEY_CLASSES_ROOT, key, outClsidStr);
	}
}

PreviewHandlerHost::PreviewHandlerHost(QWidget* parent)
	: QWidget(parent)
{
	setMinimumSize(200, 200);

	// Important: PreviewHandler 需要一个原生 HWND 作为父窗口
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_DontCreateNativeAncestors);

	// 背景交给 handler 自己绘制；若没有 handler，则用系统默认
	setAutoFillBackground(true);
}

PreviewHandlerHost::~PreviewHandlerHost()
{
	unload();
}

void PreviewHandlerHost::unload()
{
	if (m_handler)
	{
		m_handler->Unload();
		m_handler.Reset();
	}
	m_unknown.Reset();
	m_loadedFilePath.clear();
}

bool PreviewHandlerHost::loadFile(const QString& filePath, QString* outError)
{
	if (outError)
		outError->clear();

	if (filePath.isEmpty())
	{
		if (outError) *outError = "空路径";
		return false;
	}

	if (!QFileInfo::exists(filePath))
	{
		if (outError) *outError = "文件不存在";
		return false;
	}

	if (m_loadedFilePath == filePath && m_handler)
		return true;

	unload();

	CLSID clsid{};
	QString resolveErr;
	if (!resolvePreviewHandlerClsid(filePath, &clsid, &resolveErr))
	{
		if (outError) *outError = resolveErr;
		return false;
	}

	const HRESULT hrCreate = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_unknown));
	if (FAILED(hrCreate))
	{
		if (outError) *outError = QString("CoCreateInstance 失败: %1").arg(hresultToString(hrCreate));
		return false;
	}

	// 初始化：优先 IInitializeWithFile；失败则 fallback 到 IInitializeWithStream
	{
		ComPtr<IInitializeWithFile> initFile;
		if (SUCCEEDED(m_unknown.As(&initFile)) && initFile)
		{
			std::wstring wpath = filePath.toStdWString();
			const HRESULT hrInit = initFile->Initialize(wpath.c_str(), STGM_READ | STGM_SHARE_DENY_NONE);
			if (FAILED(hrInit))
			{
				if (outError) *outError = QString("IInitializeWithFile::Initialize 失败: %1").arg(hresultToString(hrInit));
				unload();
				return false;
			}
		}
		else
		{
			ComPtr<IInitializeWithStream> initStream;
			if (SUCCEEDED(m_unknown.As(&initStream)) && initStream)
			{
				ComPtr<IStream> stream;
				std::wstring wpath = filePath.toStdWString();
				const HRESULT hrStream = SHCreateStreamOnFileEx(
					wpath.c_str(),
					STGM_READ | STGM_SHARE_DENY_NONE,
					FILE_ATTRIBUTE_NORMAL,
					FALSE,
					nullptr,
					&stream
				);
				if (FAILED(hrStream))
				{
					if (outError) *outError = QString("SHCreateStreamOnFileEx 失败: %1").arg(hresultToString(hrStream));
					unload();
					return false;
				}

				const HRESULT hrInit = initStream->Initialize(stream.Get(), STGM_READ);
				if (FAILED(hrInit))
				{
					if (outError) *outError = QString("IInitializeWithStream::Initialize 失败: %1").arg(hresultToString(hrInit));
					unload();
					return false;
				}
			}
			else
			{
				if (outError) *outError = "PreviewHandler 不支持 IInitializeWithFile / IInitializeWithStream";
				unload();
				return false;
			}
		}
	}

	const HRESULT hrPH = m_unknown.As(&m_handler);
	if (FAILED(hrPH) || !m_handler)
	{
		if (outError) *outError = QString("QueryInterface IPreviewHandler 失败: %1").arg(hresultToString(hrPH));
		unload();
		return false;
	}

	// 绑定到当前 QWidget 的 HWND
	HWND hwndParent = reinterpret_cast<HWND>(winId());
	RECT rc{};
	GetClientRect(hwndParent, &rc);
	const HRESULT hrSetWindow = m_handler->SetWindow(hwndParent, &rc);
	if (FAILED(hrSetWindow))
	{
		if (outError) *outError = QString("IPreviewHandler::SetWindow 失败: %1").arg(hresultToString(hrSetWindow));
		unload();
		return false;
	}

	const HRESULT hrDo = m_handler->DoPreview();
	if (FAILED(hrDo))
	{
		if (outError) *outError = QString("IPreviewHandler::DoPreview 失败: %1").arg(hresultToString(hrDo));
		unload();
		return false;
	}

	m_loadedFilePath = filePath;
	updateHandlerRect();
	return true;
}

void PreviewHandlerHost::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	updateHandlerRect();
}

void PreviewHandlerHost::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	updateHandlerRect();
}

void PreviewHandlerHost::updateHandlerRect()
{
	if (!m_handler)
		return;

	HWND hwndParent = reinterpret_cast<HWND>(winId());
	RECT rc{};
	GetClientRect(hwndParent, &rc);
	m_handler->SetRect(&rc);
}

bool PreviewHandlerHost::resolvePreviewHandlerClsid(const QString& filePath, CLSID* outClsid, QString* outError)
{
	if (outError)
		outError->clear();

	const QFileInfo fi(filePath);
	const QString suffix = fi.suffix();
	if (suffix.isEmpty())
	{
		if (outError) *outError = "文件无后缀，无法按后缀查找 PreviewHandler";
		return false;
	}

	const std::wstring extKey = (L"." + suffix.toLower().toStdWString());

	std::wstring clsidStr;

	// 1) HKCR\.ext\shellex\{PreviewHandlerGUID}
	if (!tryReadHandlerClsidFromSubkey(extKey, &clsidStr))
	{
		// 2) HKCR\SystemFileAssociations\.ext\shellex\{PreviewHandlerGUID}
		const std::wstring sysAssocKey = L"SystemFileAssociations\\" + extKey;
		if (!tryReadHandlerClsidFromSubkey(sysAssocKey, &clsidStr))
		{
			// 3) 通过 ProgID：HKCR\.ext (Default) -> ProgID，然后 HKCR\<ProgID>\shellex\{...}
			std::wstring progId;
			if (regReadDefaultString(HKEY_CLASSES_ROOT, extKey, &progId))
			{
				tryReadHandlerClsidFromSubkey(progId, &clsidStr);
			}
		}
	}

	if (clsidStr.empty())
	{
		if (outError) *outError = QString("注册表未找到该后缀的 PreviewHandler: %1").arg(QString::fromStdWString(extKey));
		return false;
	}

	const HRESULT hr = CLSIDFromString(clsidStr.c_str(), outClsid);
	if (FAILED(hr))
	{
		if (outError) *outError = QString("CLSIDFromString 失败: %1 (值=%2)").arg(hresultToString(hr), QString::fromStdWString(clsidStr));
		return false;
	}

	return true;
}


