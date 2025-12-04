#pragma once

#include <QDateTime>
#include <windows.h>

namespace CommonHelper
{
	void LogKeyZipDebugMsg(const QString& msg);
	QDateTime fileTimeToDateTime(FILETIME filetime);
	QString formatFileSize(quint64 bytes);
}