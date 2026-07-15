#pragma once

#include <QString>
#include <QStringList>

class PreviewAllRegister
{
public:
	static const QString REGISTER_HANDLER_ARGUMENT;

	static const QStringList imageExtList;
	static const QStringList archiveExtList;
	static const QStringList markdownExtList;

	static bool ensureHandlerRegistered();
	static bool registerHandler();
	static void unregisterHandler();

	static void registerExtentions(const QStringList& extList);
	static void unregisterExtentions(const QStringList& extList);
	static void unregisterAllExtentions();

private:
	enum class RegistryScope
	{
		CurrentUser,
		LocalMachine
	};

	static const QString CLSID_PreviewHandlerCategory;
	static const QString CLSID_PreviewAllHandler;
	static const QString APPID_PREVHOST64;
	static const QString NAME_PreviewAllHandler;

	static bool isRegisteredHandler();
	static QString registryRootName(RegistryScope scope);

	static bool registerHandler(RegistryScope scope);
	static void unregisterHandler(RegistryScope scope);
	static bool isRegisteredHandler(RegistryScope scope);

	static void registerExtention(const QString& suffix, RegistryScope scope);
	static void unregisterExtention(const QString& suffix, RegistryScope scope);
};
