#include "previewallregister.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <cstdlib>
#include <string>
#include <shellapi.h>

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
	registerHandler(HKEY_CURRENT_USER);
	registerHandler(HKEY_LOCAL_MACHINE);
	return isRegisteredHandler();
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
	QSettings previewHandlers("HKEY_CLASSES_ROOT\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
	bool bClassRootRegistered = previewHandlers.value(".").toString() == NAME_PreviewAllHandler;
	bool bCurrentUserRegistered = isRegisteredHandler(HKEY_CURRENT_USER);
	bool bLocalMachineRegistered = isRegisteredHandler(HKEY_LOCAL_MACHINE);
	return bClassRootRegistered && bCurrentUserRegistered && bLocalMachineRegistered;
}

void PreviewAllRegister::registerExtentions(const QStringList& extList)
{
	for (const QString& suffix : extList)
	{
		registerExtention(suffix, HKEY_CURRENT_USER);
	}
}

void PreviewAllRegister::unregisterExtentions(const QStringList& extList)
{
	for (const QString& suffix : extList)
	{
		unregisterExtention(suffix, HKEY_CURRENT_USER);
	}
}

void PreviewAllRegister::unregisterAllExtentions()
{
	unregisterExtentions(imageExtList);
	unregisterExtentions(archiveExtList);
	unregisterExtentions(markdownExtList);
}


void PreviewAllRegister::registerHandler(HKEY hkey)
{
	QString rootName;
	if (hkey == HKEY_CURRENT_USER)
		rootName = "HKEY_CURRENT_USER";
	else if (hkey == HKEY_LOCAL_MACHINE)
		rootName = "HKEY_LOCAL_MACHINE";
	else
		return;

	{
		QSettings clsidRoot(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
		clsidRoot.setValue(".", NAME_PreviewAllHandler);
		clsidRoot.setValue("AppID", APPID_PREVHOST64);
		clsidRoot.setValue("DisableLowILProcessIsolation", 1);

		QSettings inproc(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler + "\\InProcServer32", QSettings::NativeFormat);
		QString curPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
		QString handlerPath = curPath + QDir::separator() + "PreviewAllHandler.dll";
		inproc.setValue(".", handlerPath);
		inproc.setValue("ThreadingModel", "Apartment");
	}

	{
		QSettings previewHandlers(rootName + "\\Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", QSettings::NativeFormat);
		previewHandlers.setValue(CLSID_PreviewAllHandler, NAME_PreviewAllHandler);
	}
}

void PreviewAllRegister::unregisterHandler(HKEY hkey)
{
	QString rootName;
	if (hkey == HKEY_CURRENT_USER)
		rootName = "HKEY_CURRENT_USER";
	else if (hkey == HKEY_LOCAL_MACHINE)
		rootName = "HKEY_LOCAL_MACHINE";
	else
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
	QString rootName;
	if (hkey == HKEY_CURRENT_USER)
		rootName = "HKEY_CURRENT_USER";
	else if (hkey == HKEY_LOCAL_MACHINE)
		rootName = "HKEY_LOCAL_MACHINE";
	else
		return;

	QSettings shellExKey(rootName + "\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
	shellExKey.setValue(".", CLSID_PreviewAllHandler);
}

void PreviewAllRegister::unregisterExtention(const QString& suffix, HKEY hkey)
{
	QString rootName;
	if (hkey == HKEY_CURRENT_USER)
		rootName = "HKEY_CURRENT_USER";
	else if (hkey == HKEY_LOCAL_MACHINE)
		rootName = "HKEY_LOCAL_MACHINE";
	else
		return;

	QSettings shellExKey(rootName + "\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
	shellExKey.remove("");
}

bool PreviewAllRegister::isRegisteredHandler(HKEY hkey)
{
	QString rootName;
	if (hkey == HKEY_CURRENT_USER)
		rootName = "HKEY_CURRENT_USER";
	else if (hkey == HKEY_LOCAL_MACHINE)
		rootName = "HKEY_LOCAL_MACHINE";
	else
		return false;

	QSettings previewHandlers(rootName + "\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
	return previewHandlers.value(".").toString() == NAME_PreviewAllHandler;
}


