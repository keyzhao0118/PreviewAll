#include "previewallregister.h"
#include <QApplication>
#include <QDir>

const QString PreviewAllRegister::CLSID_PreviewHandlerCategory = "{8895b1c6-b41f-4c1c-a562-0d564250836f}";
const QString PreviewAllRegister::CLSID_PreviewAllHandler = "{A26D5A00-AF3F-47B7-B075-A3282DE904E6}";
const QString PreviewAllRegister::APPID_PREVHOST64 = "{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}";
const QString PreviewAllRegister::NAME_PreviewAllHandler = "PreviewAllHandler";

const QStringList PreviewAllRegister::imageExtList = { ".png",".jpg",".jpeg",".tif",".tiff",".bmp",".webp",".ico",".svg",".gif" };
const QStringList PreviewAllRegister::archiveExtList = { ".zip", ".rar", ".7z" };
const QStringList PreviewAllRegister::codeExtList = { ".c" };

void PreviewAllRegister::registerHandler()
{
	registerHandler(HKEY_CURRENT_USER);
	registerHandler(HKEY_LOCAL_MACHINE);
}

void PreviewAllRegister::unregisterHandler()
{
	unregisterHandler(HKEY_CURRENT_USER);
	unregisterHandler(HKEY_LOCAL_MACHINE);
}

void PreviewAllRegister::registerExtention(const QString& suffix)
{
	registerExtention(suffix, HKEY_CURRENT_USER);
	registerExtention(suffix, HKEY_LOCAL_MACHINE);
}

void PreviewAllRegister::unregisterExtention(const QString& suffix)
{
	unregisterExtention(suffix, HKEY_CURRENT_USER);
	unregisterExtention(suffix, HKEY_LOCAL_MACHINE);
}

bool PreviewAllRegister::isRegisteredExtention(const QString& suffix)
{
	QSettings shellExKey("HKEY_CLASSES_ROOT\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
	return shellExKey.value(".").toString() == CLSID_PreviewAllHandler;
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


