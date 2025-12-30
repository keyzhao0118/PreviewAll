#include "commonhelper.h"
#include <shellapi.h>
#include <QDebug>
#include <QFileIconProvider>
#include <QLibrary>

#include "instreamwrapper.h"

namespace
{

extern "C" const GUID CLSID_CFormatZip;
extern "C" const GUID CLSID_CFormat7z;
extern "C" const GUID CLSID_CFormatRar;
extern "C" const GUID CLSID_CFormatRar5;
typedef UINT32(WINAPI* CreateObjectFunc)(const GUID* clsID, const GUID* iid, void** outObject);

bool isZip(const uint8_t* p)
{
	return p[0] == 0x50 && p[1] == 0x4B &&
		(p[2] == 0x03 || p[2] == 0x05 || p[2] == 0x07) &&
		(p[3] == 0x04 || p[3] == 0x06 || p[3] == 0x08);
}

bool is7z(const uint8_t* p)
{
	static const uint8_t sig[6] = {
		0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C
	};
	return memcmp(p, sig, 6) == 0;
}

bool isRar4(const uint8_t* p)
{
	static const uint8_t sig[7] = {
		0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00
	};
	return memcmp(p, sig, 7) == 0;
}

bool isRar5(const uint8_t* p)
{
	static const uint8_t sig[8] = {
		0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00
	};
	return memcmp(p, sig, 8) == 0;
}

CLSID getAdapatedCLSID(const QString& archivePath)
{
	QFile file(archivePath);
	if (!file.open(QIODevice::ReadOnly))
		return CLSID_NULL;

	uint8_t header[32] = { 0 };
	const qint64 readSize = file.read(reinterpret_cast<char*>(header), sizeof(header));
	file.close();

	if (readSize < 8)
		return CLSID_NULL;

	if (is7z(header))
		return CLSID_CFormat7z;

	if (isRar5(header))
		return CLSID_CFormatRar5;

	if (isRar4(header))
		return CLSID_CFormatRar;

	if (isZip(header))
		return CLSID_CFormatZip;

	return CLSID_NULL;
}

} // anonymous namespace

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

	// Create QDateTime in LocalTime
	return QDateTime::fromMSecsSinceEpoch(msSinceEpoch, Qt::LocalTime);
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

// Helper: get Windows file type display name via SHGetFileInfo.
// Uses name (for extension) and isDir to decide the shell attributes queried.
QString CommonHelper::fileTypeDisplayName(const QString& name, bool bIsDir)
{
	if (bIsDir)
		return QObject::tr("Folder");

	const int dot = name.lastIndexOf('.');
	if (dot < 0 || dot == name.length() - 1)
		return QObject::tr("File");

	const QString ext = name.mid(dot); // includes dot
	const QByteArray extData = ext.toLocal8Bit();

	SHFILEINFOA sfiA = { 0 };
	if (SHGetFileInfoA(extData.constData(), FILE_ATTRIBUTE_NORMAL, &sfiA, sizeof(sfiA), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
		return QString::fromLocal8Bit(sfiA.szTypeName);
	else
		return ext;
}

QIcon CommonHelper::fileIconForName(const QString& name, bool bIsDir)
{
	static QHash<QString, QIcon> iconCache;

	if (bIsDir)
	{
		if(!iconCache.contains("folder"))
		{
			QFileIconProvider provider;
			QIcon folderIcon = provider.icon(QFileIconProvider::Folder);
			iconCache.insert("folder", folderIcon);
			return folderIcon;
		}
		else
		{
			return iconCache.value("folder");
		}
	}

	const int dot = name.lastIndexOf('.');
	if (dot < 0 || dot == name.length() - 1)
	{
		if (!iconCache.contains("file"))
		{
			QFileIconProvider provider;
			QIcon fileIcon = provider.icon(QFileIconProvider::File);
			iconCache.insert("file", fileIcon);
			return fileIcon;
		}
		else
		{
			return iconCache.value("file");
		}
	}

	const QString ext = name.mid(dot);
	if (!iconCache.contains(ext))
	{
		QFileIconProvider provider;
		QIcon extIcon = provider.icon(QFileInfo(ext));
		iconCache.insert(ext, extIcon);
		return extIcon;
	}
	else
	{
		return iconCache.value(ext);
	}
	
}

bool CommonHelper::tryOpenArchive(
	const QString& archivePath,
	IArchiveOpenCallback* openCallback,
	CMyComPtr<IInArchive>& outInArchive)
{
	outInArchive.Release();

	QLibrary sevenZipLib("7zip.dll");
	if (!sevenZipLib.load())
		return false;

	CreateObjectFunc createObjectFunc = (CreateObjectFunc)sevenZipLib.resolve("CreateObject");
	if (!createObjectFunc)
		return false;

	const GUID clsid = getAdapatedCLSID(archivePath);
	if (clsid == CLSID_NULL)
		return false;

	CMyComPtr<IInArchive> archive;
	if (createObjectFunc(&clsid, &IID_IInArchive, (void**)&archive) != S_OK)
		return false;

	InStreamWrapper* inStreamSpec = new InStreamWrapper(archivePath);
	CMyComPtr<IInStream> inStream(inStreamSpec);
	if (!inStreamSpec->isOpen())
		return false;

	HRESULT hr = archive->Open(inStream, nullptr, openCallback);
	if (hr != S_OK)
		return false;

	outInArchive = archive;
	return true;
}
