#include "outstreamwrapper.h"
#include <QDir>
#include <QFileInfo>

OutStreamWrapper::OutStreamWrapper(const QString& filePath)
	: m_file(filePath)
{
	const QFileInfo fileInfo(filePath);
	QDir parentDir = fileInfo.dir();
	if (!parentDir.exists())
		parentDir.mkpath(QStringLiteral("."));
	m_file.open(QIODevice::WriteOnly);
}

OutStreamWrapper::~OutStreamWrapper()
{
	m_file.close();
}

STDMETHODIMP OutStreamWrapper::Write(const void* data, UInt32 size, UInt32* processedSize)
{
	if (!m_file.isOpen())
		return E_FAIL;
	const qint64 bytesWritten = m_file.write(reinterpret_cast<const char*>(data), static_cast<qint64>(size));
	if (bytesWritten < 0)
		return E_FAIL;
	if (processedSize)
		*processedSize = static_cast<UInt32>(bytesWritten);
	return S_OK;
}