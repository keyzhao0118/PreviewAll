#include "commonhelper.h"
#include <QDebug>

void CommonHelper::LogKeyZipDebugMsg(const QString& msg)
{

	QString logMsg = "[KeyZip]: " + msg;
	qDebug() << logMsg;
}

QDateTime CommonHelper::fileTimeToDateTime(FILETIME filetime)
{
	// Check for invalid FILETIME (zero)
	if (filetime.dwLowDateTime == 0 && filetime.dwHighDateTime == 0) {
		return QDateTime();
	}

	// Combine FILETIME into 64-bit value
	ULARGE_INTEGER uli;
	uli.LowPart = filetime.dwLowDateTime;
	uli.HighPart = filetime.dwHighDateTime;

	// Constants: difference between Windows and Unix epochs in 100-ns units
	static constexpr quint64 WINDOWS_TO_UNIX_EPOCH_100NS = 116444736000000000ULL;

	// Guard against underflow
	if (uli.QuadPart < WINDOWS_TO_UNIX_EPOCH_100NS) {
		return QDateTime();
	}

	// Convert to milliseconds since Unix epoch
	const quint64 unixTime100ns = uli.QuadPart - WINDOWS_TO_UNIX_EPOCH_100NS;
	const qint64 msSinceEpoch = static_cast<qint64>(unixTime100ns / 10000ULL);

	// Create QDateTime in UTC
	return QDateTime::fromMSecsSinceEpoch(msSinceEpoch, Qt::UTC);
}

QString CommonHelper::formatFileSize(quint64 bytes)
{
	if (bytes == 0) {
		return QStringLiteral("0 B");
	}

	static const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
	int unitIndex = 0;
	double value = static_cast<double>(bytes);

	while (value >= 1024.0 && unitIndex < (static_cast<int>(sizeof(units) / sizeof(units[0])) - 1)) {
		value /= 1024.0;
		++unitIndex;
	}

	QString numberStr;
	if (unitIndex == 0) {
		// Bytes: no decimals
		numberStr = QString::number(static_cast<qulonglong>(value));
	} else {
		// Up to 2 decimals
		numberStr = QString::number(value, 'f', 2);

		// Trim trailing zeros and decimal point if needed
		while (numberStr.endsWith('0')) {
			numberStr.chop(1);
		}
		if (numberStr.endsWith('.')) {
			numberStr.chop(1);
		}
	}

	return numberStr + QStringLiteral(" ") + QString::fromLatin1(units[unitIndex]);
}
