#include "previewallregister.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <cstdlib>
#include <shellapi.h>
#include <string>

namespace
{
	QString registryRootName(HKEY hkey)
	{
		if (hkey == HKEY_CURRENT_USER)
			return "HKEY_CURRENT_USER";
		if (hkey == HKEY_LOCAL_MACHINE)
			return "HKEY_LOCAL_MACHINE";
		return {};
	}

	QString handlerPath()
	{
		return QDir::toNativeSeparators(
			QDir(QCoreApplication::applicationDirPath()).filePath("PreviewAllHandler.dll"));
	}

	bool pathsMatch(const QString& actual, const QString& expected)
	{
		return QDir::cleanPath(QDir::fromNativeSeparators(actual)).compare(
			QDir::cleanPath(QDir::fromNativeSeparators(expected)), Qt::CaseInsensitive) == 0;
	}
}

const QString PreviewAllRegister::REGISTER_HANDLER_ARGUMENT = "--register-preview-handler";
const QString PreviewAllRegister::CLSID_PreviewHandlerCategory = "{8895b1c6-b41f-4c1c-a562-0d564250836f}";
const QString PreviewAllRegister::CLSID_PreviewAllHandler = "{A26D5A00-AF3F-47B7-B075-A3282DE904E6}";
const QString PreviewAllRegister::APPID_PREVHOST64 = "{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}";
const QString PreviewAllRegister::NAME_PreviewAllHandler = "PreviewAllHandler";

const QStringList PreviewAllRegister::imageExtList = { ".png",".jpg",".jpeg",".tif",".tiff",".bmp",".webp",".ico",".svg",".gif" };
const QStringList PreviewAllRegister::archiveExtList = { ".zip", ".rar", ".7z" };
const QStringList PreviewAllRegister::markdownExtList = { ".md", ".markdown" };

bool PreviewAllRegister::registerHandler()
{
	const bool currentUserWritten = registerHandler(HKEY_CURRENT_USER);
	const bool localMachineWritten = registerHandler(HKEY_LOCAL_MACHINE);
	return currentUserWritten && localMachineWritten && isRegisteredHandler();
}

bool PreviewAllRegister::ensureHandlerRegistered()
{
	if (isRegisteredHandler())
		return true;

	const std::wstring executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
	const std::wstring arguments = REGISTER_HANDLER_ARGUMENT.toStdWString();

	SHELLEXECUTEINFOW executeInfo{};
	executeInfo.cbSize = sizeof(executeInfo);
	executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	executeInfo.lpVerb = L"runas";
	executeInfo.lpFile = executable.c_str();
	executeInfo.lpParameters = arguments.c_str();
	executeInfo.nShow = SW_HIDE;

	if (!ShellExecuteExW(&executeInfo) || !executeInfo.hProcess)
	{
		qWarning() << "Failed to launch elevated preview handler registration. Windows error:" << GetLastError();
		return false;
	}

	const DWORD waitResult = WaitForSingleObject(executeInfo.hProcess, INFINITE);
	DWORD exitCode = EXIT_FAILURE;
	const bool exitedSuccessfully = waitResult == WAIT_OBJECT_0
		&& GetExitCodeProcess(executeInfo.hProcess, &exitCode)
		&& exitCode == EXIT_SUCCESS;
	CloseHandle(executeInfo.hProcess);

	if (!exitedSuccessfully || !isRegisteredHandler())
	{
		qWarning() << "Elevated preview handler registration failed.";
		return false;
	}

	return true;
}

void PreviewAllRegister::unregisterHandler()
{
	unregisterHandler(HKEY_CURRENT_USER);
	unregisterHandler(HKEY_LOCAL_MACHINE);
}

bool PreviewAllRegister::isRegisteredHandler()
{
	return QFileInfo(handlerPath()).isFile()
		&& isRegisteredHandler(HKEY_CURRENT_USER)
		&& isRegisteredHandler(HKEY_LOCAL_MACHINE);
}

