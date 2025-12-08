#pragma once

#include <QDateTime>
#include <QIcon>
#include <windows.h>

namespace CommonHelper
{
	void LogKeyZipDebugMsg(const QString& msg);
	QDateTime fileTimeToDateTime(FILETIME filetime);
	QString formatFileSize(quint64 bytes);
	QString fileTypeDisplayName(const QString& name, bool bIsDir);
	QIcon fileIconForName(const QString& name, bool bIsDir);
}