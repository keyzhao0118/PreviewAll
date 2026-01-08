#pragma once

#include <QSettings>
#include <Windows.h>

class PreviewAllRegister
{
public:
	const static QString CLSID_PreviewHandlerCategory;
	const static QString CLSID_PreviewAllHandler;
	const static QString APPID_PREVHOST64;
	const static QString NAME_PreviewAllHandler;

	const static QStringList imageExtList;
	const static QStringList archiveExtList;
	const static QStringList codeExtList;

	void static registerHandler();
	void static unregisterHandler();
	void static registerExtention(const QString& suffix);
	void static unregisterExtention(const QString& suffix);
	bool static isRegisteredHandler();
	bool static isRegisteredExtention(const QString& suffix);


private:
	void static registerHandler(HKEY hkey);
	void static unregisterHandler(HKEY hkey);
	void static registerExtention(const QString& suffix, HKEY hkey);
	void static unregisterExtention(const QString& suffix, HKEY hkey);
};
