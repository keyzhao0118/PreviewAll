#pragma once

#include <Common/MyCom.h>
#include <7zip/IStream.h>
#include <QFile>

class OutStreamWrapper :
	public ISequentialOutStream,
	public CMyUnknownImp
{
	Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)

public:
	OutStreamWrapper(const QString& filePath);
	~OutStreamWrapper();

	// ISequentialOutStream
	STDMETHOD(Write)(const void* data, UInt32 size, UInt32* processedSize) override;

private:
	QFile m_file;
};