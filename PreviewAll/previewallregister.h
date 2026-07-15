#pragma once

#include <QSettings>
#include <Windows.h>

class PreviewAllRegister
{
public:
	const static QString REGISTER_HANDLER_ARGUMENT;
	const static QString CLSID_PreviewHandlerCategory;
	const static QString CLSID_PreviewAllHandler;
	const static QString APPID_PREVHOST64;
	const static QString NAME_PreviewAllHandler;

	const static QStringList imageExtList;
	const static QStringList archiveExtList;
	const static QStringList markdownExtList;

	bool static registerHandler();
	bool static ensureHandlerRegistered();
	void static unregisterHandler();
	bool static isRegisteredHandler();

	void static registerExtentions(const QStringList& extList);
	void static unregisterExtentions(const QStringList& extList);
	void static unregisterAllExtentions();

private:
	bool static registerHandler(HKEY hkey);
	void static unregisterHandler(HKEY hkey);
	void static registerExtention(const QString& suffix, HKEY hkey);
	void static unregisterExtention(const QString& suffix, HKEY hkey);
	bool static isRegisteredHandler(HKEY hkey);
};
