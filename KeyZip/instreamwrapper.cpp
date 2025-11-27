#include "instreamwrapper.h"

InStreamWrapper::InStreamWrapper(const QString& filePath)
{
	m_file.setFileName(filePath);
	m_file.open(QIODevice::ReadOnly);
}

InStreamWrapper::~InStreamWrapper()
{
	m_file.close();
}

bool InStreamWrapper::isOpen() const
{
	return m_file.isOpen();
}

STDMETHODIMP InStreamWrapper::Read(void* data, UInt32 size, UInt32* processedSize)
{
	qint64 bytesRead = m_file.read(static_cast<char*>(data), size);
	if (processedSize)
	{
		*processedSize = static_cast<UInt32>(bytesRead);
	}
	return S_OK;
}

STDMETHODIMP InStreamWrapper::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition)
{
	qint64 newPosValue = 0;
	switch (seekOrigin)
	{
	case STREAM_SEEK_SET:
		newPosValue = offset;
		break;
	case STREAM_SEEK_CUR:
		newPosValue = m_file.pos() + offset;
		break;
	case STREAM_SEEK_END:
		newPosValue = m_file.size() + offset;
		break;
	default:
		return STG_E_INVALIDFUNCTION;
	}

	if (m_file.seek(newPosValue))
	{
		if (newPosition)
		{
			*newPosition = m_file.pos();
		}
		return S_OK;
	}
	return E_FAIL;
}

