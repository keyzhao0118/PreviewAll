#pragma once

#include <QObject>
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include <Common/MyCom.h>

class ArchiveOpenCallBack
	: public QObject
	, public IArchiveOpenCallback
	, public ICryptoGetTextPassword
	, public CMyUnknownImp
{
	Q_OBJECT
	Z7_COM_UNKNOWN_IMP_2(IArchiveOpenCallback, ICryptoGetTextPassword)

public:
	// IArchiveOpenCallback
	STDMETHOD(SetTotal)(const UInt64* files, const UInt64* bytes) override;
	STDMETHOD(SetCompleted)(const UInt64* files, const UInt64* bytes) override;

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR* password) override;

};
