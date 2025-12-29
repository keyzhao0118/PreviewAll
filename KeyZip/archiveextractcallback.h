#pragma once

#include <QObject>
#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include <Common/MyCom.h>

class ArchiveExtractCallBack
	: public QObject
	, public IArchiveExtractCallback
	, public ICryptoGetTextPassword
	, public CMyUnknownImp
{
	Q_OBJECT
	Z7_COM_UNKNOWN_IMP_2(IArchiveExtractCallback, ICryptoGetTextPassword)

public:
	void init(IInArchive* archive, const QString& destDirPath, const QString& password);

	// IArchiveExtractCallback
	STDMETHOD(SetTotal)(const UInt64 size) override;
	STDMETHOD(SetCompleted)(const UInt64* completedSize) override;
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode) override;
	STDMETHOD(PrepareOperation)(Int32 askExtractMode) override;
	STDMETHOD(SetOperationResult)(Int32 opRes) override;

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR* password) override;

signals:
	void requirePassword(bool& bCancel, QString& password);
	void updateProgress(quint64 completed, quint64 total);

private:
	quint64 m_totalSize = 0;
	quint64 m_completedSize = 0;

	CMyComPtr<IInArchive> m_archive;
	CMyComPtr<ISequentialOutStream> m_outStream;
	QString m_destDirPath;
	QString m_password;

	// state for current item
	UInt32 m_currentIndex = (UInt32)-1;
	QString m_currentFullPath;
	bool m_currentIsDir = false;
};

