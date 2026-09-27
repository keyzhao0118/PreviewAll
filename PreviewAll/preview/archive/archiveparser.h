#pragma once

#include <QSharedPointer>
#include <QString>
#include <Windows.h>
#include <7zip/Archive/IArchive.h>
#include <Common/MyCom.h>
#include <atomic>

class ArchiveTree;
class ArchiveTreeNode;

class ArchiveParser
{
public:
	enum class ParseResult { Succeeded, Failed, HeaderEncrypted, Cancelled };
	explicit ArchiveParser(const QString& archivePath);
	~ArchiveParser();

	void stopParse();

	const ArchiveTreeNode* getRootNode() const;
	quint64 getFileCount() const;
	quint64 getFolderCount() const;

	ParseResult parseArchive();
private:
	HRESULT tryOpenArchive(
		const QString& archivePath,
		IArchiveOpenCallback* openCallback,
		CMyComPtr<IInArchive>& outInArchive);

	bool processArchive(CMyComPtr<IInArchive> archive);
	bool checkStopParse();

	void onRequestPassword(BSTR* password);

private:
	QString m_archivePath;
	QSharedPointer<ArchiveTree> m_archiveTree;
	std::atomic_bool m_stopParse = false;
	bool m_headerEncrypted = false;

};