void PreviewAllRegister::registerExtentions(const QStringList& extList)
{
	for (const QString& suffix : extList)
		registerExtention(suffix, HKEY_CURRENT_USER);
}

void PreviewAllRegister::unregisterExtentions(const QStringList& extList)
{
	for (const QString& suffix : extList)
		unregisterExtention(suffix, HKEY_CURRENT_USER);
}

void PreviewAllRegister::unregisterAllExtentions()
{
	unregisterExtentions(imageExtList);
	unregisterExtentions(archiveExtList);
	unregisterExtentions(markdownExtList);
}

bool PreviewAllRegister::registerHandler(HKEY hkey)
{
	const QString rootName = registryRootName(hkey);
	if (rootName.isEmpty())
		return false;

	QSettings clsidRoot(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
	clsidRoot.setValue(".", NAME_PreviewAllHandler);
	clsidRoot.setValue("AppID", APPID_PREVHOST64);
	clsidRoot.setValue("DisableLowILProcessIsolation", 1);

	QSettings inproc(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler + "\\InProcServer32", QSettings::NativeFormat);
	inproc.setValue(".", handlerPath());
	inproc.setValue("ThreadingModel", "Apartment");

	QSettings previewHandlers(rootName + "\\Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", QSettings::NativeFormat);
	previewHandlers.setValue(CLSID_PreviewAllHandler, NAME_PreviewAllHandler);

	clsidRoot.sync();
	inproc.sync();
	previewHandlers.sync();

	if (clsidRoot.status() != QSettings::NoError
		|| inproc.status() != QSettings::NoError
		|| previewHandlers.status() != QSettings::NoError)
	{
		return false;
	}

	return isRegisteredHandler(hkey);
}

void PreviewAllRegister::unregisterHandler(HKEY hkey)
{
	const QString rootName = registryRootName(hkey);
	if (rootName.isEmpty())
		return;

	QSettings inproc(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler + "\\InProcServer32", QSettings::NativeFormat);
	inproc.remove("");

	QSettings clsidRoot(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
	clsidRoot.remove("");

	QSettings previewHandlers(rootName + "\\Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", QSettings::NativeFormat);
	previewHandlers.remove(CLSID_PreviewAllHandler);
}

void PreviewAllRegister::registerExtention(const QString& suffix, HKEY hkey)
{
	const QString rootName = registryRootName(hkey);
	if (rootName.isEmpty())
		return;

	QSettings shellExKey(rootName + "\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
	shellExKey.setValue(".", CLSID_PreviewAllHandler);
}

void PreviewAllRegister::unregisterExtention(const QString& suffix, HKEY hkey)
{
	const QString rootName = registryRootName(hkey);
	if (rootName.isEmpty())
		return;

	QSettings shellExKey(rootName + "\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
	shellExKey.remove("");
}

bool PreviewAllRegister::isRegisteredHandler(HKEY hkey)
{
	const QString rootName = registryRootName(hkey);
	if (rootName.isEmpty())
		return false;

	QSettings clsidRoot(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
	QSettings inproc(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler + "\\InProcServer32", QSettings::NativeFormat);
	QSettings previewHandlers(rootName + "\\Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", QSettings::NativeFormat);

	const QVariant isolationValue = clsidRoot.value("DisableLowILProcessIsolation");
	const bool isolationValueMatches = isolationValue.metaType().id() == QMetaType::Int
		&& isolationValue.toInt() == 1;

	return clsidRoot.value(".").toString() == NAME_PreviewAllHandler
		&& clsidRoot.value("AppID").toString() == APPID_PREVHOST64
		&& isolationValueMatches
		&& pathsMatch(inproc.value(".").toString(), handlerPath())
		&& inproc.value("ThreadingModel").toString() == "Apartment"
		&& previewHandlers.value(CLSID_PreviewAllHandler).toString() == NAME_PreviewAllHandler;
}
